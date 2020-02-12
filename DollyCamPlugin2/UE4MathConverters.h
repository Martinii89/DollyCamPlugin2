#pragma once
#include "Math/UnrealMath.h"
#include "bakkesmod/wrappers/wrapperstructs.h"

constexpr auto RadToDeg = 57.295779513082321600;    // 180 / Pi
constexpr auto DegToRad = 0.017453292519943296;    // Pi / 180
constexpr auto UnrRotToRad = 0.00009587379924285;// Pi / 32768
constexpr auto RadToUnrRot = 10430.3783504704527;// 32768 / Pi
constexpr auto DegToUnrRot = 182.0444;
constexpr auto UnrRotToDeg = 0.00549316540360483;

inline UE4Math::FRotator ToFRotator(const Rotator& R)
{
	UE4Math::FRotator res;
	res.Pitch = R.Pitch * UnrRotToDeg;
	res.Yaw = R.Yaw * UnrRotToDeg;
	res.Roll = R.Roll * UnrRotToDeg;
	return res;
}

inline UE4Math::FQuat ToQuat(const Rotator& R)
{
	auto rot = ToFRotator(R);
	return UE4Math::FQuat(rot);
}

inline Rotator ToBMRotator(const UE4Math::FRotator r)
{
	return { (int)(r.Pitch * DegToUnrRot) , (int)(r.Yaw * DegToUnrRot) , (int)(r.Roll * DegToUnrRot) };
}

inline Rotator ToBMRotator(const UE4Math::FQuat q)
{
	auto rot = q.Rotator();
	return ToBMRotator(rot);
}


