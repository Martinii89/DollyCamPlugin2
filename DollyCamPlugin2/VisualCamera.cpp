#include "pch.h"
#include "VisualCamera.h"
#define _USE_MATH_DEFINES
#include <math.h>


VisualCamera::VisualCamera()
{
	//Matte box (4 verts)
	originalMatteBoxSide[0] = Vector{49.3646f,-19.1627f,10.9706f};
	originalMatteBoxSide[1] = Vector{49.3646f,-19.1627f,-13.8512f};
	originalMatteBoxSide[2] = Vector{35.7538f,-7.15999f,-7.56008f};
	originalMatteBoxSide[3] = Vector{35.7538f,-7.15999f,4.67943f};

	//Body (7 verts)
	originalBodySide[0] = Vector{21.6909f,-8.78553f,5.29488f};
	originalBodySide[1] = Vector{21.6909f,-8.78552f,-8.17553f};
	originalBodySide[2] = Vector{13.3338f,-8.78552f,-14.3408f};
	originalBodySide[3] = Vector{-12.42f,-8.78552f,-14.3408f};
	originalBodySide[4] = Vector{-19.6006f,-8.78553f,-4.96522f};
	originalBodySide[5] = Vector{-9.44514f,-8.78553f,12.487f};
	originalBodySide[6] = Vector{13.2901f,-8.78553f,12.487f};

	//Reel (12 verts)
	originalReelSide[0] = Vector{13.9088f,0,3.72685f};
	originalReelSide[1] = Vector{13.9088f,0,-3.72686f};
	originalReelSide[2] = Vector{10.1819f,0,-10.182f};
	originalReelSide[3] = Vector{3.72684f,0,-13.9088f};
	originalReelSide[4] = Vector{-3.72686f,0,-13.9088f};
	originalReelSide[5] = Vector{-10.182f,0,-10.1819f};
	originalReelSide[6] = Vector{-13.9088f,0,-3.72685f};
	originalReelSide[7] = Vector{-13.9088f,0,3.72685f};
	originalReelSide[8] = Vector{-10.1819f,0,10.1819f};
	originalReelSide[9] = Vector{-3.72685f,0,13.9088f};
	originalReelSide[10] = Vector{3.72685f,0,13.9088f};
	originalReelSide[11] = Vector{10.1819f,0,10.1819f};

	//Lens (8 verts)
	originalLens[0] = Vector{-7.03141f,0,5.31923f};
	originalLens[1] = Vector{-7.03141f,-3.76126f,3.76126f};
	originalLens[2] = Vector{-7.03141f,-5.31923f,0};
	originalLens[3] = Vector{-7.03141f,-3.76126f,-3.76126f};
	originalLens[4] = Vector{-7.03141f,0,-5.31923f};
	originalLens[5] = Vector{-7.03141f,3.76126f,-3.76126f};
	originalLens[6] = Vector{-7.03141f,5.31923f,0};
	originalLens[7] = Vector{-7.03141f,3.76126f,3.76126f};


	//FILL OUT ALL ORIGINAL POINTS
	
	allOriginalCameraVerts.clear();
	//Matte box (8 verts): 0-7
	for(int i=0; i<4; i++)
	{
		Vector temp = originalMatteBoxSide[i];
		allOriginalCameraVerts.push_back(temp);
		temp.Y *= -1;
		allOriginalCameraVerts.push_back(temp);
	}
	//Body (14 verts): 8-21
	for(int i=0; i<7; i++)
	{
		Vector temp = originalBodySide[i];
		allOriginalCameraVerts.push_back(temp);
		temp.Y *= -1;
		allOriginalCameraVerts.push_back(temp);
	}
	//Reels (24 verts): 22-69
	for(int reel=0; reel<2; reel++)
	{
		Vector reelPosition = {0,0,0};
		if(reel == 0){reelPosition = Vector{-1.441f, -8.79f, 26.4f};};//top reel
		if(reel == 1){reelPosition = Vector{-25.531f, -8.79f, 12.49f};};//back reel
		for(int i=0; i<12; i++)
		{
			Vector temp = originalReelSide[i] + reelPosition;
			allOriginalCameraVerts.push_back(temp);
			temp.Y *= -1;
			allOriginalCameraVerts.push_back(temp);
		}
	}
	//Lens (16 verts): 70-85
	for(int i=0; i<8; i++)
	{
		Vector lensPosition = {28.772f, 0.f, -1.44f};
		Vector temp = lensPosition + originalLens[i];
		allOriginalCameraVerts.push_back(temp);
		Vector temp2 = originalLens[i];
		temp2.X *= -1;
		temp = lensPosition + temp2;
		allOriginalCameraVerts.push_back(temp);
	}
}

VisualCamera::~VisualCamera(){}

void VisualCamera::TransformCamera(Vector location, Rotator rotation, float scale)
{
	allCalculatedCameraVerts.clear();
	Quat quat = RotatorToQuat(rotation);
	for(int i=0; i<allOriginalCameraVerts.size(); i++)
	{
		Vector calculatedVert = RotateVectorWithQuat(allOriginalCameraVerts[i], quat);
		calculatedVert = calculatedVert * scale;
		calculatedVert = calculatedVert + location;
		allCalculatedCameraVerts.push_back(calculatedVert);
	}
}

void VisualCamera::DrawObject(CanvasWrapper canvas, int startingIndex, int range)
{
	for(int i = 0; i < range; i+=2)
	{
		if(i == range-2)
		{
			canvas.DrawLine(canvas.Project(allCalculatedCameraVerts[startingIndex + range-1]), canvas.Project(allCalculatedCameraVerts[startingIndex+1]));
			canvas.DrawLine(canvas.Project(allCalculatedCameraVerts[startingIndex + range-2]), canvas.Project(allCalculatedCameraVerts[startingIndex]));
		}
		else
		{
			canvas.DrawLine(canvas.Project(allCalculatedCameraVerts[startingIndex + i]), canvas.Project(allCalculatedCameraVerts[startingIndex + i + 2]));
			canvas.DrawLine(canvas.Project(allCalculatedCameraVerts[startingIndex + i + 1]), canvas.Project(allCalculatedCameraVerts[startingIndex + i + 3]));
		}
		canvas.DrawLine(canvas.Project(allCalculatedCameraVerts[startingIndex + i]), canvas.Project(allCalculatedCameraVerts[startingIndex + i + 1]));
	}
}

void VisualCamera::DrawCamera(CanvasWrapper canvas, Vector location, Rotator rotation, float scale, LinearColor color)
{
	TransformCamera(location, rotation, scale);
	canvas.SetColor((char)color.R, (char)color.G, (char)color.B, (char)color.A);

	//DRAW LINES
	vector<int> objectRanges;//Number of vertices per object
	objectRanges.push_back(8);//Matte box
	objectRanges.push_back(14);//Body
	objectRanges.push_back(24);//Reel 1
	objectRanges.push_back(24);//Reel 2
	objectRanges.push_back(16);//Lens
	int lineIndex = 0;

	for(int i=0; i<objectRanges.size(); i++)
	{
		DrawObject(canvas, lineIndex, objectRanges[i]);
		lineIndex += objectRanges[i];
	}
}





//MATH FUNCTIONS

Quat VisualCamera::RotatorToQuat(Rotator rot)
{
	float rotatorToRadian = ((M_PI/180)/2)/182.044449;
	float sinPitch = sin(rot.Pitch*rotatorToRadian);
	float cosPitch = cos(rot.Pitch*rotatorToRadian);
	float sinYaw = sin(rot.Yaw*rotatorToRadian);
	float cosYaw = cos(rot.Yaw*rotatorToRadian);
	float sinRoll = sin(rot.Roll*rotatorToRadian);
	float cosRoll = cos(rot.Roll*rotatorToRadian);
	
	Quat convertedQuat;
	convertedQuat.X = (cosRoll*sinPitch*sinYaw) - (sinRoll*cosPitch*cosYaw);
	convertedQuat.Y = ((-cosRoll)*sinPitch*cosYaw) - (sinRoll*cosPitch*sinYaw);
	convertedQuat.Z = (cosRoll*cosPitch*sinYaw) - (sinRoll*sinPitch*cosYaw);
	convertedQuat.W = (cosRoll*cosPitch*cosYaw) + (sinRoll*sinPitch*sinYaw);

	return convertedQuat;
}
//from Arator's HelperFunctions: https://github.com/AratorRL/SciencePlugin/blob/ebb74838471f9352c1684b166f0d98eaa1604d31/SciencePlugin/HelperFunctions.cpp
Vector VisualCamera::RotateVectorWithQuat(Vector vec, Quat quat)
{
	Quat p;
	p.W = 0;
	p.X = vec.X;
	p.Y = vec.Y;
	p.Z = vec.Z;

	Quat result = (quat * p) * quat.conjugate();
	return Vector(result.X, result.Y, result.Z);
}
