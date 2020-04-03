#pragma once
#include <memory>
#include <map>
#include "utils\customrotator.h"
#include "bakkesmod\plugin\bakkesmodplugin.h"
#include "gameapplier.h"
#include "models.h"
#include "interpstrategies/interpstrategy.h"
#include "bakkesmod\wrappers\includes.h"
#include "VisualCamera.h"

struct DollySettings
{
	bool renderDollyPath = true;
	bool renderFrameTicks = true;
	bool visualCameraActive = true;
	bool animationResetActive = false;
	std::vector<int> openFrames;
};

class DollyCam
{
private:
	std::shared_ptr<savetype> currentPath;
	std::shared_ptr<savetype> currentRenderPath;

	std::shared_ptr<IGameApplier> gameApplier;
	std::shared_ptr<InterpStrategy> locationInterpStrategy;
	std::shared_ptr<InterpStrategy> rotationInterpStrategy;
	DollySettings& settings;

	VisualCamera visualCamera;
	bool usesSameInterp = false;
	bool isActive = false;

	void UpdateRenderPath();
	void CheckIfSameInterp();
	void ResetAnimations();
	float GetAccuarateFrame();

public:
	DollyCam(std::shared_ptr<GameWrapper> _gameWrapper, std::shared_ptr<CVarManagerWrapper> _cvarManager, std::shared_ptr<IGameApplier> _gameApplier, DollySettings& _settings);
	~DollyCam();

	bool lockCamera = false;
	float anti_jitter_factor = 0.96f;
	std::shared_ptr<GameWrapper> gameWrapper;
	std::shared_ptr<CVarManagerWrapper> cvarManager;

	bool IsActive();
	void Activate();
	void Deactivate();
	void Apply();
	void Reset();
	void SetRenderPath(bool render);
	void SetRenderFrames(bool renderFrames);

	//Takes a snapshot of the current camera state and adds it to current path, returns true if taking snapshot was successfull
	CameraSnapshot TakeSnapshot(bool saveToPath);
	void InsertSnapshot(CameraSnapshot snapshot);
	bool IsFrameUsed(int frame);
	vector<int> GetUsedFrames();
	CameraSnapshot GetSnapshot(int frame);
	void DeleteFrameByIndex(int frame);

	// Changes the frame of a snapshot.
	bool ChangeFrame(int oldFrame, int newFrame);
	// Uses the frame param of the snapshot and updates a coresponding snapshot in the path.
	void UpdateFrame(CameraSnapshot snapshot);
	
	void Render(CanvasWrapper cw);
	void RefreshInterpData();
	void RefreshInterpDataRotation();
	string GetInterpolationMethod(bool locationInterp);
	shared_ptr<InterpStrategy> CreateInterpStrategy(int interpStrategy);

	bool SaveToFile(string filename);
	bool LoadFromFile(string filename);
	std::shared_ptr<savetype> GetCurrentPath();
	void SetCurrentPath(std::shared_ptr<savetype> newPath);


};
