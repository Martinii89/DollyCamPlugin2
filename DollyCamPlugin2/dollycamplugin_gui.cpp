#include "dollycamplugin.h"
#include "imgui\imgui.h"
#include "imgui\imgui_internal.h"
#include "imgui/imgui_timeline.h"
#include "serialization.h"
#include "bakkesmod\..\\utils\parser.h"
#include <functional>
#include <vector>
#include "bakkesmod/wrappers/GuiManagerWrapper.h"

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
		const char* widgetFormat = "%.0f";
		ImGui::PushItemWidth(60);
		ImGui::SameLine();
		if (ImGui::DragFloat(("##x" + to_string(i)).c_str(), &snap.location.X, 1.f, 0.f, 0.f, widgetFormat))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 100)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::SameLine();
		if (ImGui::DragFloat(("##y" + to_string(i)).c_str(), &snap.location.Y, 1.f, 0.f, 0.f, widgetFormat))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 100)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::SameLine();
		if (ImGui::DragFloat(("##z" + to_string(i)).c_str(), &snap.location.Z, 1.f, 0.f, 0.f, widgetFormat))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 100)
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
		ImGui::PushItemWidth(60);
		ImGui::SameLine();
		if (ImGui::DragFloat(("##Pitch" + to_string(i)).c_str(), &snap.rotation.Pitch._value, 1.f, snap.rotation.Pitch._min, snap.rotation.Pitch._max, widgetFormat))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 100)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::SameLine();
		if (ImGui::DragFloat(("##Yaw" + to_string(i)).c_str(), &snap.rotation.Yaw._value, 1.f, snap.rotation.Yaw._min, snap.rotation.Yaw._max, widgetFormat))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 100)
			{
				lastUpdate = chrono::system_clock::now();
				dollyCam->gameWrapper->Execute([dollyCam, snap](GameWrapper* gw) {dollyCam->UpdateFrame(snap); });
			}
		}
		ImGui::SameLine();
		if (ImGui::DragFloat(("##Roll" + to_string(i)).c_str(), &snap.rotation.Roll._value, 1.f, snap.rotation.Roll._min, snap.rotation.Roll._max, widgetFormat))
		{
			if (chrono::duration_cast<std::chrono::milliseconds> (chrono::system_clock::now() - lastUpdate).count() > 100)
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
		{"Frame",		40,		true, false, [](CameraSnapshot snap, int i) {return to_string(snap.frame); },						noWidget},
		//{"Time",		60,		true, false, [](CameraSnapshot snap, int i) {return to_string_with_precision(snap.timeStamp, 2); }, noWidget},
		{"Location",	200,	true, true, [](CameraSnapshot snap, int i) {return vector_to_string(snap.location); },				locationWidget},
		{"Rotation",	200,	true, true, [](CameraSnapshot snap, int i) {return rotator_to_string(snap.rotation.ToRotator()); },rotationWidget},
		//{"FOV",			40,		true, false, [](CameraSnapshot snap, int i) {return to_string_with_precision(snap.FOV, 1); },		noWidget},
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
	auto columns = Columns::column;
	int totalWidth = std::accumulate(columns.begin(), columns.end(), 0, [](int sum, const Columns::TableColumns& element) {return sum + element.GetWidth(); });
	//ImGui::SetNextWindowSizeConstraints(ImVec2(totalWidth, 300), ImVec2(FLT_MAX, FLT_MAX));

	//setting bg alpha to 0.75
	//auto context = ImGui::GetCurrentContext();
	//const ImGuiCol bg_color_idx = ImGuiCol_WindowBg;
	//context->Style.Colors[bg_color_idx].w = 0.75;

	string menuName = "Snapshots";
	if (!ImGui::Begin(menuName.c_str(), &isWindowOpen, ImGuiWindowFlags_ResizeFromAnySide))
	{
		// Early out if the window is collapsed, as an optimization.
		block_input = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
		ImGui::End();
		return;
	}

	// Make style consistent with BM
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

	int enabledCount = std::accumulate(columns.begin(), columns.end(), 0, [](int sum, const Columns::TableColumns& element) {return sum + element.enabled; });
	ImGui::BeginChild("#CurrentSnapshotsTab", ImVec2(-5, 0));
	//ImGui::BeginChild("#CurrentSnapshotsTab", ImVec2(totalWidth, -ImGui::GetFrameHeightWithSpacing()));
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
	ImGui::BeginChild("", ImVec2(0, -100));
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
	static bool lockCam = false;
	ImGui::Checkbox("Lock camera", &lockCam);
	dollyCam->lockCamera = ImGui::IsMouseDragging() || lockCam;

	static int CinderSpinnervalue = 50;
	ImGui::DragInt("CinderSpinner", &CinderSpinnervalue, 1.0f, 0, 100);

	auto replayServer = gameWrapper->GetGameEventAsReplay();
	if (!replayServer.IsNull())
	{
		DrawTimeline(replayServer);
	}

	ImGui::EndChild();
	ImGui::End();

	if (!isWindowOpen)
	{
		cvarManager->executeCommand("togglemenu " + GetMenuName());
	}
	block_input = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
}

void DollyCamPlugin::DrawTimeline(ReplayServerWrapper& replayServer)
{
	ImGui::BeginChild("timelinechild", { 0, 35 });
	auto replay = replayServer.GetReplay();
	auto totalReplayTime = replay.GetNumFrames();
	ImGui::BeginTimeline("Timeline", totalReplayTime);

	static float currentFrame = 0;
	static float seekFrame = 0.0f;

	auto frames = dollyCam->GetUsedFrames();
	if (frames.size() > 0)
	{
		auto firstFrame = (float)frames[0];
		if (ImGui::TimelineMarker("FirstFrame", firstFrame, "First frame"));
	}

	if (ImGui::TimelineMarker("currentFrame", currentFrame, "Current frame"))
	{
		seekFrame = currentFrame;
	}
	else {
		currentFrame = replay.GetCurrentFrame();
	}
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