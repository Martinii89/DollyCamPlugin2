#include "pch.h"
#include "models.h"

POV NewPOV::ToPOV()
{
	return{ location, rotation_rotator, FOV };
}
