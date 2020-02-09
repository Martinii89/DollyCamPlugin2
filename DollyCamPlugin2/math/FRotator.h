#pragma once

struct Rotator;

//FRotator is already in the sdk.
struct FFRotator
{
	/** Rotation around the right axis (around Y axis), Looking up and down (0=Straight Ahead, +Up, -Down) */
	float Pitch;

	/** Rotation around the up axis (around Z axis), Running in circles 0=East, +North, -South. */
	float Yaw;

	/** Rotation around the forward axis (around X axis), Tilting your head, 0=Straight, +Clockwise, -CCW. */
	float Roll;

	FFRotator() : Pitch(0), Yaw(0), Roll(0) {};
	FFRotator(const Rotator& R);
	Rotator ToRotator() const;

	static inline float ClampAxis(float Angle)
	{
		// returns Angle in the range (-360,360)
		Angle = FMath::Fmod(Angle, 360.f);

		if (Angle < 0.f)
		{
			// shift to [0,360) range
			Angle += 360.f;
		}

		return Angle;
	}
	static inline float NormalizeAxis(float Angle) {
		// returns Angle in the range [0,360)
		Angle = ClampAxis(Angle);

		if (Angle > 180.f)
		{
			// shift to (-180,180]
			Angle -= 360.f;
		}

		return Angle;
	}
};

