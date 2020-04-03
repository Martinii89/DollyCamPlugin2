#pragma once
#pragma comment(lib, "BakkesMod.lib")
#include "bakkesmod\plugin\bakkesmodplugin.h"
#include "bakkesmod\plugin\pluginwindow.h"
#include "dollycam.h"
#include "imgui/imgui.h"

struct CameraOverride {
	bool enabled = false;
	ProfileCameraSettings cameraSettings;
};

struct SidebarSettings
{
	float width = 150;
	float height = 1080;
	float triggerWidth = 250;
	float transitionSpeed = 1.0;
	bool mouseTransition = true;
	float alpha = 0.0f;
	float posOffset = width;

	bool compact = false;
	float LocationSpeed = 1.0;
	float LocationPower = 1.0f;
	float RotationSpeed = 10.0f;
	float RotationPower = 1.0f;
	float editTimeLimit = 50;
	//int selectedFrame = -1; //for highlighting the selected snapshot ingame?
	POV originalView;
};

struct TabsSettings
{
	bool oldSnapshots = false;
	bool cameraOverride = false;
};

struct GuiState {
	TabsSettings tabsSettings;
	CameraOverride cameraOverride;
	SidebarSettings sidebarSettings;
	DollySettings dollySettings;
	bool camLock = false;
	bool showSettings = false;
};

class DollyCamPlugin : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginWindow
{
private:
	std::shared_ptr<DollyCam> dollyCam;
	std::shared_ptr<bool> renderCameraPath;
	ImFont* fa;
	CameraSnapshot selectedSnapshot;
	GuiState guiState;
	bool IsApplicable();

	void CameraLock(ServerWrapper camInput, void* params, string funcName);

	//gui stuff
	bool isWindowOpen = true;
	bool isMinimized = false;
	bool block_input = false;

public:
	virtual void onLoad();
	virtual void onUnload();

	//Info/debug methods
	void PrintSnapshotInfo(CameraSnapshot shot);

	void SaveSettings();
	void LoadSettings();

	//Engine hooks
	void onReplayOpen(std::string funcName);
	void onReplayClose(std::string funcName);
	void onTick(std::string funcName);
	void onRender(CanvasWrapper canvas);

	//Console command handlers
	void OnAllCommand(vector<string> params);
	void OnCamCommand(vector<string> params);
	void OnInReplayCommand(vector<string> params);
	void OnReplayCommand(vector<string> params);
	void OnSnapshotCommand(vector<string> params);
	void OnSnapshotModifyCommand(vector<string> params);
	void OnSetCameraSettings(vector<string> params);

	void OnLiveCommand(vector<string> params);

	//Cvar change listeners
	void OnInterpModeChanged(string oldValue, CVarWrapper newCvar);
	void OnRenderFramesChanged(string oldValue, CVarWrapper newCvar);
	void OnChaikinChanged(string oldValue, CVarWrapper newCvar);

	//Interp config methods
	void OnBezierCommand(vector<string> params);
	virtual void Render();
	bool CanUseFaFont();
	void DrawSettingsWindow();
	bool SidebarTransition(float actualWidth);
	void DrawSnapshotsNodes();
	void DrawSaveLoadSettings();
	void DrawInterpolationSettings();
	void SetStyle();
	void DrawSnapshots();
	void DrawTimeline();
	void ReadPlayerCameraSettings();
	void OverridePlayerCameraSettings();
	virtual std::string GetMenuName();
	virtual std::string GetMenuTitle();
	virtual void SetImGuiContext(uintptr_t ctx);

	// Inherited via PluginWindow
	virtual bool ShouldBlockInput() override;
	virtual bool IsActiveOverlay() override;
	virtual void OnOpen() override;
	virtual void OnClose() override;
};
