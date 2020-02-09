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

class DollyCam
{
private:
	std::shared_ptr<savetype> currentPath;

	std::shared_ptr<IGameApplier> gameApplier;
	std::shared_ptr<InterpStrategy> locationInterpStrategy;
	std::shared_ptr<InterpStrategy> rotationInterpStrategy;

	std::shared_ptr<savetype> currentRenderPath;
	VisualCamera visualCamera;
	bool usesSameInterp = false;
	bool isActive = false;
	bool renderPath = false;
	bool renderFrames = false;
	void UpdateRenderPath();
	void CheckIfSameInterp();
	void ResetAnimations();
	float GetAccuarateFrame();

public:
	DollyCam(std::shared_ptr<GameWrapper> _gameWrapper, std::shared_ptr<CVarManagerWrapper> _cvarManager, std::shared_ptr<IGameApplier> _gameApplier);
	~DollyCam();

	std::shared_ptr<GameWrapper> gameWrapper;
	std::shared_ptr<CVarManagerWrapper> cvarManager;
	//Takes a snapshot of the current camera state and adds it to current path, returns true if taking snapshot was successfull
	CameraSnapshot TakeSnapshot(bool saveToPath);
	bool IsActive();
	void Activate();
	void Deactivate();
	void Apply();
	void Reset();
	void InsertSnapshot(CameraSnapshot snapshot);
	bool IsFrameUsed(int frame);
	CameraSnapshot GetSnapshot(int frame);
	void DeleteFrameByIndex(int frame);
	bool ChangeFrame(int oldFrame, int newFrame);
	void UpdateFrame(CameraSnapshot snapshot);
	vector<int> GetUsedFrames();
	void SetRenderPath(bool render);
	void SetRenderFrames(bool renderFrames);
	void Render(CanvasWrapper cw);
	void RefreshInterpData();
	void RefreshInterpDataRotation();
	string GetInterpolationMethod(bool locationInterp);
	shared_ptr<InterpStrategy> CreateInterpStrategy(int interpStrategy);
	bool SaveToFile(string filename);
	bool LoadFromFile(string filename);
	std::shared_ptr<savetype> GetCurrentPath();
	void SetCurrentPath(std::shared_ptr<savetype> newPath);

	bool lockCamera = false;
	bool gotLockPov = false;
	POV lockPOV;
};
