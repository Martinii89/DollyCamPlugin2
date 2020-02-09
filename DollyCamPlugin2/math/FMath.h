#pragma once

#include <cmath>


constexpr auto PI = (3.1415926535897932f);
constexpr auto INV_PI = (0.31830988618f);
constexpr auto HALF_PI = (1.57079632679f);

constexpr auto RadToDeg = 57.295779513082321600;    // 180 / Pi
constexpr auto DegToRad = 0.017453292519943296;    // Pi / 180
constexpr auto UnrRotToRad = 0.00009587379924285;// Pi / 32768
constexpr auto RadToUnrRot = 10430.3783504704527;// 32768 / Pi
constexpr auto DegToUnrRot = 182.0444;
constexpr auto UnrRotToDeg = 0.00549316540360483;

struct FMath
{
	static float Fmod(float x, float y) { return std::fmodf(x, y); }
	static inline float Abs(float x) { return std::fabs(x); }
	static inline float Sqrt(float x) { return std::sqrtf(x); }
	static inline float InvSqrt(float x) { return 1.0f / std::sqrtf(x); }


	static inline void SinCos(float* ScalarSin, float* ScalarCos, float  Value)
	{
		// Map Value to y in [-pi,pi], x = 2*pi*quotient + remainder.
		float quotient = (INV_PI * 0.5f) * Value;
		if (Value >= 0.0f)
		{
			quotient = (float)((int)(quotient + 0.5f));
		}
		else
		{
			quotient = (float)((int)(quotient - 0.5f));
		}
		float y = Value - (2.0f * PI) * quotient;

		// Map y to [-pi/2,pi/2] with sin(y) = sin(Value).
		float sign;
		if (y > HALF_PI)
		{
			y = PI - y;
			sign = -1.0f;
		}
		else if (y < -HALF_PI)
		{
			y = -PI - y;
			sign = -1.0f;
		}
		else
		{
			sign = +1.0f;
		}

		float y2 = y * y;

		// 11-degree minimax approximation
		*ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

		// 10-degree minimax approximation
		float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
		*ScalarCos = sign * p;
	}

	static inline float Sin(float x) { return std::sinf(x); }

	static inline float Atan2(const float x, const float y) { return std::atan2f(x, y); }
	static inline float Acos(const float x) { return std::acosf(x); }

	static inline float FastAsin(float Value)
	{
		return std::asin(Value);
		//// Clamp input to [-1,1].
		//bool nonnegative = (Value >= 0.0f);
		//float x = FMath::Abs(Value);
		//float omx = 1.0f - x;
		//if (omx < 0.0f)
		//{
		//	omx = 0.0f;
		//}
		//float root = FMath::Sqrt(omx);
		//// 7-degree minimax approximation
		//float result = ((((((-0.0012624911f * x + 0.0066700901f) * x - 0.0170881256f) * x + 0.0308918810f) * x - 0.0501743046f) * x + 0.0889789874f) * x - 0.2145988016f) * x + FASTASIN_HALF_PI;
		//result *= root;  // acos(|x|)
		//				 // acos(x) = pi - acos(-x) when x < 0, asin(x) = pi/2 - acos(x)
		//return (nonnegative ? FASTASIN_HALF_PI - result : result - FASTASIN_HALF_PI);
	}

	static constexpr inline float FloatSelect(float Comparand, float ValueGEZero, float ValueLTZero)
	{
		return Comparand >= 0.f ? ValueGEZero : ValueLTZero;
	}
	/** Multiples value by itself */
	template< class T >
	static inline T Square(const T A)
	{
		return A * A;
	}
};
