#include "pch.h"
#include <map>
#include "splineinterp.h"
#include "nbezierinterp.h"
#include "bakkesmod/wrappers/wrapperstructs.h"
#include "utils/parser.h"
#include "..\UE4MathConverters.h"

namespace U = UE4Math;

vector<tinyspline::real> SolveForT(tinyspline::BSpline &spline, float tGoal, float e, int maxSteps = 50)
{
	float uMin = 0;
	float uMax = 1;

	double u = (uMin + uMax) / 2;
	auto res = spline.eval(u).result();
	auto t = res[0];
	float error = abs(t - tGoal);
	int steps = 1;
	while (error > e || steps > maxSteps)
	{
		if (t < tGoal) {
			uMin = (uMin + uMax) / 2;
		}
		if (t > tGoal)
		{
			uMax = (uMin + uMax) / 2;
		}
		u = (uMin + uMax) / 2;
		res = spline.eval(u).result();
		t = res[0];
		error = abs(t - tGoal);
		steps++;
	}
	return res;
}


SplineInterpStrategy::SplineInterpStrategy(std::shared_ptr<savetype> _camPath, int degree)
{
	setCamPath(_camPath, degree);
	backupStrategy = std::make_shared<NBezierInterpStrategy>(NBezierInterpStrategy(_camPath, degree));
	int n = camPath->size();
	if (n >= 4)
	{
		InitPositions(n);
		InitRotations(n);
		InitFOVs(n);
		//InitSlerp(n);
	}

}

NewPOV SplineInterpStrategy::GetPOV(float latestFrame)
{
	//auto t = GetRelativeTime(gameTime);
	auto t = GetRelativeTimeFromFrame(latestFrame);

	int n = camPath->size();
	if (n < 4)
	{
		return backupStrategy->GetPOV(latestFrame);
	}
	auto nextSnapshot = camPath->upper_bound(latestFrame);
	auto currentSnapshot = std::prev(nextSnapshot);
	//auto percent = 1.0 - (nextSnapshot->second.timeStamp - gameTime) / ((float)nextSnapshot->second.timeStamp - currentSnapshot->second.timeStamp);
	auto framePercent = 1.0 - (nextSnapshot->second.frame - latestFrame) / ((float)nextSnapshot->second.frame - currentSnapshot->second.frame);
	if (currentSnapshot == camPath->end() || nextSnapshot == camPath->end() || camPath->begin()->first > latestFrame || t > 1) //We're at the end of the playback
		return{ Vector(0), Rotator(0,0,0), 0 };


	int accuracy = cvarManager->getCvar("dolly_spline_acc").getIntValue();
	float epsilon = 1.0 / accuracy; // Acceptable error is 1 / 1000 seconds.
	auto posRes = camPositions.bisect(latestFrame, epsilon).result();
	auto rotRes = camRotations.bisect(latestFrame, epsilon).result();
	auto fovRes = camFOVs.bisect(latestFrame, epsilon).result();

	//auto q1 = slerpQuats[currentSnapshot->second.frame];
	//auto q2 = slerpQuats[nextSnapshot->second.frame];

	//auto q1T = quatTangents[currentSnapshot->second.frame];
	//auto q2T = quatTangents[nextSnapshot->second.frame];
	//cvarManager->log("Latest frame:" + std::to_string(latestFrame));
	//cvarManager->log("Current frame:" + std::to_string(currentSnapshot->second.frame));
	//cvarManager->log("Next frame:" + std::to_string(nextSnapshot->second.frame));
	//cvarManager->log("Percent:" + std::to_string(framePercent));

	//cvarManager->log(q1.Rotator().ToString());
	//cvarManager->log(q2.Rotator().ToString());
	//cvarManager->log(q1T.Rotator().ToString());
	//cvarManager->log(q2T.Rotator().ToString());
	//auto qSlerp = U::FQuat::Slerp(q1, q2, framePercent);
	//auto newRotator = ToBMRotator(qSlerp);

	//auto qSquad = U::FQuat::Squad(q1, q1T, q2, q2T, framePercent);
	//cvarManager->log(qSquad.Rotator().ToString() + "\n");
	//auto newRotator = ToBMRotator(qSquad);

	Vector v;
	v.X = float(posRes[1]);
	v.Y = float(posRes[2]);
	v.Z = float(posRes[3]);

	float fov = float(fovRes[1]);

	//CustomRotator rot = CustomRotator(float(rotRes[1]), float(rotRes[2]), float(rotRes[3]));
	Rotator rot = Rotator(float(rotRes[1]), float(rotRes[2]), float(rotRes[3]));
	return {v, rot, fov};
}

std::string SplineInterpStrategy::GetName()
{
	return "Spline interpolation";
}

float SplineInterpStrategy::GetRelativeTimeFromFrame(float frame)
{
	auto startSnapshot = camPath->begin();
	auto endSnapshot = (--camPath->end());

	float startFrame = startSnapshot->second.frame;
	float endFrame = endSnapshot->second.frame;

	float totalFrames = endFrame - startFrame;
	float relativeTime = (frame - startFrame) / totalFrames;
	if (relativeTime < 0) return 0;
	if (relativeTime > 1) return 1;
	return relativeTime;
}


void SplineInterpStrategy::InitFOVs(int numberOfPoints)
{
	auto POVs = vector<tinyspline::real>();

	auto fovsControllPoints = camFOVs.controlPoints();
	for (const auto& item : *camPath)
	{
		auto point = item.second;
		//POVs.push_back(double(point.timeStamp));
		POVs.push_back(double(point.frame));
		POVs.push_back(double(point.FOV));
	}
	camFOVs = tinyspline::Utils::interpolateCubic(&POVs, 2);
}

void SplineInterpStrategy::InitRotations(int numberOfPoints)
{
	//(t, x, y, z)
	auto rotations = vector<tinyspline::real>();

	auto previousRotation = camPath->begin()->second.rotation;
	float accumulatedPitch = previousRotation.Pitch._value;
	float accumulatedYaw = previousRotation.Yaw._value;
	float accumulatedRoll = previousRotation.Roll._value;

	for (const auto& item : *camPath)
	{
		auto point = item.second;
		auto thisRotator = point.rotation;
		auto diffRotation = previousRotation.diffTo(thisRotator);

		accumulatedPitch += diffRotation.Pitch._value;
		accumulatedYaw += diffRotation.Yaw._value;
		accumulatedRoll += diffRotation.Roll._value;

		previousRotation = thisRotator;

		//rotations.push_back(double(point.timeStamp));
		rotations.push_back(double(point.frame));
		rotations.push_back(double(accumulatedPitch));
		rotations.push_back(double(accumulatedYaw));
		rotations.push_back(double(accumulatedRoll));
	}

	camRotations = tinyspline::Utils::interpolateCubic(&rotations, 4);
}

void SplineInterpStrategy::InitPositions(int numberOfPoints)
{
	//(t, x, y, z)
	auto positions = vector<tinyspline::real>();
	for (const auto& item : *camPath)
	{
		auto point = item.second;
		//positions.push_back(double(point.timeStamp));
		positions.push_back(double(point.frame));
		positions.push_back(double(point.location.X));
		positions.push_back(double(point.location.Y));
		positions.push_back(double(point.location.Z));
	}
	camPositions = tinyspline::Utils::interpolateCubic(&positions, 4);
}

//void SplineInterpStrategy::InitSlerp(int numberOfPoints)
//{
	//auto rotations = std::map<int, U::FQuat>();
	//std::vector<int> frames;
	//auto snapshots = *camPath;
	//for (auto& item : snapshots)
	//{
	//	frames.push_back(item.second.frame);
	//	Rotator rot = item.second.rotation_rotator;
	//	U::FQuat q = ToQuat(rot);
	//	q.Normalize();
	//	rotations.emplace(item.second.frame, q);
	//}
	//for (size_t i = 0; i < frames.size(); i++)
	//{
	//	U::FQuat prevQ;
	//	U::FQuat Q = rotations[frames[i]];
	//	U::FQuat nextQ;
	//	if (i == 0){
	//		prevQ = rotations[frames[i]];
	//	}
	//	else {
	//		prevQ = rotations[frames[i - 1]];
	//	}
	//	if (i == frames.size() - 1) {
	//		nextQ = rotations[frames[i]];
	//	}
	//	else {
	//		nextQ = rotations[frames[i + 1]];
	//	}
	//	U::FQuat tangent;
	//	U::FQuat::CalcTangents(prevQ, Q, nextQ, 20, tangent);

	//	quatTangents.emplace(frames[i], tangent);
	//}
	//slerpQuats = rotations;
//}