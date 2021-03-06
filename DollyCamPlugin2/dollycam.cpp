#include "pch.h"
#include "dollycam.h"
#include "bakkesmod\wrappers\gamewrapper.h"
#include "bakkesmod\wrappers\replayserverwrapper.h"
#include "bakkesmod\wrappers\GameObject\CameraWrapper.h"
#include "utils/parser.h"
#include "utils/io.h"

#include "interpstrategies\supportedstrategies.h"
#include "serialization.h"
#include "UE4MathConverters.h"

void DollyCam::UpdateRenderPath()
{
	if (!gameWrapper->IsInReplay())
		return;
	currentRenderPath = make_shared<savetype>(savetype());
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode_location");
	auto locationRenderStrategy = CreateInterpStrategy(interpMode.getIntValue());
	auto firstFrame = currentPath->begin();
	//float beginTime = firstFrame->second.timeStamp;

	int startFrame = firstFrame->first;
	int endFrame = (--currentPath->end())->first;

	float replayTickRate = 1.f / (float)gameWrapper->GetGameEventAsReplay().GetReplayFPS();

	int lastSyncedFrame = startFrame;
	float timePerFrame = replayTickRate;
	for (int i = startFrame; i <= endFrame; i++)
	{
		if (currentPath->find(i) != currentPath->end())
		{
			lastSyncedFrame = i;
			auto currentSnapshot = currentPath->find(i);
			//beginTime = currentSnapshot->second.timeStamp;
			timePerFrame = replayTickRate;
			//auto nextSnapshot = currentPath->upper_bound(i);
			//timePerFrame = (nextSnapshot->second.timeStamp - beginTime) / (nextSnapshot->second.frame - currentSnapshot->second.frame);
			//if (timePerFrame < .01f || timePerFrame > .08f) //outliers
			//	timePerFrame = replayTickRate;
		}

		CameraSnapshot snapshot;
		snapshot.frame = i;
		//snapshot.timeStamp = beginTime + (timePerFrame * (i - lastSyncedFrame));
		NewPOV pov = locationRenderStrategy->GetPOV(/*snapshot.timeStamp,*/ i);
		snapshot.location = pov.location;
		snapshot.rotation =  CustomRotator(pov.rotation_rotator);
		snapshot.rotation_rotator = pov.rotation_rotator;
		snapshot.FOV = pov.FOV;

		if (snapshot.FOV > 1)
			currentRenderPath->insert(make_pair(i, snapshot));
	}
}

void DollyCam::CheckIfSameInterp()
{
	usesSameInterp = cvarManager->getCvar("dolly_interpmode_location").getIntValue() == cvarManager->getCvar("dolly_interpmode_rotation").getIntValue();
}

void DollyCam::ResetAnimations()
{
	gameWrapper->ExecuteUnrealCommand("SET SeqAct_Interp Position 0.0");
}

float DollyCam::GetAccuarateFrame()
{
	if (!gameWrapper->IsInReplay())
		return -1.0f;
	auto replay = gameWrapper->GetGameEventAsReplay().GetReplay();

	auto recordedFPS = replay.GetRecordFPS();
	// magic stutter free number..
	//if (recordedFPS < 120)
	//{
	//	auto magic = (0.96 - 30 * 0.04/90) + (0.04 / 90) * recordedFPS;
	//	recordedFPS *= magic;
	//}

	recordedFPS *= anti_jitter_factor;

	auto recordedFrameTime = 1.0f / recordedFPS;
	auto dt = replay.GetAccumulatedDeltaTime();
	auto delta_frame = dt / recordedFrameTime;
	//delta_frame = std::min(1.0f, delta_frame);
	auto floatFrame = replay.GetCurrentFrame() + delta_frame;


	//static double lastFloatFrame = 0;
	//if (lastFloatFrame != floatFrame)
	//{
	//	cvarManager->log(std::to_string(replay.GetCurrentFrame()) + "|" + std::to_string(replay.GetRecordFPS())+ "|" + std::to_string(dt));
	//}
	//lastFloatFrame = floatFrame;
	return floatFrame;
}

DollyCam::DollyCam(std::shared_ptr<GameWrapper> _gameWrapper, std::shared_ptr<CVarManagerWrapper> _cvarManager, std::shared_ptr<IGameApplier> _gameApplier, DollySettings& _settings) : settings(_settings)
{
	currentPath = std::unique_ptr<savetype>(new savetype());
	gameWrapper = _gameWrapper;
	cvarManager = _cvarManager;
	gameApplier = _gameApplier;
}

DollyCam::~DollyCam()
{
}

CameraSnapshot DollyCam::TakeSnapshot(bool saveToPath)
{
	CameraSnapshot save;
	if (!gameWrapper->IsInReplay())
		return save;

	ReplayServerWrapper sw = gameWrapper->GetGameEventAsReplay();
	auto replay = sw.GetReplay();
	CameraWrapper flyCam = gameWrapper->GetCamera();
	if (sw.IsNull())
		return save;

	//save.timeStamp = sw.GetReplayTimeElapsed();
	//save.timeStamp = sw.GetCurrentReplayFrame()/replay.GetRecordFPS();
	save.FOV = flyCam.GetFOV();
	save.location = flyCam.GetLocation();
	save.rotation = CustomRotator(flyCam.GetRotation());
	save.rotation_rotator = flyCam.GetRotation();
	save.frame = sw.GetCurrentReplayFrame();

	if (saveToPath) {
		this->InsertSnapshot(save);
	}

	return save;
}

bool DollyCam::IsActive()
{
	return isActive;
}

void DollyCam::Activate()
{
	if (isActive)
		return;

	isActive = true;
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode_location");
	locationInterpStrategy = CreateInterpStrategy(interpMode.getIntValue());
	cvarManager->log("Dollycam activated");
}

void DollyCam::Deactivate()
{
	if (!isActive)
		return;
	CameraWrapper flyCam = gameWrapper->GetCamera();
	if (!flyCam.GetCameraAsActor().IsNull()) {
		flyCam.SetLockedFOV(false);
	}
	isActive = false;
	cvarManager->log("Dollycam deactivated");
}

//float lastWrite = -5000.f;
//float diff = .0f;
//bool isFirst = true;
void DollyCam::Apply()
{
	int currentFrame = 0;
	//ServerWrapper sw(NULL);
	if (gameWrapper->IsInReplay()) {
		currentFrame = gameWrapper->GetGameEventAsReplay().GetCurrentReplayFrame();
		//sw = gameWrapper->GetGameEventAsReplay();
	}
	else if (gameWrapper->IsInGame()) {
		currentFrame = gameWrapper->GetGameEventAsServer().GetReplayDirector().GetReplay().GetCurrentFrame();
		//sw = gameWrapper->GetGameEventAsServer();
	}
	else
	{
		return;
	}
	if (currentFrame < currentPath->begin()->first || currentFrame >(--currentPath->end())->first)
		return;
	if (currentFrame == currentPath->begin()->first)
	{

		if (settings.animationResetActive)
		{
			ResetAnimations();
		}
	}

	auto floatFrame = GetAccuarateFrame();

	NewPOV pov = locationInterpStrategy->GetPOV(/*oldTiming,*/ floatFrame);
	if (!usesSameInterp)
	{
		NewPOV secondaryPov = rotationInterpStrategy->GetPOV(/*oldTiming,*/ floatFrame);
		pov.rotation_rotator = secondaryPov.rotation_rotator;
		pov.FOV = secondaryPov.FOV;
	}
	if (pov.FOV < 1) { //Invalid camerastate
		return;
	}
	gameApplier->SetPOV(pov.location, pov.rotation_rotator, pov.FOV);
}

void DollyCam::Reset()
{
	this->currentPath->clear();
	this->RefreshInterpData();
	this->RefreshInterpDataRotation();
}

void DollyCam::InsertSnapshot(CameraSnapshot snapshot)
{
	this->currentPath->insert_or_assign(snapshot.frame, snapshot);
	this->RefreshInterpData();
	this->RefreshInterpDataRotation();
	SaveToFile("_temp.json");
}

bool DollyCam::IsFrameUsed(int frame)
{
	return currentPath->find(frame) != currentPath->end();
}

CameraSnapshot DollyCam::GetSnapshot(int frame)
{
	CameraSnapshot snapshot;
	auto it = currentPath->find(frame);
	if (it != currentPath->end())
	{
		snapshot = it->second;
	}
	else {
		snapshot.frame = -1;
	}
	return snapshot;
}

void DollyCam::DeleteFrameByIndex(int index)
{
	int i = 1;
	auto it = currentPath->begin();
	while (it != currentPath->end())
	{
		if (i == index)
		{
			int frame = it->second.frame;
			currentPath->erase(it);
			cvarManager->log("Deleted snapshot #" + to_string(index) + " with ID: " + to_string(frame));
			break;
		}
		i++; it++;
	}
	this->RefreshInterpData();
	this->RefreshInterpDataRotation();
}

bool DollyCam::ChangeFrame(const int oldFrame, const int newFrame)
{
	const auto it = currentPath->find(oldFrame);
	if (it != currentPath->end())
	{
		currentPath->erase(it);
		it->second.frame = newFrame;
		InsertSnapshot(it->second);

		return true;
	}
	return false;
}

void DollyCam::UpdateFrame(CameraSnapshot snapshot)
{
	int frame = snapshot.frame;
	auto old = GetSnapshot(frame);
	if (old.frame == frame)
	{
		InsertSnapshot(snapshot);
		//cvarManager->log("updated frame");
	}
	else {
		cvarManager->log("Invalid snapshot update.");
	}
}

vector<int> DollyCam::GetUsedFrames()
{
	vector<int> frames = vector<int>();
	for (auto it = currentPath->begin(); it != currentPath->end(); it++)
	{
		frames.push_back(it->first);
	}
	return frames;
}

void DollyCam::SetRenderPath(bool render)
{
	settings.renderDollyPath = render;
}

void DollyCam::SetRenderFrames(bool _renderFrames)
{
	settings.renderFrameTicks = _renderFrames;
}
inline float Dot(Rotator rot, Vector line)
{
	Vector fov = RotatorToVector(rot);
	return Vector::dot(fov, line);
}
void DollyCam::Render(CanvasWrapper cw)
{
	//if (!renderPath || !currentRenderPath || currentRenderPath->size() < 2)
	if (!settings.renderDollyPath || !currentRenderPath || currentRenderPath->size() < 2)
		return;

	ReplayServerWrapper sw = gameWrapper->GetGameEventAsReplay();
	CameraWrapper cam = gameWrapper->GetCamera();
	auto location = cam.GetLocation();
	auto rotation = cam.GetRotation();
	int currentFrame = sw.GetCurrentReplayFrame();

	Vector2 prevLine = cw.Project(currentRenderPath->begin()->second.location);
	Vector2 canvasSize = cw.GetSize();

	int colTest = 255;
	for (auto it = (++currentRenderPath->begin()); it != currentRenderPath->end(); ++it)
	{
		Vector2 line = cw.Project(it->second.location);
		if (!IsActive() && it->first == currentFrame && settings.visualCameraActive)
		{
			visualCamera.DrawCamera(cw, it->second.location, it->second.rotation.ToRotator(), 2.0f);
		}
		if (it->first - 2 < currentFrame && it->first + 2 > currentFrame)
		{
			cw.SetColor(255, 0, 0, 255);
		}
		else
		{
			cw.SetColor(0, 0, colTest, 255);
			colTest = colTest == 0 ? 255 : 255;
		}

		line.X = max(0, line.X);
		line.X = min(line.X, canvasSize.X);
		line.Y = max(0, line.Y);
		line.Y = min(line.Y, canvasSize.Y);
		bool inFrustum = false;
		Vector cam_to_line = (it->second.location - location);
		cam_to_line.normalize();
		float cam_dot_line = Dot(rotation, cam_to_line);
		if (cam_dot_line > 0) inFrustum = true;
		if (inFrustum) {
			if (!(((line.X < .1 || line.X >= canvasSize.X - .5) && (line.Y < .1 || line.Y >= canvasSize.Y - .5)) || ((prevLine.X < .1 || prevLine.X >= canvasSize.X - .5) && (prevLine.Y < .1 || prevLine.Y >= canvasSize.Y - .5))))
			{
				cw.DrawLine(prevLine, line);
				cw.DrawLine(prevLine.minus({ 1,1 }), line.minus({ 1,1 })); //make lines thicker
				cw.DrawLine(prevLine.minus({ -1,-1 }), line.minus({ -1,-1 }));
				if (settings.renderFrameTicks) {
					cw.SetColor(255, 255, 255, 255);
					cw.SetPosition(line);
					cw.DrawBox(Vector2{ 2, 2 });
					//cw.DrawString(to_string(it->first));
				}
			}
		}
		prevLine = line;
	}

	int index = 1;
	for (auto it = currentPath->begin(); it != currentPath->end(); it++)
	{
		auto boxLoc = cw.Project(it->second.location);
		bool inFrustum = false;
		Vector cam_to_box = (it->second.location - location);
		cam_to_box.normalize();
		float cam_dot_line = Dot(rotation, cam_to_box);
		if (cam_dot_line > 0) inFrustum = true;
		if (inFrustum) {
			cw.SetColor(255, 0, 0, 255);
			if (std::count(settings.openFrames.begin(), settings.openFrames.end(), it->second.frame))
			{
				cw.SetColor(252, 255, 0, 255);
			}
			if (boxLoc.X >= 0 && boxLoc.X <= canvasSize.X && boxLoc.Y >= 0 && boxLoc.Y <= canvasSize.Y) {
				boxLoc.X -= 5;
				boxLoc.Y -= 5;
				cw.SetPosition(boxLoc);
				auto tmp = Vector2();
				tmp.X = 10; tmp.Y = 10;
				cw.FillBox(tmp);
				cw.SetColor(255, 255, 255, 255);
				cw.DrawString("(" + to_string(index) + ")" + " (ID:" + to_string(it->first) + ", w:" + to_string_with_precision(it->second.weight, 2) + ")");
			}
		}
		index++;
	}
	static int lAltIndex = gameWrapper->GetFNameIndexByString("LeftAlt");
	bool altPressed = gameWrapper->IsKeyPressed(lAltIndex);
	bool _lockCamera = altPressed ? !lockCamera : lockCamera;
	if (_lockCamera)
	{
		auto size = cw.GetSize();
		Vector2 topRight = { size.X - 220, 0 };
		cw.SetPosition(topRight);
		cw.SetColor(255, 0, 0, 255);
		cw.DrawString("CAMERA LOCKED", 2, 2);
	}
	//auto rot = ToFRotator(rotation);
	//auto q = UE4Math::FQuat(rot);
	//auto rot2 = q.Rotator();
	//auto rotator = ToBMRotator(rot2);
	////cam.SetRotation(rotator);
	//cw.SetColor(255, 255, 255, 255);
	//cw.SetPosition(Vector2({ 0, 0 }));
	//cw.DrawString("camera FFRotator: " + std::to_string(rot.Pitch) + ",  " + std::to_string(rot.Yaw) + ",  " + std::to_string(rot.Roll));
	//cw.SetPosition(Vector2({ 0, 20 }));
	//cw.DrawString("FQuat values: [X:" + std::to_string(q.X) + ", Y:" + std::to_string(q.Y) + ", Z:" + std::to_string(q.Z) + ", W:" + std::to_string(q.W));
	//cw.SetPosition(Vector2({ 0, 40 }));
	//cw.DrawString("rot->quat->rot: " + std::to_string(rot2.Pitch) + ",  " + std::to_string(rot2.Yaw) + ",  " + std::to_string(rot2.Roll));
	//
	//auto replay = sw.GetReplay();
	//CameraWrapper flyCam = gameWrapper->GetCamera();

	//auto timeStamp = sw.GetReplayTimeElapsed();
	//auto frame = sw.GetCurrentReplayFrame();
	//auto replay_frame = replay.GetCurrentFrame();
	//auto replay_playbacktime = replay.GetPlaybackTime();
	//auto dt = replay.GetAccumulatedDeltaTime();
	//cw.SetPosition(Vector2({ 0, 60 }));
	//cw.DrawString("Timestamp: " + std::to_string(timeStamp));
	//cw.SetPosition(Vector2({ 0, 80 }));
	//cw.DrawString("Frame: " + std::to_string(frame));

	//cw.SetPosition(Vector2({ 0, 100 }));
	//cw.DrawString("Replay Frame: " + std::to_string(replay_frame));

	//cw.SetPosition(Vector2({ 0, 120 }));
	//cw.DrawString("Replay playback time: " + std::to_string(replay_playbacktime));	
	//cw.SetPosition(Vector2({ 0, 140 }));
	//cw.DrawString("Acumulated delta time: " + std::to_string(dt));
}

void DollyCam::RefreshInterpData()
{
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode_location");
	locationInterpStrategy = CreateInterpStrategy(interpMode.getIntValue());
	UpdateRenderPath();
	CheckIfSameInterp();
}

void DollyCam::RefreshInterpDataRotation()
{
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode_rotation");
	rotationInterpStrategy = CreateInterpStrategy(interpMode.getIntValue());
	if (locationInterpStrategy->GetName().compare((rotationInterpStrategy)->GetName()) == 0)
	{
		rotationInterpStrategy = locationInterpStrategy;
	}
	CheckIfSameInterp();
}

string DollyCam::GetInterpolationMethod(bool locationInterp)
{
	if (locationInterp && locationInterpStrategy)
		return locationInterpStrategy->GetName();
	else if (rotationInterpStrategy)
		return rotationInterpStrategy->GetName();
	return "none";
}

shared_ptr<InterpStrategy> DollyCam::CreateInterpStrategy(int interpStrategy)
{
	int chaikinDegree = cvarManager->getCvar("dolly_chaikin_degree").getIntValue();
	switch (interpStrategy)
	{
	case 0:
		return std::make_shared<LinearInterpStrategy>(LinearInterpStrategy(currentPath, chaikinDegree));
		break;
	case 1:
		return std::make_shared<NBezierInterpStrategy>(NBezierInterpStrategy(currentPath, chaikinDegree));
		break;
	case 2:
		return std::make_shared<CosineInterpStrategy>(CosineInterpStrategy(currentPath));
		break;
	case 3:
		return std::make_shared<LinearInterpStrategy>(LinearInterpStrategy(currentPath, chaikinDegree));
		//return std::make_shared<HermiteInterpStrategy>(HermiteInterpStrategy(currentPath));
		break;
	case 4:
		return std::make_shared<CatmullRomInterpStrategy>(CatmullRomInterpStrategy(currentPath, chaikinDegree));
		break;
	case 5:
		auto tmp = std::make_shared<SplineInterpStrategy>(SplineInterpStrategy(currentPath, chaikinDegree));
		tmp->cvarManager = cvarManager;
		return tmp;
	}

	cvarManager->log("Interpstrategy not found!!! Defaulting to linear interp.");
	return std::make_shared<LinearInterpStrategy>(LinearInterpStrategy(currentPath, chaikinDegree));
}

bool DollyCam::SaveToFile(string filename)
{
	string fullPath = "bakkesmod/data/campaths/" + filename;
	std::map<string, CameraSnapshot> pathCopy;
	for (auto& i : *currentPath)
	{
		pathCopy.insert_or_assign(to_string(i.first), i.second);
	}
	json j = pathCopy;
	ofstream myfile;
	myfile.open(fullPath);
	if (myfile)
	{
		myfile << j.dump(4);
	}
	else {
		return false;
	}
	myfile.close();
	if (myfile.bad())
	{
		return false;
	}
	return true;
}

bool DollyCam::LoadFromFile(string filename)
{
	string fullPath = "bakkesmod/data/campaths/" + filename;
	if (!file_exists(fullPath))
	{
		cvarManager->log("File does not exist!");
		return false;
	}
	std::ifstream i(fullPath);
	json j;
	i >> j;
	currentPath->clear();
	auto v8 = j.get<std::map<string, CameraSnapshot>>();
	for (auto& i : v8)
	{
		string first = i.first;
		int intVal = get_safe_int(first);
		CameraSnapshot value = i.second;
		currentPath->insert_or_assign(intVal, value);
	}

	this->RefreshInterpData();
	this->RefreshInterpDataRotation();
	return true;
}

std::shared_ptr<savetype> DollyCam::GetCurrentPath()
{
	return currentPath;
}

void DollyCam::SetCurrentPath(std::shared_ptr<savetype> newPath)
{
	currentPath = newPath;
}