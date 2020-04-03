#include "pch.h"
#include "linearinterp.h"

LinearInterpStrategy::LinearInterpStrategy(std::shared_ptr<savetype> _camPath, int degree)
{
	setCamPath(_camPath, degree);
	//camPath = std::make_unique<savetype>(*_camPath); //Copy campath
}

NewPOV LinearInterpStrategy::GetPOV(float latestFrame)
{
	auto nextSnapshot = camPath->upper_bound(latestFrame);
	auto currentSnapshot = std::prev(nextSnapshot);

	if (currentSnapshot == camPath->end() || nextSnapshot == camPath->end() || camPath->begin()->first > latestFrame) //We're at the end of the playback
		return{ Vector(0), Rotator(0,0,0), 0 };

	float t = percElapsed(latestFrame);

	NewPOV pov; 
	pov.location = currentSnapshot->second.location + (nextSnapshot->second.location - currentSnapshot->second.location) * t;

	CustomRotator dif = (currentSnapshot->second.rotation.diffTo(nextSnapshot->second.rotation));
	CustomRotator dif2 = dif * t;
	CustomRotator rot2 = currentSnapshot->second.rotation + dif2;
	pov.rotation_rotator = rot2.ToRotator();


	pov.FOV = currentSnapshot->second.FOV + (nextSnapshot->second.FOV - currentSnapshot->second.FOV) * t;

	return pov;
}

std::string LinearInterpStrategy::GetName()
{
	return "linear interpolation";
}