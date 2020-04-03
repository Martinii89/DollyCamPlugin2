#include "pch.h"
#include "serialization.h"
#include "utils\parser.h"
#include "bakkesmod\wrappers\wrapperstructs.h"

std::string vector_to_string(Vector v)
{
	return to_string_with_precision(v.X, 2) + ", " + to_string_with_precision(v.Y, 2) + ", " + to_string_with_precision(v.Z, 2);
}

std::string rotator_to_string(Rotator r)
{
	return to_string_with_precision(r.Pitch, 2) + ", " + to_string_with_precision(r.Yaw, 2) + ", " + to_string_with_precision(r.Roll, 2);
}

void to_json(json& j, const Vector& p) {
	j = json{ { "x", p.X },{ "y", p.Y },{ "z", p.Z } };
}

void from_json(const json& j, Vector& p) {
	p.X = j.at("x").get<float>();
	p.Y = j.at("y").get<float>();
	p.Z = j.at("z").get<float>();
}

void to_json(json& j, const Rotator& p) {
	j = json{ { "pitch", p.Pitch },{ "yaw", p.Yaw },{ "roll", p.Roll } };
}

void from_json(const json& j, Rotator& p) {
	p.Pitch = j.at("pitch").get<float>();
	p.Yaw = j.at("yaw").get<float>();
	p.Roll = j.at("roll").get<float>();
}

void to_json(json& j, const CustomRotator& p) {
	j = json{ { "pitch", p.Pitch._value },{ "yaw", p.Yaw._value },{ "roll", p.Roll._value } };
}

void from_json(const json& j, CustomRotator& p) {
	p.Pitch._value = j.at("pitch").get<float>();
	p.Yaw._value = j.at("yaw").get<float>();
	p.Roll._value = j.at("roll").get<float>();
}

void to_json(json& j, const CameraSnapshot& p) {
	j = json{ { "frame", p.frame }/*,{ "timestamp", p.timeStamp }*/,{ "FOV", p.FOV },
	{ "location", p.location },{ "rotation", p.rotation }, { "rotation_rotator", p.rotation_rotator },{ "weight", p.weight } };
}

void from_json(const json& j, CameraSnapshot& p) {
	p.frame = j.at("frame").get<int>();
	//p.timeStamp = j.at("timestamp").get<float>();
	p.FOV = j.at("FOV").get<float>();
	p.location = j.at("location").get<Vector>();
	p.rotation = (j.at("rotation").get<CustomRotator>());
	j.at("rotation_rotator").get_to(p.rotation_rotator);
	p.weight = j.at("weight").get<float>();
}

void to_json(json& j, const SidebarSettings& p)
{
	j = json{ {"compact", p.compact}, {"editTimeLimit", p.editTimeLimit}, {"height", p.height}, {"LocationPower", p.LocationPower},
	{"LocationSpeed", p.LocationSpeed}, {"RotationPower", p.RotationPower}, {"RotationSpeed", p.RotationSpeed},
	{"transitionSpeed", p.transitionSpeed},{"mouseTransition", p.mouseTransition}, {"triggerWidth", p.triggerWidth}, {"width", p.width} };
}

void to_json(json& j, const TabsSettings& p)
{
	j = json{ {"cameraOverride", p.cameraOverride}, {"oldSnapshots", p.oldSnapshots} };
}

void to_json(json& j, const GuiState& p)
{
	j = json{ {"sidebarSettings", p.sidebarSettings}, {"tabsSettings", p.tabsSettings}, {"dollySettings", p.dollySettings} };
}

void to_json(json& j, const DollySettings& p)
{
	j = json{ {"animationResetActive", p.animationResetActive}, {"renderDollyPath", p.renderDollyPath}, 
	{"renderFrameTicks", p.renderFrameTicks}, {"visualCameraActive", p.visualCameraActive} };
}

void from_json(const json& j, SidebarSettings& p)
{
	j.at("compact").get_to(p.compact);
	j.at("editTimeLimit").get_to(p.editTimeLimit);
	j.at("height").get_to(p.height);
	j.at("LocationPower").get_to(p.LocationPower);
	j.at("LocationSpeed").get_to(p.LocationSpeed);
	j.at("RotationPower").get_to(p.RotationPower);
	j.at("RotationSpeed").get_to(p.RotationSpeed);
	j.at("transitionSpeed").get_to(p.transitionSpeed);
	j.at("mouseTransition").get_to(p.mouseTransition);
	j.at("triggerWidth").get_to(p.triggerWidth);
	j.at("width").get_to(p.width);
};

void from_json(const json& j, TabsSettings& p)
{
	j.at("cameraOverride").get_to(p.cameraOverride);
	j.at("oldSnapshots").get_to(p.oldSnapshots);
}

void from_json(const json& j, GuiState& p)
{
	j.at("sidebarSettings").get_to(p.sidebarSettings);
	j.at("tabsSettings").get_to(p.tabsSettings);
	if (j.count("dollySettings") != 0)
	{
		j.at("dollySettings").get_to(p.dollySettings);
	}
}

void from_json(const json& j, DollySettings& p)
{
	j.at("animationResetActive").get_to(p.animationResetActive);
	j.at("renderDollyPath").get_to(p.renderDollyPath);
	j.at("renderFrameTicks").get_to(p.renderFrameTicks);
	j.at("visualCameraActive").get_to(p.visualCameraActive);
}
