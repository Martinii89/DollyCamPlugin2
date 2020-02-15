#include "dollycamplugin.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_timeline.h"
#include "imgui/imgui_tabs.h"
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
		g.DragLastMouseDelta.x = -ImGui::GetMouseDragDelta(0, 1).x;
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

#ifdef _WIN32
#define IM_NEWLINE "\r\n"
#else
#define IM_NEWLINE "\n"
#endif

void ImGui::ShowFontSelector(const char* label)
{
	ImGuiIO& io = ImGui::GetIO();
	ImFont* font_current = ImGui::GetFont();
	if (ImGui::BeginCombo(label, font_current->GetDebugName()))
	{
		for (int n = 0; n < io.Fonts->Fonts.Size; n++)
			if (ImGui::Selectable(io.Fonts->Fonts[n]->GetDebugName(), io.Fonts->Fonts[n] == font_current))
				io.FontDefault = io.Fonts->Fonts[n];
		ImGui::EndCombo();
	}
	ImGui::SameLine();
	ShowHelpMarker(
		"- Load additional fonts with io.Fonts->AddFontFromFileTTF().\n"
		"- The font atlas is built when calling io.Fonts->GetTexDataAsXXXX() or io.Fonts->Build().\n"
		"- Read FAQ and documentation in extra_fonts/ for more details.\n"
		"- If you need to add/remove fonts at runtime (e.g. for DPI change), do it before calling NewFrame().");
}

bool ImGui::ShowStyleSelector(const char* label)
{
	static int style_idx = 0;
	if (ImGui::Combo(label, &style_idx, "Classic\0Dark\0Light\0"))
	{
		switch (style_idx)
		{
		case 0: ImGui::StyleColorsClassic(); break;
		case 1: ImGui::StyleColorsDark(); break;
		case 2: ImGui::StyleColorsLight(); break;
		}
		return true;
	}
	return false;
}

void ImGui::ShowStyleEditor(ImGuiStyle* ref)
{
	// You can pass in a reference ImGuiStyle structure to compare to, revert to and save to (else it compares to an internally stored reference)
	ImGuiStyle& style = ImGui::GetStyle();
	static ImGuiStyle ref_saved_style;

	// Default to using internal storage as reference
	static bool init = true;
	if (init && ref == NULL)
		ref_saved_style = style;
	init = false;
	if (ref == NULL)
		ref = &ref_saved_style;

	ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.50f);

	if (ImGui::ShowStyleSelector("Colors##Selector"))
		ref_saved_style = style;
	ImGui::ShowFontSelector("Fonts##Selector");

	// Simplified Settings
	if (ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f"))
		style.GrabRounding = style.FrameRounding; // Make GrabRounding always the same value as FrameRounding
	{ bool window_border = (style.WindowBorderSize > 0.0f); if (ImGui::Checkbox("WindowBorder", &window_border)) style.WindowBorderSize = window_border ? 1.0f : 0.0f; }
	ImGui::SameLine();
	{ bool frame_border = (style.FrameBorderSize > 0.0f); if (ImGui::Checkbox("FrameBorder", &frame_border)) style.FrameBorderSize = frame_border ? 1.0f : 0.0f; }
	ImGui::SameLine();
	{ bool popup_border = (style.PopupBorderSize > 0.0f); if (ImGui::Checkbox("PopupBorder", &popup_border)) style.PopupBorderSize = popup_border ? 1.0f : 0.0f; }

	// Save/Revert button
	if (ImGui::Button("Save Ref"))
		*ref = ref_saved_style = style;
	ImGui::SameLine();
	if (ImGui::Button("Revert Ref"))
		style = *ref;
	ImGui::SameLine();
	ShowHelpMarker("Save/Revert in local non-persistent storage. Default Colors definition are not affected. Use \"Export Colors\" below to save them somewhere.");

	if (ImGui::TreeNode("Rendering"))
	{
		ImGui::Checkbox("Anti-aliased lines", &style.AntiAliasedLines); ImGui::SameLine(); ShowHelpMarker("When disabling anti-aliasing lines, you'll probably want to disable borders in your style as well.");
		ImGui::Checkbox("Anti-aliased fill", &style.AntiAliasedFill);
		ImGui::PushItemWidth(100);
		ImGui::DragFloat("Curve Tessellation Tolerance", &style.CurveTessellationTol, 0.02f, 0.10f, FLT_MAX, NULL, 2.0f);
		if (style.CurveTessellationTol < 0.0f) style.CurveTessellationTol = 0.10f;
		ImGui::DragFloat("Global Alpha", &style.Alpha, 0.005f, 0.20f, 1.0f, "%.2f"); // Not exposing zero here so user doesn't "lose" the UI (zero alpha clips all widgets). But application code could have a toggle to switch between zero and non-zero.
		ImGui::PopItemWidth();
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Settings"))
	{
		ImGui::SliderFloat2("WindowPadding", (float*)&style.WindowPadding, 0.0f, 20.0f, "%.0f");
		ImGui::SliderFloat("PopupRounding", &style.PopupRounding, 0.0f, 16.0f, "%.0f");
		ImGui::SliderFloat2("FramePadding", (float*)&style.FramePadding, 0.0f, 20.0f, "%.0f");
		ImGui::SliderFloat2("ItemSpacing", (float*)&style.ItemSpacing, 0.0f, 20.0f, "%.0f");
		ImGui::SliderFloat2("ItemInnerSpacing", (float*)&style.ItemInnerSpacing, 0.0f, 20.0f, "%.0f");
		ImGui::SliderFloat2("TouchExtraPadding", (float*)&style.TouchExtraPadding, 0.0f, 10.0f, "%.0f");
		ImGui::SliderFloat("IndentSpacing", &style.IndentSpacing, 0.0f, 30.0f, "%.0f");
		ImGui::SliderFloat("ScrollbarSize", &style.ScrollbarSize, 1.0f, 20.0f, "%.0f");
		ImGui::SliderFloat("GrabMinSize", &style.GrabMinSize, 1.0f, 20.0f, "%.0f");
		ImGui::Text("BorderSize");
		ImGui::SliderFloat("WindowBorderSize", &style.WindowBorderSize, 0.0f, 1.0f, "%.0f");
		ImGui::SliderFloat("ChildBorderSize", &style.ChildBorderSize, 0.0f, 1.0f, "%.0f");
		ImGui::SliderFloat("PopupBorderSize", &style.PopupBorderSize, 0.0f, 1.0f, "%.0f");
		ImGui::SliderFloat("FrameBorderSize", &style.FrameBorderSize, 0.0f, 1.0f, "%.0f");
		ImGui::Text("Rounding");
		ImGui::SliderFloat("WindowRounding", &style.WindowRounding, 0.0f, 14.0f, "%.0f");
		ImGui::SliderFloat("ChildRounding", &style.ChildRounding, 0.0f, 16.0f, "%.0f");
		ImGui::SliderFloat("FrameRounding", &style.FrameRounding, 0.0f, 12.0f, "%.0f");
		ImGui::SliderFloat("ScrollbarRounding", &style.ScrollbarRounding, 0.0f, 12.0f, "%.0f");
		ImGui::SliderFloat("GrabRounding", &style.GrabRounding, 0.0f, 12.0f, "%.0f");
		ImGui::Text("Alignment");
		ImGui::SliderFloat2("WindowTitleAlign", (float*)&style.WindowTitleAlign, 0.0f, 1.0f, "%.2f");
		ImGui::SliderFloat2("ButtonTextAlign", (float*)&style.ButtonTextAlign, 0.0f, 1.0f, "%.2f"); ImGui::SameLine(); ShowHelpMarker("Alignment applies when a button is larger than its text content.");
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Colors"))
	{
		static int output_dest = 0;
		static bool output_only_modified = true;
		if (ImGui::Button("Export Unsaved"))
		{
			if (output_dest == 0)
				ImGui::LogToClipboard();
			else
				ImGui::LogToTTY();
			ImGui::LogText("ImVec4* colors = ImGui::GetStyle().Colors;" IM_NEWLINE);
			for (int i = 0; i < ImGuiCol_COUNT; i++)
			{
				const ImVec4& col = style.Colors[i];
				const char* name = ImGui::GetStyleColorName(i);
				if (!output_only_modified || memcmp(&col, &ref->Colors[i], sizeof(ImVec4)) != 0)
					ImGui::LogText("colors[ImGuiCol_%s]%*s= ImVec4(%.2ff, %.2ff, %.2ff, %.2ff);" IM_NEWLINE, name, 23 - (int)strlen(name), "", col.x, col.y, col.z, col.w);
			}
			ImGui::LogFinish();
		}
		ImGui::SameLine(); ImGui::PushItemWidth(120); ImGui::Combo("##output_type", &output_dest, "To Clipboard\0To TTY\0"); ImGui::PopItemWidth();
		ImGui::SameLine(); ImGui::Checkbox("Only Modified Colors", &output_only_modified);

		ImGui::Text("Tip: Left-click on colored square to open color picker,\nRight-click to open edit options menu.");

		static ImGuiTextFilter filter;
		filter.Draw("Filter colors", 200);

		static ImGuiColorEditFlags alpha_flags = 0;
		ImGui::RadioButton("Opaque", &alpha_flags, 0); ImGui::SameLine();
		ImGui::RadioButton("Alpha", &alpha_flags, ImGuiColorEditFlags_AlphaPreview); ImGui::SameLine();
		ImGui::RadioButton("Both", &alpha_flags, ImGuiColorEditFlags_AlphaPreviewHalf);

		ImGui::BeginChild("#colors", ImVec2(0, 300), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
		ImGui::PushItemWidth(-160);
		for (int i = 0; i < ImGuiCol_COUNT; i++)
		{
			const char* name = ImGui::GetStyleColorName(i);
			if (!filter.PassFilter(name))
				continue;
			ImGui::PushID(i);
			ImGui::ColorEdit4("##color", (float*)&style.Colors[i], ImGuiColorEditFlags_AlphaBar | alpha_flags);
			if (memcmp(&style.Colors[i], &ref->Colors[i], sizeof(ImVec4)) != 0)
			{
				// Tips: in a real user application, you may want to merge and use an icon font into the main font, so instead of "Save"/"Revert" you'd use icons.
				// Read the FAQ and extra_fonts/README.txt about using icon fonts. It's really easy and super convenient!
				ImGui::SameLine(0.0f, style.ItemInnerSpacing.x); if (ImGui::Button("Save")) ref->Colors[i] = style.Colors[i];
				ImGui::SameLine(0.0f, style.ItemInnerSpacing.x); if (ImGui::Button("Revert")) style.Colors[i] = ref->Colors[i];
			}
			ImGui::SameLine(0.0f, style.ItemInnerSpacing.x);
			ImGui::TextUnformatted(name);
			ImGui::PopID();
		}
		ImGui::PopItemWidth();
		ImGui::EndChild();

		ImGui::TreePop();
	}

	bool fonts_opened = ImGui::TreeNode("Fonts", "Fonts (%d)", ImGui::GetIO().Fonts->Fonts.Size);
	if (fonts_opened)
	{
		ImFontAtlas* atlas = ImGui::GetIO().Fonts;
		if (ImGui::TreeNode("Atlas texture", "Atlas texture (%dx%d pixels)", atlas->TexWidth, atlas->TexHeight))
		{
			ImGui::Image(atlas->TexID, ImVec2((float)atlas->TexWidth, (float)atlas->TexHeight), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255), ImColor(255, 255, 255, 128));
			ImGui::TreePop();
		}
		ImGui::PushItemWidth(100);
		for (int i = 0; i < atlas->Fonts.Size; i++)
		{
			ImFont* font = atlas->Fonts[i];
			ImGui::PushID(font);
			bool font_details_opened = ImGui::TreeNode(font, "Font %d: \'%s\', %.2f px, %d glyphs", i, font->ConfigData ? font->ConfigData[0].Name : "", font->FontSize, font->Glyphs.Size);
			ImGui::SameLine(); if (ImGui::SmallButton("Set as default")) ImGui::GetIO().FontDefault = font;
			if (font_details_opened)
			{
				ImGui::PushFont(font);
				ImGui::Text("The quick brown fox jumps over the lazy dog");
				ImGui::PopFont();
				ImGui::DragFloat("Font scale", &font->Scale, 0.005f, 0.3f, 2.0f, "%.1f");   // Scale only this font
				ImGui::SameLine(); ShowHelpMarker("Note than the default embedded font is NOT meant to be scaled.\n\nFont are currently rendered into bitmaps at a given size at the time of building the atlas. You may oversample them to get some flexibility with scaling. You can also render at multiple sizes and select which one to use at runtime.\n\n(Glimmer of hope: the atlas system should hopefully be rewritten in the future to make scaling more natural and automatic.)");
				ImGui::Text("Ascent: %f, Descent: %f, Height: %f", font->Ascent, font->Descent, font->Ascent - font->Descent);
				ImGui::Text("Fallback character: '%c' (%d)", font->FallbackChar, font->FallbackChar);
				ImGui::Text("Texture surface: %d pixels (approx) ~ %dx%d", font->MetricsTotalSurface, (int)sqrtf((float)font->MetricsTotalSurface), (int)sqrtf((float)font->MetricsTotalSurface));
				for (int config_i = 0; config_i < font->ConfigDataCount; config_i++)
				{
					ImFontConfig* cfg = &font->ConfigData[config_i];
					ImGui::BulletText("Input %d: \'%s\', Oversample: (%d,%d), PixelSnapH: %d", config_i, cfg->Name, cfg->OversampleH, cfg->OversampleV, cfg->PixelSnapH);
				}
				if (ImGui::TreeNode("Glyphs", "Glyphs (%d)", font->Glyphs.Size))
				{
					// Display all glyphs of the fonts in separate pages of 256 characters
					const ImFontGlyph* glyph_fallback = font->FallbackGlyph; // Forcefully/dodgily make FindGlyph() return NULL on fallback, which isn't the default behavior.
					font->FallbackGlyph = NULL;
					for (int base = 0; base < 0x10000; base += 256)
					{
						int count = 0;
						for (int n = 0; n < 256; n++)
							count += font->FindGlyph((ImWchar)(base + n)) ? 1 : 0;
						if (count > 0 && ImGui::TreeNode((void*)(intptr_t)base, "U+%04X..U+%04X (%d %s)", base, base + 255, count, count > 1 ? "glyphs" : "glyph"))
						{
							float cell_spacing = style.ItemSpacing.y;
							ImVec2 cell_size(font->FontSize * 1, font->FontSize * 1);
							ImVec2 base_pos = ImGui::GetCursorScreenPos();
							ImDrawList* draw_list = ImGui::GetWindowDrawList();
							for (int n = 0; n < 256; n++)
							{
								ImVec2 cell_p1(base_pos.x + (n % 16) * (cell_size.x + cell_spacing), base_pos.y + (n / 16) * (cell_size.y + cell_spacing));
								ImVec2 cell_p2(cell_p1.x + cell_size.x, cell_p1.y + cell_size.y);
								const ImFontGlyph* glyph = font->FindGlyph((ImWchar)(base + n));;
								draw_list->AddRect(cell_p1, cell_p2, glyph ? IM_COL32(255, 255, 255, 100) : IM_COL32(255, 255, 255, 50));
								font->RenderChar(draw_list, cell_size.x, cell_p1, ImGui::GetColorU32(ImGuiCol_Text), (ImWchar)(base + n)); // We use ImFont::RenderChar as a shortcut because we don't have UTF-8 conversion functions available to generate a string.
								if (glyph && ImGui::IsMouseHoveringRect(cell_p1, cell_p2))
								{
									ImGui::BeginTooltip();
									ImGui::Text("Codepoint: U+%04X", base + n);
									ImGui::Separator();
									ImGui::Text("AdvanceX: %.1f", glyph->AdvanceX);
									ImGui::Text("Pos: (%.2f,%.2f)->(%.2f,%.2f)", glyph->X0, glyph->Y0, glyph->X1, glyph->Y1);
									ImGui::Text("UV: (%.3f,%.3f)->(%.3f,%.3f)", glyph->U0, glyph->V0, glyph->U1, glyph->V1);
									ImGui::EndTooltip();
								}
							}
							ImGui::Dummy(ImVec2((cell_size.x + cell_spacing) * 16, (cell_size.y + cell_spacing) * 16));
							ImGui::TreePop();
						}
					}
					font->FallbackGlyph = glyph_fallback;
					ImGui::TreePop();
				}
				ImGui::TreePop();
			}
			ImGui::PopID();
		}
		static float window_scale = 1.0f;
		ImGui::DragFloat("this window scale", &window_scale, 0.005f, 0.3f, 2.0f, "%.1f");              // scale only this window
		ImGui::DragFloat("global scale", &ImGui::GetIO().FontGlobalScale, 0.005f, 0.3f, 2.0f, "%.1f"); // scale everything
		ImGui::PopItemWidth();
		ImGui::SetWindowFontScale(window_scale);
		ImGui::TreePop();
	}

	ImGui::PopItemWidth();
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
	if (fa != nullptr && fa->IsLoaded())
		ImGui::PushFont(fa);
	// Make style consistent with BM
	SetStyle();
	//int totalWidth = std::accumulate(columns.begin(), columns.end(), 0, [](int sum, const Columns::TableColumns& element) {return sum + element.GetWidth(); });

	auto& sidebarSetting = guiState.sidebarSettings;
	float actualWidth = sidebarSetting.width + (sidebarSetting.compact ? 0.0f : 50.0f);
	/*SidebarTransition(actualWidth);*/

	ImGui::SetNextWindowPos({ 0 - (sidebarSetting.posOffset), 0 }, ImGuiCond_Always);
	ImGui::SetNextWindowSize({ actualWidth, sidebarSetting.height }, ImGuiCond_Always);
	if (ImGui::Begin("##sidebar", &isWindowOpen, { 0, 0 }, sidebarSetting.alpha, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar)) {
		if (SidebarTransition(actualWidth))
		{
			DrawSnapshotsNodes();
			if (ImGui::Button(ICON_FA_BACKWARD))
			{
				gameWrapper->Execute([this](GameWrapper* gw) {
					auto firstFrame = std::max(0, dollyCam->GetUsedFrames().front() - 30); //add some padding
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
	dollyCam->lockCamera = ImGui::IsMouseDragging() || guiState.camLock || view_index > 0;
	block_input = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
	if (fa != nullptr && fa->IsLoaded())
		ImGui::PopFont();
}

void DollyCamPlugin::DrawSettingsWindow()
{
	string menuName = "Settings";
	ImGui::SetNextWindowPos({ 600,0 }, ImGuiCond_Once);
	ImGui::SetNextWindowSize({ 600,500 }, ImGuiCond_Once);
	if (!ImGui::Begin(menuName.c_str(), &guiState.showSettings, ImGuiWindowFlags_ResizeFromAnySide))
	{
		// Early out if the window is collapsed, as an optimization.
		block_input = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
		ImGui::End();
		return;
	}

	auto& tabSettings = guiState.tabsSettings;
	ImGui::BeginTabBar("#");
	ImGui::DrawTabsBackground();
	if (ImGui::AddTab("Settings"))
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
	}

	if (tabSettings.oldSnapshots && ImGui::AddTab("Keyframes"))
	{
		ImGui::BeginChild("#", ImVec2(-5, -50));
		DrawSnapshots();

		// Draw checkbox for locking the camera
		ImGui::Checkbox("Lock camera", &guiState.camLock);;

		//static int CinderSpinnervalue = 50;
		//ImGui::DragInt("CinderSpinner", &CinderSpinnervalue, 1.0f, 0, 100);

		DrawTimeline();
		ImGui::EndChild();
	}

	if (tabSettings.cameraOverride && ImGui::AddTab("Camera override"))
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
	if (mPosX > sidebarSetting.triggerWidth && !ImGui::IsMouseDragging()) {
		if (sidebarSetting.alpha > minAlpha) {
			sidebarSetting.alpha -= 0.02 * speed;
			sidebarSetting.alpha = std::max(minAlpha, sidebarSetting.alpha);
		}
		if (sidebarSetting.posOffset < actualWidth)
			sidebarSetting.posOffset += 3 * speed;
	}
	//Show sidebar
	if (mPosX < 250) {
		if (sidebarSetting.alpha < 1) {
			sidebarSetting.alpha += 0.02 * speed;
			sidebarSetting.alpha = std::min(1.0f, sidebarSetting.alpha);
		}
		if (sidebarSetting.posOffset > 0) {
			sidebarSetting.posOffset -= 3 * speed;
			sidebarSetting.posOffset = std::max(0.0f, sidebarSetting.posOffset);
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
		totalReplayTime = replay.GetNumFrames() / replayServer.GetReplayFPS();
	}
	else {
	}
	ImGui::BeginTimeline("Timeline", totalReplayTime);

	static float currentTime = 0;
	static float seekTime = 0.0f;
	auto frames = dollyCam->GetUsedFrames();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 237.0 / 255, 25.0 / 255, 0, 1 });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 1.0, 72.0 / 255, 50.0 / 255, 1 });
	ImGui::PushStyleColor(ImGuiCol_ColumnActive, ImVec4{ 1, 87.0 / 255, 67.0 / 255, 1 });
	if (ImGui::TimelineMarker("currentFrame", currentTime, "Time: "))
	{
		seekTime = currentTime;
	}
	else {
		if (!replayServer.IsNull())
		{
			auto replay = replayServer.GetReplay();
			currentTime = replay.GetCurrentFrame() / replayServer.GetReplayFPS();
		}
	}
	ImGui::PopStyleColor(3);
	if (IsItemJustMadeInactive())
	{
		int _seekTime = seekTime; //lambda can't capture static
		gameWrapper->Execute([_seekTime, this](GameWrapper* gw) {
			auto replayServer = gameWrapper->GetGameEventAsReplay();
			int frame = _seekTime * replayServer.GetReplayFPS();
			replayServer.SkipToFrame(frame);
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
	ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
	auto io = ImGui::GetIO();
	//io.Fonts->AddFontDefault();
	auto default = io.Fonts->AddFontFromFileTTF("bakkesmod/font.ttf", 13.0f);
	ImFontConfig config;
	config.MergeMode = true;
	static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	fa = io.Fonts->AddFontFromFileTTF("bakkesmod/data/fonts/fa-solid-900.ttf", 13.0f, &config, icon_ranges);
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