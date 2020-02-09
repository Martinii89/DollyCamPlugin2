#include "Fquat.h"

/**
* Creates and initializes a new quaternion from the given rotator.
*
* @param R The rotator to initialize from.
*/

FQuat::FQuat(const FFRotator& R)
{
	const float DEG_TO_RAD = PI / (180.f);
	const float RADS_DIVIDED_BY_2 = DEG_TO_RAD / 2.f;
	float SP, SY, SR;
	float CP, CY, CR;

	const float PitchNoWinding = FMath::Fmod(R.Pitch, 360.0f);
	const float YawNoWinding = FMath::Fmod(R.Yaw, 360.0f);
	const float RollNoWinding = FMath::Fmod(R.Roll, 360.0f);

	FMath::SinCos(&SP, &CP, PitchNoWinding * RADS_DIVIDED_BY_2);
	FMath::SinCos(&SY, &CY, YawNoWinding * RADS_DIVIDED_BY_2);
	FMath::SinCos(&SR, &CR, RollNoWinding * RADS_DIVIDED_BY_2);

	X = CR * SP * SY - SR * CP * CY;
	Y = -CR * SP * CY - SR * CP * SY;
	Z = CR * CP * SY - SR * SP * CY;
	W = CR * CP * CY + SR * SP * SY;
}

FQuat FQuat::Slerp_NotNormalized(const FQuat& Quat1, const FQuat& Quat2, float Slerp)
{
	// Get cosine of angle between quats.
	const float RawCosom =
		Quat1.X * Quat2.X +
		Quat1.Y * Quat2.Y +
		Quat1.Z * Quat2.Z +
		Quat1.W * Quat2.W;
	// Unaligned quats - compensate, results in taking shorter route.
	const float Cosom = FMath::FloatSelect(RawCosom, RawCosom, -RawCosom);

	float Scale0, Scale1;

	if (Cosom < 0.9999f)
	{
		const float Omega = FMath::Acos(Cosom);
		const float InvSin = 1.f / FMath::Sin(Omega);
		Scale0 = FMath::Sin((1.f - Slerp) * Omega) * InvSin;
		Scale1 = FMath::Sin(Slerp * Omega) * InvSin;
	}
	else
	{
		// Use linear interpolation.
		Scale0 = 1.0f - Slerp;
		Scale1 = Slerp;
	}

	// In keeping with our flipped Cosom:
	Scale1 = FMath::FloatSelect(RawCosom, Scale1, -Scale1);

	FQuat Result;

	Result.X = Scale0 * Quat1.X + Scale1 * Quat2.X;
	Result.Y = Scale0 * Quat1.Y + Scale1 * Quat2.Y;
	Result.Z = Scale0 * Quat1.Z + Scale1 * Quat2.Z;
	Result.W = Scale0 * Quat1.W + Scale1 * Quat2.W;

	return Result;
}

const FQuat FQuat::Identity(0, 0, 0, 1);