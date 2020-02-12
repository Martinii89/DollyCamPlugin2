#pragma once
#include "interpstrategy.h"
#include "tinyspline\tinysplinecpp.h"
#include "bakkesmod\plugin\bakkesmodplugin.h"
#include "Math/Quat.h"

class SplineInterpStrategy : public InterpStrategy
{
public:
	SplineInterpStrategy(std::shared_ptr<savetype> _camPath, int degree);
	virtual NewPOV GetPOV(float latestFrame);
	virtual std::string GetName();
	std::shared_ptr<CVarManagerWrapper> cvarManager;

private:

	float GetRelativeTimeFromFrame(float frame);


	void InitFOVs(int numberOfPoints);
	void InitRotations(int numberOfPoints);
	void InitPositions(int numberOfPoints);
	//void InitSlerp(int numberOfPoints);

	tinyspline::BSpline camPositions;
	tinyspline::BSpline camRotations;
	tinyspline::BSpline camFOVs;

	//std::map<int,  UE4Math::FQuat> slerpQuats;
	//std::map<int,  UE4Math::FQuat> quatTangents;

	std::shared_ptr<InterpStrategy> backupStrategy;
};