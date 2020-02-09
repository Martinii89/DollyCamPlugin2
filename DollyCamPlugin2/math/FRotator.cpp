#include "bakkesmod/wrappers/wrapperstructs.h"
#include "FMath.h"
#include "FRotator.h"


FFRotator::FFRotator(const Rotator& R)
{
	Pitch = R.Pitch * UnrRotToDeg;
	Yaw = R.Yaw * UnrRotToDeg;
	Roll = R.Roll * UnrRotToDeg;
}

Rotator FFRotator::ToRotator() const
{
	return { (int)(Pitch * DegToUnrRot) , (int)(Yaw * DegToUnrRot) , (int)(Roll * DegToUnrRot) };
}
