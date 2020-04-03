#include "pch.h"
#include "dollycamplugin.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_timeline.h"
#include "serialization.h"
#include "bakkesmod\..\\utils\parser.h"
#include <functional>
#include <vector>
#include <set>
#include <Shlwapi.h>
#include "bakkesmod/wrappers/GuiManagerWrapper.h"
#include "imgui/IconsFontAwesome5.h"
#pragma comment(lib, "Shlwapi.lib")

static int view_index = -1;

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

bool IsItemJustMadeActive()
{
	return ImGui::IsItemActive() && !IsItemActiveLastFrame();
}

bool IsItemDeactivated()
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	return (g.ActiveIdPreviousFrame == window->DC.LastItemId && g.ActiveIdPreviousFrame != 0 && g.ActiveId != window->DC.LastItemId);
}

bool SliderFloatWithSteps(const char* label, float* v, float v_min, float v_max, float v_step, const char* display_format)
{
	if (!display_format)
		display_format = "%.3f";

	char text_buf[64] = {};
	ImFormatString(text_buf, IM_ARRAYSIZE(text_buf), display_format, *v);

	// Map from [v_min,v_max] to [0,N]
	const int countValues = int((v_max - v_min) / v_step);
	int v_i = int((*v - v_min) / v_step);
	const bool value_changed = ImGui::SliderInt(label, &v_i, 0, countValues, text_buf);

	// Remap from [0,N] to [v_min,v_max]
	*v = v_min + float(v_i) * v_step;
	return value_changed;
}

void DragInPlace()
{
	static POINT point;
	if (IsItemJustMadeActive()) {
		GetCursorPos(&point);
	}
	if (ImGui::IsItemActive())
	{
		ImGuiContext& g = *GImGui;
		//g.DragLastMouseDelta.x = -ImGui::GetMouseDragDelta(0, 1).x;
		g.DragCurrentAccum = -ImGui::GetMouseDragDelta(0, 1).x;
		SetCursorPos(point.x, point.y);
	}
}

void BeginAltBg(int i)
{
	auto bg = ImVec4{ 25 / 255.f, 25 / 255.f, 25 / 255.f, 255 / 255.f };
	auto bg_alt = ImVec4{ 46 / 255.f, 46 / 255.f, 46 / 255.f, 255 / 255.f };
	auto bg_hover = ImVec4{ 68 / 255.f, 68 / 255.f, 68 / 255.f, 255 / 255.f };
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, bg_hover);
	if (i % 2 == 0)
	{
		ImGui::PushStyleColor(ImGuiCol_Header, bg_alt);
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Header, bg);
	}
}

void EndAltBg(int i)
{
	ImGui::PopStyleColor(2);
}

static const std::vector<std::string> LegalHotkeys{ "F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","Escape","Tab","Tilde","ScrollLock","Pause","one","two","three","four","five","six","seven","eight","nine","zero","Underscore","Equals","Backslash","LeftBracket","RightBracket","Enter","CapsLock","Semicolon","Quote","LeftShift","Comma","Period","Slash","RightShift","LeftControl","LeftAlt","SpaceBar","RightAlt","RightControl","Left","Up","Down","Right","Home","End","Insert","PageUp","Delete","PageDown","NumLock","Divide","Multiply","Subtract","Add","PageDown","NumPadOne","NumPadTwo","NumPadThree","NumPadFour","NumPadFive","NumPadSix","NumPadSeven","NumPadEight","NumPadNine","NumPadZero","Decimal","LeftMouseButton","RightMouseButton","ThumbMouseButton","ThumbMouseButton2","MouseScrollUp","MouseScrollDown","MouseX","MouseY","XboxTypeS_LeftThumbStick","XboxTypeS_RightThumbStick","XboxTypeS_DPad_Up","XboxTypeS_DPad_Left","XboxTypeS_DPad_Right","XboxTypeS_DPad_Down","XboxTypeS_Back","XboxTypeS_Start","XboxTypeS_Y","XboxTypeS_X","XboxTypeS_B","XboxTypeS_A","XboxTypeS_LeftShoulder","XboxTypeS_RightShoulder","XboxTypeS_LeftTrigger","XboxTypeS_RightTrigger","XboxTypeS_LeftTriggerAxis","XboxTypeS_RightTriggerAxis","XboxTypeS_LeftX","XboxTypeS_LeftY","XboxTypeS_RightX","XboxTypeS_RightY" };

bool DrawHotkeySelection(const char* label, const char** selected, std::set<std::string> boundKeys)
{
	bool retValue = false;
	static char filter[256];
	static bool focus = true;
	auto& style = ImGui::GetStyle();
	//auto oldAlpha = style.Alpha;
	//style.Alpha = 1.0;
	//ImGui::Text(std::to_string(style.Alpha).c_str());
	auto popupbg = ImGui::GetStyleColorVec4(ImGuiCol_PopupBg);
	popupbg.w = 1.0;
	ImGui::PushStyleColor(ImGuiCol_PopupBg, popupbg);
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1);
	if (ImGui::BeginCombo(label, *selected))
	{
		if (focus) {
			focus = false;
			ImGui::SetKeyboardFocusHere(1);
		}
		std::string lbl = "##keyfilter" + std::string(label);
		ImGui::InputText(lbl.c_str(), filter, IM_ARRAYSIZE(filter));
		for (size_t i = 0; i < LegalHotkeys.size(); i++)
		{
			if (strlen(filter) > 0 && StrStrIA(LegalHotkeys[i].c_str(), filter) == nullptr) continue;
			bool this_selected = !strcmp(*selected, LegalHotkeys[i].c_str());
			bool already_Bound = (boundKeys.find(LegalHotkeys[i]) != boundKeys.end());
			if (already_Bound)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0, 1));
				ImGui::Text((LegalHotkeys[i] + " (already bound)").c_str());
				ImGui::PopStyleColor();
			}
			else {
				if (ImGui::Selectable(LegalHotkeys[i].c_str(), this_selected))
				{
					*selected = LegalHotkeys[i].c_str();
					retValue = true;
				}
			}
		}
		ImGui::EndCombo();
	}
	if (IsItemJustMadeInactive())
	{
		focus = true;
		memset(filter, 0, sizeof filter);
	}
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	return retValue;
}

std::map<std::string, std::string> GetAllBindings(std::shared_ptr<CVarManagerWrapper> cvarManager)
{
	auto bindings = std::map<std::string, std::string>();
	for (auto& hotkey : LegalHotkeys)
	{
		auto bind = cvarManager->getBindStringForKey(hotkey);
		if (!bind.empty())
		{
			bindings.emplace(bind, hotkey);
		}
	}
	return bindings;
}

std::set<std::string> GetBoundKeysFromBindings(std::map<std::string, std::string> bindings)
{
	std::set<std::string> ret;
	for (auto& bind : bindings)
	{
		ret.emplace(bind.second);
	}
	return ret;
}

void Rebind(std::shared_ptr<CVarManagerWrapper> cvarManager, std::string command, std::string oldKey, std::string newHotkey)
{
	//Unbind old
	if (!oldKey.empty()) {
		cvarManager->executeCommand("unbind " + oldKey);
	}
	if (!newHotkey.empty())
	{
		cvarManager->setBind(newHotkey, command);
	}
}

static void ShowHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(450.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

namespace Columns
{
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

static std::string saveloadStatus = "";

void SetStatusTimeout(std::shared_ptr<GameWrapper> gw, std::string status)
{
	saveloadStatus = status;
	gw->SetTimeout([](GameWrapper*) {saveloadStatus = ""; }, 2);
}

void DollyCamPlugin::Render()
{

	if (CanUseFaFont())
		ImGui::PushFont(fa);
	// Make style consistent with BM
	SetStyle();
	//int totalWidth = std::accumulate(columns.begin(), columns.end(), 0, [](int sum, const Columns::TableColumns& element) {return sum + element.GetWidth(); });

	auto& sidebarSetting = guiState.sidebarSettings;
	float actualWidth = sidebarSetting.width + (sidebarSetting.compact ? 0.0f : 50.0f);
	/*SidebarTransition(actualWidth);*/

	ImGui::SetNextWindowPos({ 0 - (sidebarSetting.posOffset), 0 }, ImGuiCond_Always);
	ImGui::SetNextWindowSize({ actualWidth, sidebarSetting.height }, ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(sidebarSetting.alpha);
	if (ImGui::Begin("##sidebar", &isWindowOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar)) {
		if (SidebarTransition(actualWidth))
		{
			DrawSnapshotsNodes();
			if (ImGui::Button(ICON_FA_BACKWARD))
			{
				gameWrapper->Execute([this](GameWrapper* gw) {
					auto firstFrame = max(0, dollyCam->GetUsedFrames().front() - 30); //add some padding
					auto replayServer = gw->GetGameEventAsReplay();
					replayServer.SkipToFrame(firstFrame);
					//cvarManager->log("got this frame:" + to_string(gw->GetGameEventAsReplay().GetCurrentReplayFrame()));
					});
			}ImGui::SameLine();
			if (!dollyCam->IsActive() && ImGui::Button("Activate")) {
				dollyCam->Activate();
			}
			if (dollyCam->IsActive() && ImGui::Button("Deactivate")) {
				dollyCam->Deactivate();
			}ImGui::SameLine();

			if (ImGui::Button(ICON_FA_CAMERA)) {
				gameWrapper->Execute([this](GameWrapper*) {dollyCam->TakeSnapshot(true); });
			}
		}
		ImGui::End();

	}

	//bool show_styles = true;
	//ImGui::Begin("Style Editor", &show_styles); ImGui::ShowStyleEditor(); ImGui::End();

	if (guiState.showSettings) {
		DrawSettingsWindow();
	}

	if (!isWindowOpen)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
	dollyCam->lockCamera = ImGui::IsMouseDragging(0) || guiState.camLock || view_index > 0;
	block_input = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
	if (CanUseFaFont())
		ImGui::PopFont();
}

bool DollyCamPlugin::CanUseFaFont()
{
	auto io = ImGui::GetIO();
	return fa != nullptr && fa->IsLoaded() && io.Fonts->IsBuilt();
}

void DollyCamPlugin::DrawSettingsWindow()
{
	string menuName = "Settings";
	ImGui::SetNextWindowPos({ 600,0 }, ImGuiCond_Once);
	ImGui::SetNextWindowSize({ 600,500 }, ImGuiCond_Once);
	if (!ImGui::Begin(menuName.c_str(), &guiState.showSettings))
	{
		// Early out if the window is collapsed, as an optimization.
		block_input = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
		ImGui::End();
		return;
	}

	auto& tabSettings = guiState.tabsSettings;
	static ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_Reorderable;
	ImGui::BeginTabBar("#", tab_bar_flags);
	if (ImGui::BeginTabItem("Settings"))
	{
		ImGui::BeginChild("#", ImVec2(-5, -50));
		//interpoltion method selectors
		if (ImGui::CollapsingHeader("Interpolation Settings")) {
			DrawInterpolationSettings();
		}

		if (ImGui::CollapsingHeader("Dolly Settings")) {
			auto& dollySettings = guiState.dollySettings;

			ImGui::Checkbox("Render the dolly path", &dollySettings.renderDollyPath);
			ImGui::Checkbox("Render frame ticks on the path", &dollySettings.renderFrameTicks);
			ImGui::Checkbox("Render the visual camera on the path", &dollySettings.visualCameraActive);
			ImGui::Checkbox("Reset animation on the first frame of the path", &dollySettings.animationResetActive);
		}
		// Path clear\save\load
		if (ImGui::CollapsingHeader("Path Managment")) {
			DrawSaveLoadSettings();
		}

		if (ImGui::CollapsingHeader("Sidebar Settings")) {
			auto& sidebarSettings = guiState.sidebarSettings;

			ImGui::Checkbox("Compact", &sidebarSettings.compact);
			ImGui::Text("Window parameters");
			ImGui::PushItemWidth(150);
			ImGui::SliderFloat("Width", &sidebarSettings.width, 100, 500, "%.0f");
			ImGui::SliderFloat("Height", &sidebarSettings.height, 100, 1080, "%.0f");
			ImGui::Checkbox("Mouse trigger", &sidebarSettings.mouseTransition);
			ImGui::SliderFloat("Trigger Width", &sidebarSettings.triggerWidth, 100, 500, "%.0f");
			ImGui::SliderFloat("Transition Speed", &sidebarSettings.transitionSpeed, 0.1, 3, "%.1f");

			ImGui::Separator();
			ImGui::Dummy({ 0, 10 });

			ImGui::Text("Slider parameters");

			ImGui::DragFloat("Location sensitivity", &sidebarSettings.LocationSpeed, 1.0f, 0.0f, 0.0f, "%.2f");
			ImGui::DragFloat("Location power(expontential)", &sidebarSettings.LocationPower, 1.0f, 0.0f, 0.0f, "%.2f");
			ImGui::DragFloat("Rotation sensitivity", &sidebarSettings.RotationSpeed, 1.0f, 0.0f, 0.0f, "%.2f");
			ImGui::DragFloat("Rotation power(expontential)", &sidebarSettings.RotationPower, 1.0f, 0.0f, 0.0f, "%.2f");

			SliderFloatWithSteps("Live edit smoothness (may affect performance)", &sidebarSettings.editTimeLimit, 1.0f, 6.0f, 1.0f, "%.0f");

			ImGui::PopItemWidth();
		}

		if (ImGui::CollapsingHeader("Tab Settings"))
		{
			ImGui::Checkbox("Show old snapshot tab", &tabSettings.oldSnapshots);
			ImGui::Checkbox("Show Camera override tab", &tabSettings.cameraOverride);
		}

		if (ImGui::CollapsingHeader("Hotkeys"))
		{
			//ImGui::Text("NOT FUNCTIONAL");
			static bool filter_dolly = true;
			ImGui::Checkbox("Dolly filter", &filter_dolly);
			static auto allBindings = GetAllBindings(cvarManager);
			static std::set<std::string> boundKeys = GetBoundKeysFromBindings(allBindings);

			for (auto& binding : allBindings) {
				auto command = binding.first;
				if (filter_dolly && command.find("dolly") == std::string::npos){
					continue;

				}
				auto key = binding.second;
				auto newKey = key.c_str();
				if (DrawHotkeySelection(command.c_str(), &newKey, boundKeys))
				{
					Rebind(cvarManager, command, key, newKey);
					boundKeys.erase(key);
					boundKeys.emplace(newKey);
					allBindings[command] = newKey;
				}
			}
		}

		ImGui::EndChild();
		ImGui::EndTabItem();
	}

	if (tabSettings.oldSnapshots && ImGui::BeginTabItem("Keyframes"))
	{
		ImGui::BeginChild("#", ImVec2(-5, -50));
		DrawSnapshots();

		// Draw checkbox for locking the camera
		ImGui::Checkbox("Lock camera", &guiState.camLock);;

		//static int CinderSpinnervalue = 50;
		//ImGui::DragInt("CinderSpinner", &CinderSpinnervalue, 1.0f, 0, 100);

		DrawTimeline();
		ImGui::EndChild();
		ImGui::EndTabItem();
	}

	if (tabSettings.cameraOverride && ImGui::BeginTabItem("Camera override"))
	{
		auto& settings = guiState.cameraOverride;
		auto& overrideSettings = settings.cameraSettings;
		ImGui::BeginChild("#", ImVec2(-5, -50));

		ImGui::Text("Use these settings to override the player camera");
		ImGui::Checkbox("Enabled", &settings.enabled);

		SliderFloatWithSteps("FOV", &overrideSettings.FOV, 60, 110, 10, "%.0f");
		SliderFloatWithSteps("Distance", &overrideSettings.Distance, 100, 400, 10, "%.2f");
		SliderFloatWithSteps("Height", &overrideSettings.Height, 40, 200, 10, "%.2f");
		SliderFloatWithSteps("Angle", &overrideSettings.Pitch, -15, 0, 1, "%.2f");
		SliderFloatWithSteps("Stiffness", &overrideSettings.Stiffness, 0, 1, 0.05, "%.2f");
		SliderFloatWithSteps("SwivelSpeed", &overrideSettings.SwivelSpeed, 1, 10, 0.1, "%.2f");
		SliderFloatWithSteps("TransitionSpeed", &overrideSettings.TransitionSpeed, 1, 2, 0.1, "%.2f");

		if (ImGui::Button("Read current settings"))
		{
			ReadPlayerCameraSettings();
		}

		if (settings.enabled)
		{
			OverridePlayerCameraSettings();
		}

		ImGui::EndChild();
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();

	if (ImGui::Button("Save settings"))
	{
		SaveSettings();
	}

	ImGui::End();
}

bool DollyCamPlugin::SidebarTransition(float actualWidth)
{
	actualWidth -= 15;
	float minAlpha = 0.5;
	auto& sidebarSetting = guiState.sidebarSettings;
	if (!sidebarSetting.mouseTransition) {
		sidebarSetting.posOffset = 0;
		sidebarSetting.alpha = 1;
		return true;
	}
	auto mPosX = ImGui::GetMousePos().x;
	float speed = sidebarSetting.transitionSpeed;
	//Hide sidebar
	if (mPosX > sidebarSetting.triggerWidth && !ImGui::IsMouseDragging(0)) {
		if (sidebarSetting.alpha > minAlpha) {
			sidebarSetting.alpha -= 0.02 * speed;
			sidebarSetting.alpha = max(minAlpha, sidebarSetting.alpha);
		}
		if (sidebarSetting.posOffset < actualWidth)
			sidebarSetting.posOffset += 3 * speed;
	}
	//Show sidebar
	if (mPosX < 250) {
		if (sidebarSetting.alpha < 1) {
			sidebarSetting.alpha += 0.02 * speed;
			sidebarSetting.alpha = min(1.0f, sidebarSetting.alpha);
		}
		if (sidebarSetting.posOffset > 0) {
			sidebarSetting.posOffset -= 3 * speed;
			sidebarSetting.posOffset = max(0.0f, sidebarSetting.posOffset);
		}
	}

	return sidebarSetting.posOffset <= actualWidth;
}

void DollyCamPlugin::DrawSnapshotsNodes()
{
	ImGui::Checkbox("Lock Camera", &guiState.camLock);
	ImGui::SameLine(ImGui::GetWindowWidth() - 25);
	if (ImGui::Button(ICON_FA_COG)) {
		guiState.showSettings = !guiState.showSettings;
	}
	ImGui::BeginChild("#", ImVec2(0, -25));
	auto& sidebarSettings = guiState.sidebarSettings;
	static float subScale = 0.8;
	static int newFrame = -1;
	int i = 1;
	guiState.dollySettings.openFrames.clear();
	for (const auto& data : *dollyCam->GetCurrentPath())
	{
		BeginAltBg(i);
		auto snap = data.second;;
		auto label = std::to_string(data.second.frame) + "##" + std::to_string(i);
		//ImGui::AlignTextToFramePadding();
		if (newFrame == snap.frame) {
			ImGui::SetNextTreeNodeOpen(true);
			newFrame = -1;
		}
		bool open = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap);
		if (open)
		{
			guiState.dollySettings.openFrames.push_back(snap.frame);
		}


		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0, 0.0));
		// View toggle
		bool active = (i == view_index);
		bool pressed;
		bool storeOriginal = false;
		ImGui::SameLine(ImGui::GetWindowWidth() - 70);
		if (active) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0, 1));
			pressed = ImGui::SmallButton((ICON_FA_EYE_SLASH "##view" + to_string(i)).c_str());
			ImGui::PopStyleColor();
		}
		else {
			pressed = ImGui::SmallButton((ICON_FA_EYE "##view" + to_string(i)).c_str());
		}
		if (pressed && active) { //If this one was already active. clear out the group and unlock the camera
			view_index = -1;
			dollyCam->gameWrapper->Execute([this](GameWrapper* gw) {
				auto camera = gw->GetCamera();
				camera.SetPOV(guiState.sidebarSettings.originalView);
				camera.SetLockedFOV(false);
				});
		}
		if (pressed && !active) {
			if (view_index == -1) storeOriginal = true;
			view_index = i;
		}
		if (i == view_index)
		{
			POV pov = { snap.location, snap.rotation.ToRotator(), snap.FOV };
			dollyCam->gameWrapper->Execute([this, pov, storeOriginal](GameWrapper* gw) {
				auto camera = gw->GetCamera();
				if (storeOriginal) {
					guiState.sidebarSettings.originalView = camera.GetPOV();
				}
				camera.SetPOV(pov);
				camera.SetFocusActor("");
				});
		}

		//Delete button
		ImGui::SameLine(ImGui::GetWindowWidth() - 45);
		std::string removeButtonLabel = ICON_FA_TRASH "##remove" + to_string(i);
		if (ImGui::SmallButton(removeButtonLabel.c_str()))
		{
			dollyCam->gameWrapper->Execute([this, i](GameWrapper* gw) {dollyCam->DeleteFrameByIndex(i); });
		}
		ImGui::PopStyleColor();
		//Make subwidgets a little smaller
		ImGui::SetWindowFontScale(subScale);
		if (open)
		{
			static chrono::system_clock::time_point lastUpdate = chrono::system_clock::now();

			bool doUpdateFrame = false;
			int updateLimit = 100.0f / guiState.sidebarSettings.editTimeLimit ;
			auto lspeed = sidebarSettings.LocationSpeed;
			auto lpower = sidebarSettings.LocationSpeed;
			auto rSpeed = sidebarSettings.RotationSpeed;
			auto rpower = sidebarSettings.RotationPower;

			int oldFrame = snap.frame;
			
			if (ImGui::InputInt(("Frame##frame" + to_string(i)).c_str(), &snap.frame, 5, 25, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				newFrame = snap.frame;
				int newFrame_ = snap.frame;
				int df = (newFrame > oldFrame) ? 1: -1;
				CameraSnapshot snap =  dollyCam->GetSnapshot(newFrame);
				int i = 0;
				while (snap.frame != -1 && i++ < 100)
				{
					newFrame += df;
					newFrame_ += df;
					cvarManager->log("trying new frame: " + std::to_string(newFrame));
					snap = dollyCam->GetSnapshot(newFrame);
				}
				dollyCam->gameWrapper->Execute([this, oldFrame, newFrame_](GameWrapper* gw) {dollyCam->ChangeFrame(oldFrame, newFrame_); });
			}

			//if (ImGui::SliderInt(("Frame##frame" + to_string(i)).c_str(), &snap.frame, snap.frame - 100, snap.frame + 100))
			//{
			//	newFrame = snap.frame;
			//}
			//if (IsItemJustMadeActive())
			//{
			//	cvarManager->log(std::to_string(oldFrame));
			//	oldFrame = snap.frame;
			//}
			//if (IsItemJustMadeInactive())
			//{
			//	cvarManager->log("old frame:" + std::to_string(oldFrame));
			//	cvarManager->log("new frame:" + std::to_string(newFrame));
			//	cvarManager->log("just inactive. editing the frame");
			//	int oldFrame_ = oldFrame;
			//	int newFrame_ = newFrame;
			//	dollyCam->gameWrapper->Execute([this, newFrame_, oldFrame_](GameWrapper* gw) {dollyCam->ChangeFrame(oldFrame_, newFrame_); });
			//}

			if (!sidebarSettings.compact)
			{
				ImGui::Columns(2, 0, false);

				ImGui::Text("Location");
				if (ImGui::DragFloat(("X##x" + to_string(i)).c_str(), &snap.location.X, lspeed, 0.f, 0.f, "%.0f", lpower)) { doUpdateFrame = true; }DragInPlace();
				if (ImGui::DragFloat(("Y##y" + to_string(i)).c_str(), &snap.location.Y, lspeed, 0.f, 0.f, "%.0f", lpower)) { doUpdateFrame = true; }DragInPlace();
				if (ImGui::DragFloat(("Z##z" + to_string(i)).c_str(), &snap.location.Z, lspeed, 0.f, 0.f, "%.0f", lpower)) { doUpdateFrame = true; }DragInPlace();
				ImGui::NextColumn();

				ImGui::Text("Rotation");
				if (ImGui::DragFloat((ICON_FA_ARROWS_ALT_V "##" + to_string(i)).c_str(), &snap.rotation.Pitch._value, rSpeed, snap.rotation.Pitch._min, snap.rotation.Pitch._max, "%.0f", rpower)) { doUpdateFrame = true; }DragInPlace();
				if (ImGui::DragFloat((ICON_FA_ARROWS_ALT_H "##" + to_string(i)).c_str(), &snap.rotation.Yaw._value, rSpeed, snap.rotation.Yaw._min, snap.rotation.Yaw._max, "%.0f", rpower)) { doUpdateFrame = true; }DragInPlace();
				if (ImGui::DragFloat((ICON_FA_SYNC "##" + to_string(i)).c_str(), &snap.rotation.Roll._value, rSpeed, snap.rotation.Roll._min, snap.rotation.Roll._max, "%.0f", rpower)) { doUpdateFrame = true; }DragInPlace();
				ImGui::NextColumn();

				ImGui::Columns(1, 0, false);

				if (ImGui::SliderFloat(("FOV##FOV" + to_string(i)).c_str(), &snap.FOV, 60, 110, "%.0f")) { doUpdateFrame = true; };
			}
			else {
				ImGui::PushItemWidth(30);
				if (ImGui::DragFloat(("##x" + to_string(i)).c_str(), &snap.location.X, lspeed, 0.f, 0.f, "X", lpower)) { doUpdateFrame = true; }ImGui::SameLine(); DragInPlace();
				if (ImGui::DragFloat(("##Pitch" + to_string(i)).c_str(), &snap.rotation.Pitch._value, rSpeed, snap.rotation.Pitch._min, snap.rotation.Pitch._max, ICON_FA_ARROWS_ALT_V, rpower)) { doUpdateFrame = true; } DragInPlace();

				if (ImGui::DragFloat(("##y" + to_string(i)).c_str(), &snap.location.Y, lspeed, 0.f, 0.f, "Y", lpower)) { doUpdateFrame = true; }ImGui::SameLine(); DragInPlace();
				if (ImGui::DragFloat(("##Yaw" + to_string(i)).c_str(), &snap.rotation.Yaw._value, rSpeed, snap.rotation.Yaw._min, snap.rotation.Yaw._max, ICON_FA_ARROWS_ALT_H, rpower)) { doUpdateFrame = true; } DragInPlace();

				if (ImGui::DragFloat(("##z" + to_string(i)).c_str(), &snap.location.Z, lspeed, 0.f, 0.f, "Z", lpower)) { doUpdateFrame = true; }ImGui::SameLine(); DragInPlace();
				if (ImGui::DragFloat(("##Roll" + to_string(i)).c_str(), &snap.rotation.Roll._value, rSpeed, snap.rotation.Roll._min, snap.rotation.Roll._max, ICON_FA_SYNC, rpower)) { doUpdateFrame = true; } DragInPlace();

				ImGui::PopItemWidth();
				if (ImGui::SliderFloat(("##FOV" + to_string(i)).c_str(), &snap.FOV, 60, 110, "FOV: %.0f")) { doUpdateFrame = true; }
			}
			if (doUpdateFrame && chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > updateLimit)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([this, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
			ImGui::Separator();

			ImGui::TreePop();
		}
		ImGui::SetWindowFontScale(1);
		EndAltBg(i);
		i++;
	}

	ImGui::EndChild();
}

void DollyCamPlugin::DrawSaveLoadSettings()
{
	if (ImGui::Button("Clear path")) {
		dollyCam->Reset();
		SetStatusTimeout(gameWrapper, "Path cleared");
	}
	ImGui::SameLine();
	static char filename[256];

	ImGui::PushItemWidth(150);
	ImGui::InputText("Filename", filename, IM_ARRAYSIZE(filename));
	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (strlen(filename) == 0)
	{
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}
	if (ImGui::Button("Save")) {
		auto res = dollyCam->SaveToFile(filename);
		if (res) {
			SetStatusTimeout(gameWrapper, "File saved");
		}
		else {
			SetStatusTimeout(gameWrapper, "Failed to save file!");
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Load")) {
		auto res = dollyCam->LoadFromFile(filename);
		if (res) {
			SetStatusTimeout(gameWrapper, "File loaded");
		}
		else {
			SetStatusTimeout(gameWrapper, "Failed to load file");
		}
	}

	if (strlen(filename) == 0)
	{
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}
	if (!saveloadStatus.empty())
	{
		ImGui::Text(saveloadStatus.c_str());
	}
}

void DollyCamPlugin::DrawInterpolationSettings()
{
	static int interpolation_location = cvarManager->getCvar("dolly_interpmode_location").getIntValue();
	static int interpolation_rotation = cvarManager->getCvar("dolly_interpmode_rotation").getIntValue();
	static std::vector<std::string> interpolationMethods = { "Linear" , "Bezier", "Cosine", "Linear", "Catmull", "Spline" };
	//static int currentInterpolation = dollyCam->GetInterpolationMethod()

	// Draw location interpolation selector
	ImGui::DragFloat("Anti jitter", &dollyCam->anti_jitter_factor, 0.00005f, 0.9, 1.1);

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
	auto& style = ImGui::GetStyle();
	if (gameWrapper != nullptr)
	{
		auto bm_style_ptr = gameWrapper->GetGUIManager().GetImGuiStyle();
		if (bm_style_ptr != nullptr)
		{
			style = *(ImGuiStyle*)bm_style_ptr;
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
	return;
	//static bool showFirstSnap = false;
	//static bool showPathDuration = true;
	//auto replayServer = gameWrapper->GetGameEventAsReplay();
	//ImGui::BeginChild("timelinechild", { 0, 30 });
	//static float totalReplayTime = 100.0f;
	//if (!replayServer.IsNull())
	//{
	//	auto replay = replayServer.GetReplay();
	//	totalReplayTime = replay.GetNumFrames() / replayServer.GetReplayFPS();
	//}
	//else {
	//}
	//ImGui::BeginTimeline("Timeline", totalReplayTime);

	//static float currentTime = 0;
	//static float seekTime = 0.0f;
	//auto frames = dollyCam->GetUsedFrames();

	//ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 237.0 / 255, 25.0 / 255, 0, 1 });
	//ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 1.0, 72.0 / 255, 50.0 / 255, 1 });
	//ImGui::PushStyleColor(ImGuiCol_ColumnActive, ImVec4{ 1, 87.0 / 255, 67.0 / 255, 1 });
	//if (ImGui::TimelineMarker("currentFrame", currentTime, "Time: "))
	//{
	//	seekTime = currentTime;
	//}
	//else {
	//	if (!replayServer.IsNull())
	//	{
	//		auto replay = replayServer.GetReplay();
	//		currentTime = replay.GetCurrentFrame() / replayServer.GetReplayFPS();
	//	}
	//}
	//ImGui::PopStyleColor(3);
	//if (IsItemJustMadeInactive())
	//{
	//	int _seekTime = seekTime; //lambda can't capture static
	//	gameWrapper->Execute([_seekTime, this](GameWrapper* gw) {
	//		auto replayServer = gameWrapper->GetGameEventAsReplay();
	//		int frame = _seekTime * replayServer.GetReplayFPS();
	//		replayServer.SkipToFrame(frame);
	//		//cvarManager->log("got this frame:" + to_string(gw->GetGameEventAsReplay().GetCurrentReplayFrame()));
	//		});
	//	//cvarManager->log("Seek to:" + to_string(seekFrame));
	//	//frameSkip = seekFrame;
	//}

	//if (showFirstSnap)
	//{
	//	if (frames.size() > 0)
	//	{
	//		auto firstFrame = (float)frames.front();
	//		ImGui::TimelineMarker("FirstFrame", firstFrame, "First frame");
	//	}
	//}

	//if (showPathDuration)
	//{
	//	if (frames.size() > 1)
	//	{
	//		auto firstFrame = (float)frames.front();
	//		auto lastFrame = (float)frames.back();
	//		float values[2] = { firstFrame, lastFrame };
	//		ImGui::TimelineEvent("", values);
	//	}
	//}
	//ImGui::EndTimeline();
	//ImGui::EndChild();
}

void DollyCamPlugin::ReadPlayerCameraSettings()
{
	gameWrapper->Execute([this](GameWrapper* gw) {
		auto cam = gw->GetCamera();
		if (!cam.IsNull())
		{
			guiState.cameraOverride.cameraSettings = cam.GetCameraSettings();
		}
		});
}

void DollyCamPlugin::OverridePlayerCameraSettings()
{
	gameWrapper->Execute([this](GameWrapper* gw) {
		auto cam = gw->GetCamera();
		if (!cam.IsNull())
		{
			cam.SetCameraSettings(guiState.cameraOverride.cameraSettings);
		}
		});
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
	cvarManager->log("Getting the imgui context!");
	gameWrapper->Execute([this, ctx](GameWrapper* gw) {
		ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
		auto io = ImGui::GetIO();
		//io.Fonts->AddFontDefault();
		auto default = io.Fonts->AddFontFromFileTTF("bakkesmod/font.ttf", 13.0f);
		ImFontConfig config;
		config.MergeMode = true;
		static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		fa = io.Fonts->AddFontFromFileTTF("bakkesmod/data/fonts/fa-solid-900.ttf", 13.0f, &config, icon_ranges);
		});
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
	//guiState.sidebarSettings.alpha = 1.0f;
	//guiState.sidebarSettings.posOffset = 0;
}

void DollyCamPlugin::OnClose()
{
	if (guiState.showSettings)
	{
		SaveSettings();
		guiState.showSettings = false;
		gameWrapper->Execute([this](GameWrapper*) {cvarManager->executeCommand("togglemenu " + GetMenuName()); });
	}
	else {
		isWindowOpen = false;
	}
}