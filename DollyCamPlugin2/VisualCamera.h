#pragma once
#include "bakkesmod/plugin/bakkesmodplugin.h"

class VisualCamera
{
private:
	Vector originalMatteBoxSide[4];
	Vector originalBodySide[7];
	Vector originalReelSide[12];//These values are offsets from the center of the reel polygon. Need to define center point when creating a new reel.
	Vector originalLens[8];//These are offsets from the center of the lens
	vector<Vector> allOriginalCameraVerts;
	vector<Vector> allCalculatedCameraVerts;

	Quat RotatorToQuat(Rotator rotation);
	Vector RotateVectorWithQuat(Vector vec, Quat quat);
	void TransformCamera(Vector location, Rotator rotation, float scale);
	void DrawObject(CanvasWrapper canvas, int startingIndex, int range);

public:
	VisualCamera();
	~VisualCamera();

	void DrawCamera(CanvasWrapper canvas, Vector location, Rotator rotation, float scale=1, LinearColor color={255,255,255,255});
};

