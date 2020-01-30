#include "dollycamplugin.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_timeline.h"
#include "imgui/imgui_tabs.h"
#include "serialization.h"
#include "bakkesmod\..\\utils\parser.h"
#include <functional>
#include <vector>
#include "bakkesmod/wrappers/GuiManagerWrapper.h"

static int view_index = -1;

namespace Columns
{
	bool IsItemActiveLastFrame()
	{
		ImGuiContext& g = *GImGui;
		if (g.ActiveIdPreviousFrame)
			return g.ActiveIdPreviousFrame == g.CurrentWindow->DC.LastItemId;
		return false;
	}

	bool IsItemJustMadeInactive()
	{
		return IsItemActiveLastFrame() && !ImGui::IsItemActive();
	}

	bool IsItemDeactivated()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		return (g.ActiveIdPreviousFrame == window->DC.LastItemId && g.ActiveIdPreviousFrame != 0 && g.ActiveId != window->DC.LastItemId);
	}

	struct TableColumns
	{
		string header;
		int width;
		bool enabled;
		bool widget;
		std::function<string(CameraSnapshot, int)> ToString;
		std::function<void(std::shared_ptr<DollyCam>, CameraSnapshot, int)> WidgetCode;

		int GetWidth() const { return enabled ? width : 0; }
		void RenderItem(std::shared_ptr<DollyCam> dollyCam, CameraSnapshot snap, int i) const {
			if (!enabled) return;
			if (widget)
			{
				WidgetCode(dollyCam, snap, i);
			}
			else {
				ImGui::Text(ToString(snap, i).c_str());
			}
		}
	};

	void locationWidget(std::shared_ptr<DollyCam> dollyCam, CameraSnapshot snap, int i)
	{
		static chrono::system_clock::time_point lastUpdate = chrono::system_clock::now();
		const char* widgetFormat = "";
		//ImGui::BeginGroup();
		//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		//ImGui::BeginChild(("##locationWidget" + std::to_string(i)).c_str(), { 0, ImGui::GetTextLineHeightWithSpacing() + 2 });
		ImGui::PushItemWidth(30);
		if (ImGui::DragFloat(("##x" + to_string(i)).c_str(), &snap.location.X, 1.f, 0.f, 0.f, "x"))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 50)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::SameLine();
		if (ImGui::DragFloat(("##y" + to_string(i)).c_str(), &snap.location.Y, 1.f, 0.f, 0.f, "y"))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 50)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::SameLine();
		if (ImGui::DragFloat(("##z" + to_string(i)).c_str(), &snap.location.Z, 1.f, 0.f, 0.f, "z"))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 50)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::PopItemWidth();
		//ImGui::EndChild();
		//ImGui::PopStyleVar();
	}

	void rotationWidget(std::shared_ptr<DollyCam> dollyCam, CameraSnapshot snap, int i)
	{
		static chrono::system_clock::time_point lastUpdate = chrono::system_clock::now();
		const char* widgetFormat = "%.0f";
		ImGui::PushItemWidth(30);
		if (ImGui::DragFloat(("##Pitch" + to_string(i)).c_str(), &snap.rotation.Pitch._value, 10.f, snap.rotation.Pitch._min, snap.rotation.Pitch._max, "Pitch"))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 50)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::SameLine();
		if (ImGui::DragFloat(("##Yaw" + to_string(i)).c_str(), &snap.rotation.Yaw._value, 10.f, snap.rotation.Yaw._min, snap.rotation.Yaw._max, "Yaw"))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 50)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::SameLine();
		if (ImGui::DragFloat(("##Roll" + to_string(i)).c_str(), &snap.rotation.Roll._value, 10.f, snap.rotation.Roll._min, snap.rotation.Roll._max, "Roll"))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 50)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::PopItemWidth();
	}

	void fovWidget(std::shared_ptr<DollyCam> dollyCam, CameraSnapshot snap, int i)
	{
		static chrono::system_clock::time_point lastUpdate = chrono::system_clock::now();
		const char* widgetFormat = "%.0f";
		ImGui::PushItemWidth(30);
		if (ImGui::DragFloat(("##Fov" + to_string(i)).c_str(), &snap.FOV, 0.25f, 5, 120, widgetFormat))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 50)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::PopItemWidth();
	}

	auto noWidget = [](std::shared_ptr<DollyCam> dollyCam, CameraSnapshot snap, int i) {};
	vector< TableColumns > column{
		{"#",			25,		true, false, [](CameraSnapshot snap, int i) {return to_string(i); },								noWidget},
		{"Frame",		60,		true, false, [](CameraSnapshot snap, int i) {return to_string(snap.frame); },						noWidget},
		//{"Time",		60,		true, false, [](CameraSnapshot snap, int i) {return to_string_with_precision(snap.timeStamp, 2); }, noWidget},
		{"Location",	130,	true, true, [](CameraSnapshot snap, int i) {return vector_to_string(snap.location); },				locationWidget},
		{"Rotation",	130,	true, true, [](CameraSnapshot snap, int i) {return rotator_to_string(snap.rotation.ToRotator()); },rotationWidget},
		{"FOV",			40,		true, true, [](CameraSnapshot snap, int i) {return to_string_with_precision(snap.FOV, 1); },		fovWidget},
		{"View",		40,		true, true, [](CameraSnapshot snap, int i) {return ""; },
			[](std::shared_ptr<DollyCam> dollyCam, CameraSnapshot snap, int i) {
				string buttonIdentifier = "##" + to_string(i);
				bool active = (i == view_index);
				bool pressed = ImGui::RadioButton(buttonIdentifier.c_str(), active);
				if (pressed && active) { //If this one was already active. clear out the group and unlock the camera
					view_index = -1;
					dollyCam->gameWrapper->Execute([](GameWrapper* gw) {
						gw->GetCamera().SetLockedFOV(false);
						});
				}
				if (pressed && !active) view_index = i;
				if (i == view_index)
				{
					POV pov = { snap.location, snap.rotation.ToRotator(), snap.FOV };
					dollyCam->gameWrapper->Execute([pov](GameWrapper* gw) {
						auto camera = gw->GetCamera();
						camera.SetPOV(pov);
						camera.SetFocusActor("");
					});
				}
			}},
		{"Remove",		80,		true, true, [](CameraSnapshot snap, int i) {return ""; },
			[](std::shared_ptr<DollyCam> dollyCam, CameraSnapshot snap, int i) {
				string buttonIdentifier = "Remove##" + to_string(i);
				if (ImGui::Button(buttonIdentifier.c_str()))
				{
					dollyCam->gameWrapper->Execute([dollyCam, i](GameWrapper* gw) {dollyCam->DeleteFrameByIndex(i); });
				}
			}}
	};
}

void DollyCamPlugin::Render()
{
	//int totalWidth = std::accumulate(columns.begin(), columns.end(), 0, [](int sum, const Columns::TableColumns& element) {return sum + element.GetWidth(); });


	string menuName = "Snapshots";
	if (!ImGui::Begin(menuName.c_str(), &isWindowOpen, ImGuiWindowFlags_ResizeFromAnySide))
	{
		// Early out if the window is collapsed, as an optimization.
		block_input = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
		ImGui::End();
		return;
	}


	// Make style consistent with BM
	SetStyle();
	//ImGui::BeginChild("#CurrentSnapshotsTab", ImVec2(0, 0));
	ImGui::BeginTabBar("#");
	ImGui::DrawTabsBackground();
	if (ImGui::AddTab("Keyframes"))
	{
		ImGui::BeginChild("#", ImVec2(-5, 0));
		DrawSnapshots();

		// Draw checkbox for locking the camera
		static bool lockCam = false;
		ImGui::Checkbox("Lock camera", &lockCam);;
		dollyCam->lockCamera = ImGui::IsMouseDragging() || lockCam || view_index > 0;

		//static int CinderSpinnervalue = 50;
		//ImGui::DragInt("CinderSpinner", &CinderSpinnervalue, 1.0f, 0, 100);

		DrawTimeline();
		ImGui::EndChild();
	}

	if (ImGui::AddTab("Settings"))
	{
		ImGui::BeginChild("#", ImVec2(-5, 0));
		//interpoltion method selectors
		DrawInterpolationSettings();

		ImGui::Separator();
		ImGui::Dummy({ 0, 10 });

		// Path clear\save\load
		if(ImGui::Button("Clear path")){
			dollyCam->Reset();
		}
		ImGui::SameLine();
		static char filename[256];
		ImGui::PushItemWidth(150);
		ImGui::InputText("Filename", filename, IM_ARRAYSIZE(filename));
		ImGui::PopItemWidth();
		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			dollyCam->SaveToFile(filename);
		}
		ImGui::SameLine();
		if (ImGui::Button("Load")) {
			dollyCam->LoadFromFile(filename);
		}

		ImGui::EndChild();
	}
	ImGui::EndTabBar();

	//ImGui::EndChild();
	ImGui::End();

	if (!isWindowOpen)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
	block_input = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

void DollyCamPlugin::DrawInterpolationSettings()
{
	static int interpolation_location = cvarManager->getCvar("dolly_interpmode_location").getIntValue();
	static int interpolation_rotation = cvarManager->getCvar("dolly_interpmode_rotation").getIntValue();
	static std::vector<std::string> interpolationMethods = { "Linear" , "Bezier", "Cosine", "Linear", "Catmull", "Spline" };
	//static int currentInterpolation = dollyCam->GetInterpolationMethod()

	// Draw location interpolation selector
	ImGui::Text("Select interpolation methods");
	ImGui::PushItemWidth(100);
	if (ImGui::BeginCombo("Location method", interpolationMethods.at(interpolation_location).c_str()))
	{
		for (size_t i = 0; i < interpolationMethods.size(); i++)
		{
			bool is_selected = (interpolation_location == i);
			std::string label = interpolationMethods.at(i) + "##" + std::to_string(i);
			if (ImGui::Selectable(label.c_str(), is_selected))
			{
				if (interpolation_location != i)
				{
					interpolation_location = i;
					cvarManager->getCvar("dolly_interpmode_location").setValue(std::to_string(i));
				}
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SameLine();
	// Draw rotation interpolation selector
	if (ImGui::BeginCombo("Rotation method", interpolationMethods.at(interpolation_rotation).c_str()))
	{
		for (size_t i = 0; i < interpolationMethods.size(); i++)
		{
			bool is_selected = (interpolation_rotation == i);
			std::string label = interpolationMethods.at(i) + "##" + std::to_string(i);
			if (ImGui::Selectable(label.c_str(), is_selected))
			{
				if (interpolation_rotation != i)
				{
					interpolation_rotation = i;
					cvarManager->getCvar("dolly_interpmode_rotation").setValue(std::to_string(i));
				}
			}
			if (is_selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::PopItemWidth();
}


void DollyCamPlugin::SetStyle()
{
	auto& style = ImGui::GetUserStyle();
	if (gameWrapper != nullptr)
	{
		auto bm_style_ptr = gameWrapper->GetGUIManager().GetImGuiUserStyle();
		if (bm_style_ptr != nullptr)
		{
			style = *(ImGui::ImGuiUserStyle*)bm_style_ptr;
		}
		else {
			cvarManager->log("bm style ptr was null!!");
		}
	}
	else {
		cvarManager->log("gamewrapper was null!!");
	}
}

void DollyCamPlugin::DrawSnapshots()
{
	auto columns = Columns::column;
	int enabledCount = std::accumulate(columns.begin(), columns.end(), 0, [](int sum, const Columns::TableColumns& element) {return sum + element.enabled; });
	ImGui::Columns(enabledCount, "snapshots");

	//Set column widths
	int index = 0;
	for (const auto& col : columns)
	{
		if (col.enabled)
		{
			ImGui::SetColumnWidth(index, col.width);
		}
		index++;
	}

	ImGui::Separator();

	//Set column headers
	for (const auto& col : columns)
	{
		if (col.enabled)
		{
			ImGui::Text(col.header.c_str());
			ImGui::NextColumn();
		}
		index++;
	}

	ImGui::Separator();
	ImGui::Columns(1);
	ImGui::BeginChild("", ImVec2(0, -4 * ImGui::GetTextLineHeightWithSpacing()));
	ImGui::Columns(enabledCount);

	//Set column widths
	index = 0;
	for (const auto& col : columns)
	{
		if (col.enabled)
		{
			ImGui::SetColumnWidth(index, col.width);
		}
		index++;
	}

	//Write rows of data
	index = 1;
	for (const auto& data : *dollyCam->GetCurrentPath())
	{
		auto snapshot = data.second;
		for (const auto& col : columns)
		{
			if (col.enabled)
			{
				col.RenderItem(dollyCam, snapshot, index);
				ImGui::NextColumn();
			}
		}
		index++;
	}
	ImGui::EndChild();
	ImGui::Columns(1);
}

void DollyCamPlugin::DrawTimeline()
{
	static bool showFirstSnap = false;
	static bool showPathDuration = true;
	auto replayServer = gameWrapper->GetGameEventAsReplay();
	ImGui::BeginChild("timelinechild", { 0, 30 });
	static float totalReplayTime = 100.0f;
	if (!replayServer.IsNull())
	{
		auto replay = replayServer.GetReplay();
		totalReplayTime = replay.GetNumFrames();
	}
	else {
	}
	ImGui::BeginTimeline("Timeline", totalReplayTime);

	static float currentFrame = 0;
	static float seekFrame = 0.0f;
	auto frames = dollyCam->GetUsedFrames();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 237.0 / 255, 25.0 / 255, 0, 1 });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 1.0, 72.0 / 255, 50.0 / 255, 1 });
	ImGui::PushStyleColor(ImGuiCol_ColumnActive, ImVec4{ 1, 87.0 / 255, 67.0 / 255, 1 });
	if (ImGui::TimelineMarker("currentFrame", currentFrame, "Current frame"))
	{
		seekFrame = currentFrame;
	}
	else {
		if (!replayServer.IsNull())
		{
			auto replay = replayServer.GetReplay();
			currentFrame = replay.GetCurrentFrame();
		}
	}
	ImGui::PopStyleColor(3);
	if (Columns::IsItemJustMadeInactive())
	{
		int _frame = seekFrame; //lambda can't capture static
		gameWrapper->Execute([_frame, this](GameWrapper* gw) {
			gw->GetGameEventAsReplay().SkipToFrame(_frame);
			//cvarManager->log("got this frame:" + to_string(gw->GetGameEventAsReplay().GetCurrentReplayFrame()));
			});
		//cvarManager->log("Seek to:" + to_string(seekFrame));
		//frameSkip = seekFrame;
	}

	if (showFirstSnap)
	{
		if (frames.size() > 0)
		{
			auto firstFrame = (float)frames.front();
			ImGui::TimelineMarker("FirstFrame", firstFrame, "First frame");
		}
	}

	if (showPathDuration)
	{
		if (frames.size() > 1)
		{
			auto firstFrame = (float)frames.front();
			auto lastFrame = (float)frames.back();
			float values[2] = { firstFrame, lastFrame };
			ImGui::TimelineEvent("", values);
		}
	}
	ImGui::EndTimeline();
	ImGui::EndChild();
}

std::string DollyCamPlugin::GetMenuName()
{
	return "dollycam";
}

std::string DollyCamPlugin::GetMenuTitle()
{
	return "Dollycam";
}

void DollyCamPlugin::SetImGuiContext(uintptr_t ctx)
{
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}

bool DollyCamPlugin::ShouldBlockInput()
{
	return block_input;
}

bool DollyCamPlugin::IsActiveOverlay()
{
	return true;
}

void DollyCamPlugin::OnOpen()
{
	isWindowOpen = true;
}

void DollyCamPlugin::OnClose()
{
	isWindowOpen = false;
}