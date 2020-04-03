#include "pch.h"
#include "RenderingTools.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>

using namespace RenderingTools;



//Quat conversions
Rotator RenderingTools::QuatToRotator(Quat quat)
{
	//From Arator's HelperFunctions
	Matrix3 matrix = QuatToMatrix(quat);
	Vector fwd = matrix.forward;
	Vector up = matrix.right;
	Vector right = matrix.up;

	//Pitch
	float pitch_f = asin(fwd.Z);
	int PITCH = (pitch_f / (M_PI / 2)) * 16384;

	//Yaw
	float hor_mag = sqrt(fwd.X * fwd.X + fwd.Y * fwd.Y);
	float hor_y = fwd.Y / hor_mag;
	float fwd_y = asin(hor_y);
	if (fwd_y >= 0)
	{
		if (fwd.X >= 0)
			fwd_y = fwd_y;
		else
			fwd_y = M_PI - fwd_y;
	}
	else
	{
		if (fwd.X >= 0)
			fwd_y = fwd_y;
		else
			fwd_y = -M_PI - fwd_y;
	}
	int YAW = (fwd_y / M_PI) * 32768;

	//Roll
	Vector vert = Vector(0, 0, 1);
	if(up.Z < 0)
		vert = Vector(0, 0, -1);
	Vector hor_right = Vector::cross(fwd, vert);
	hor_right = { -hor_right.X, -hor_right.Y, -hor_right.Z }; // left-handed coordinate system
	hor_right.normalize();
	float roll_cos = Vector::dot(hor_right, right);
	float roll_f = acos(roll_cos);
	
	float up_f = asin(up.Z);
	
	if (right.Z >= 0)
	{
		if (up.Z >= 0)
			roll_f = -roll_f;
		else
			roll_f  = -M_PI + roll_f;
	}
	else
	{
		if (up.Z >= 0)
			roll_f = roll_f;
		else
			roll_f = M_PI - roll_f;
	}
	int ROLL = (roll_f / M_PI) * 32768;

	return Rotator(PITCH, YAW, ROLL);
}
Rotator RenderingTools::QuatToRotator2(Quat q)
{
	q = RenderingTools::NormalizeQuat(q);
	float Yaw, Roll, Pitch;
	// roll (x-axis rotation)
	double sinr_cosp = 2 * (q.W * q.X + q.Y * q.Z);
	double cosr_cosp = 1 - 2 * (q.X * q.X + q.Y * q.Y);
	Roll = std::atan2(sinr_cosp, cosr_cosp);

	// pitch (y-axis rotation)
	double sinp = 2 * (q.W * q.Y - q.Z * q.X);
	if (std::abs(sinp) >= 1)
		Pitch = std::copysign(M_PI / 2, sinp); // use 90 degrees if out of range
	else
		Pitch = std::asin(sinp);

	// yaw (z-axis rotation)
	double siny_cosp = 2 * (q.W * q.Z + q.X * q.Y);
	double cosy_cosp = 1 - 2 * (q.Y * q.Y + q.Z * q.Z);
	Yaw = std::atan2(siny_cosp, cosy_cosp);
	//const float radToURot = 180.0f / M_PI;
	const float radToURot = 32768 / M_PI;
	return Rotator(-Pitch * radToURot, Yaw * radToURot, -Roll * radToURot);
}
Quat RenderingTools::RotatorToQuat(Rotator rot)
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
Matrix3 RenderingTools::QuatToMatrix(Quat quat)
{
	//Using Arator's quatToXX functions
	Vector forward = RotateVectorWithQuat(Vector(1, 0, 0), quat);
	Vector right = RotateVectorWithQuat(Vector(0, 1, 0), quat);
	Vector up = RotateVectorWithQuat(Vector(0, 0, 1), quat);
	forward.normalize();
	right.normalize();
	up.normalize();

	return Matrix3{forward, right, up};
}
Quat RenderingTools::MatrixToQuat(Matrix3 matrix)
{
	//https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
	//W, X, Y, Z
	Quat q(1,0,0,0);

	//{ fwd.X, right.X, up.X }
	//{ fwd.Y, right.Y, up.Y }  ------->  newQuat
	//{ fwd.Z, right.Z, up.Z }
	float a[3][3];

	Vector fwd = matrix.forward;
	Vector right = matrix.right;
	Vector up = matrix.up;

	a[0][0] = fwd.X, a[0][1] = right.X, a[0][2] = up.X;
	a[1][0] = fwd.Y, a[1][1] = right.Y, a[1][2] = up.Y;
	a[2][0] = fwd.Z, a[2][1] = right.Z, a[2][2] = up.Z;


	float trace = a[0][0] + a[1][1] + a[2][2];
	if( trace > 0 )
	{
		float s = 0.5f / sqrtf(trace+ 1.0f);
		q.W = 0.25f / s;
		q.X = ( a[2][1] - a[1][2] ) * s;
		q.Y = ( a[0][2] - a[2][0] ) * s;
		q.Z = ( a[1][0] - a[0][1] ) * s;
	}
	else
	{
		if ( a[0][0] > a[1][1] && a[0][0] > a[2][2] )
		{
			float s = 2.0f * sqrtf( 1.0f + a[0][0] - a[1][1] - a[2][2]);
			q.W = (a[2][1] - a[1][2] ) / s;
			q.X = 0.25f * s;
			q.Y = (a[0][1] + a[1][0] ) / s;
			q.Z = (a[0][2] + a[2][0] ) / s;
		}
		else if (a[1][1] > a[2][2])
		{
			float s = 2.0f * sqrtf( 1.0f + a[1][1] - a[0][0] - a[2][2]);
			q.W = (a[0][2] - a[2][0] ) / s;
			q.X = (a[0][1] + a[1][0] ) / s;
			q.Y = 0.25f * s;
			q.Z = (a[1][2] + a[2][1] ) / s;
		}
		else
		{
			float s = 2.0f * sqrtf( 1.0f + a[2][2] - a[0][0] - a[1][1] );
			q.W = (a[1][0] - a[0][1] ) / s;
			q.X = (a[0][2] + a[2][0] ) / s;
			q.Y = (a[1][2] + a[2][1] ) / s;
			q.Z = 0.25f * s;
		}
	}

	return q;
}

float RenderingTools::QuatDot(const Quat a, const Quat b)
{
	return a.W * b.W + a.X * b.X + a.Y * b.Y + a.Z * b.Z;
}


//Rotations
Quat RenderingTools::AngleAxisRotation(float angle, Vector axis)
{
	//Angle in radians
	Quat result;
	float angDiv2 = angle * 0.5f;
	result.W = cos(angDiv2);
	result.X = axis.X * sin(angDiv2);
	result.Y = axis.Y * sin(angDiv2);
	result.Z = axis.Z * sin(angDiv2);

	return result;
}
Vector RenderingTools::RotateVectorWithQuat(Vector v, Quat q, bool normalize)
{
	//From Arator's HelperFunctions: https://github.com/AratorRL/SciencePlugin/blob/ebb74838471f9352c1684b166f0d98eaa1604d31/SciencePlugin/HelperFunctions.cpp
	Quat p;
	p.W = 0;
	p.X = v.X;
	p.Y = v.Y;
	p.Z = v.Z;

	Quat result = (q * p) * q.conjugate();
	Vector res = {result.X, result.Y, result.Z};

	if(normalize) res.normalize();

	return res;
}
Matrix3 RenderingTools::RotateMatrixWithQuat(Matrix3 matrix, Quat q, bool normalize)
{
	matrix.forward = RotateVectorWithQuat(matrix.forward, q);
	matrix.right = RotateVectorWithQuat(matrix.right, q);
	matrix.up = RotateVectorWithQuat(matrix.up, q);

	if(normalize) matrix.normalize();

	return matrix;
}
Matrix3 RenderingTools::SingleAxisAlignment(Matrix3 matrix, Vector targetDirection, int lookAtAxis, int rotationOrder, int step)
{
	//Rotate matrix along one axis to align with a vector
	//Used twice in LookAt to fully align a matrix with a vector

	Matrix3 rotOrder = GetRotationOrder(matrix, lookAtAxis, rotationOrder);
	Vector primaryRotationAxis;
	Vector secondaryRotationAxis;
	Vector finalAxis = rotOrder.up;
	if(step == 1)
	{
		primaryRotationAxis = rotOrder.forward;
		secondaryRotationAxis = rotOrder.right;
	}
	else
	{
		primaryRotationAxis = rotOrder.right;
		secondaryRotationAxis = rotOrder.forward;
	}

	Vector targetDirectionRejected = VectorRejection(targetDirection, primaryRotationAxis);
	targetDirectionRejected.normalize();
	float a = (targetDirectionRejected - finalAxis).magnitude();
	float b = targetDirectionRejected.magnitude();
	float c = finalAxis.magnitude();
	float rotAngle = acos((b*b + c*c - a*a)/2*b*c);
	if(step == 1)
	{
		if(Vector::dot(targetDirectionRejected, secondaryRotationAxis) >= 0)
			rotAngle *= -1;
	}
	else
	{
		if(Vector::dot(targetDirectionRejected, secondaryRotationAxis) <= 0)
			rotAngle *= -1;
	}
	Quat rotAngleQuat = AngleAxisRotation(rotAngle, primaryRotationAxis);
	return RotateMatrixWithQuat(matrix, rotAngleQuat, true);
}
Matrix3 RenderingTools::GetRotationOrder(Matrix3 inMatrix, int lookAtAxis, int rotationOrder)
{
	//Returning as a matrix is a bit janky, but I think its cleaner than returning vector<Vector>
	Vector firstRotationAxis;
	Vector secondRotationAxis;
	Vector finalAxis;
	if(lookAtAxis == 1)
	{
		finalAxis = inMatrix.forward;
		if(rotationOrder == 1)
		{
			firstRotationAxis = inMatrix.up;
			secondRotationAxis = inMatrix.right;
		}
		else
		{
			firstRotationAxis = inMatrix.right;
			secondRotationAxis = inMatrix.up;
		}
	}
	else if(lookAtAxis == 2)
	{
		finalAxis = inMatrix.right;
		if(rotationOrder == 1)
		{
			firstRotationAxis = inMatrix.forward;
			secondRotationAxis = inMatrix.up;
		}
		else
		{
			firstRotationAxis = inMatrix.up;
			secondRotationAxis = inMatrix.forward;
		}
	}
	else
	{
		finalAxis = inMatrix.up;
		if(rotationOrder == 1)
		{
			firstRotationAxis = inMatrix.forward;
			secondRotationAxis = inMatrix.right;
		}
		else
		{
			firstRotationAxis = inMatrix.right;
			secondRotationAxis = inMatrix.forward;
		}
	}

	Matrix3 output;
	output.forward = firstRotationAxis;
	output.right = secondRotationAxis;
	output.up = finalAxis;
	return output;
}
Quat RenderingTools::LookAt(Matrix3 matrix, Vector targetDirection, int lookAtAxis, int rotationOrder)
{
	/*
	lookAtAxis - final axis that will point at target
		rotationOrder - which axis will be rotated around first, then second

	1 = Forward
		1 = UP RIGHT
		2 = RIGHT UP
	2 = Right
		1 = FORWARD UP
		2 = UP FORWARD
	3 = Up
		1 = FORWARD RIGHT
		2 = RIGHT FORWARD
	*/

	matrix.normalize();

	matrix = SingleAxisAlignment(matrix, targetDirection, lookAtAxis, rotationOrder, 1);//Rotate around first axis
	matrix = SingleAxisAlignment(matrix, targetDirection, lookAtAxis, rotationOrder, 2);//Rotate around second axis

	return MatrixToQuat(matrix);
}
Quat RenderingTools::Slerp(Quat q1, Quat q2, float percent) 
{
	//https://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/slerp/index.htm
	Quat q;
	double dot = q1.W*q2.W + q1.X*q2.X + q1.Y*q2.Y + q1.Z*q2.Z;
	// Flip one quat to get the shortest distance
	if (dot < 0)
	{
		q2 = -q2;
		dot = -dot;
	}
	// if qa=qb or qa=-qb then theta = 0 and we can return qa
	if (abs(dot) >= 1.0)
	{
		q.W = q1.W;
		q.X = q1.X;
		q.Y = q1.Y;
		q.Z = q1.Z;
		return q;
	}
	// Calculate temporary values.
	double halfTheta = acos(dot);
	double sinHalfTheta = sqrt(1.0 - dot*dot);
	// if theta = 180 degrees then result is not fully defined
	// we could rotate around any axis normal to qa or qb
	if (fabs(sinHalfTheta) < 0.001)
	{
		q.W = q1.W*0.5 + q2.W*0.5;
		q.X = q1.X*0.5 + q2.X*0.5;
		q.Y = q1.Y*0.5 + q2.Y*0.5;
		q.Z = q1.Z*0.5 + q2.Z*0.5;
		return q;
	}
	double ratioA = sin((1 - percent) * halfTheta) / sinHalfTheta;
	double ratioB = sin(percent * halfTheta) / sinHalfTheta; 

	q.W = (q1.W*ratioA + q2.W*ratioB);
	q.X = (q1.X*ratioA + q2.X*ratioB);
	q.Y = (q1.Y*ratioA + q2.Y*ratioB);
	q.Z = (q1.Z*ratioA + q2.Z*ratioB);
	
	return q;
}

Quat RenderingTools::NormalizeQuat(Quat q)
{
	float mag = sqrt(q.X*q.X + q.Y*q.Y + q.Z*q.Z + q.W*q.W);
	q.X /= mag;
	q.Y /= mag;
	q.Z /= mag;

	return q;
}

//Projection and reflection
Vector RenderingTools::VectorProjection(Vector vec1, Vector vec2)
{
	float dot = Vector::dot(vec1, vec2);
	float vec2magnitude = vec2.magnitude();
	return (vec2 * dot/(vec2magnitude * vec2magnitude));
}
Vector RenderingTools::VectorRejection(Vector vec1, Vector vec2)
{
	return vec1 - VectorProjection(vec1, vec2);
}
Vector RenderingTools::VectorReflection(Vector incident, Vector normal)
{
	//From JsonV
	//taken from: https://en.wikipedia.org/wiki/Specular_reflection
	normal.normalize();
	float norm_dot = Vector::dot(incident, normal);
	if (norm_dot < 0)
	{
		normal = normal * -1;
		norm_dot *= -1.0;
	}
	norm_dot *= 2.0;
	Vector change = normal * norm_dot;
	Vector r_unit = change - incident;
	return r_unit;
}

//Geometric objects
Line RenderingTools::GetLineFromPoints(Vector lineBegin, Vector lineEnd)
{
	//Get parametric line from two points
	Line newLine;
	newLine.lineBegin = lineBegin;
	newLine.lineEnd = lineEnd;
	newLine.direction = lineEnd - lineBegin;
	return newLine;
}
Plane RenderingTools::GetPlaneFromTriangle(Triangle Tri)
{
	//Get the normal vector from the cross product of two edges of the triangle.
	//Get 'd' from negative normal dotted with known point on plane

	Vector normal = Vector::cross((Tri.vert1 - Tri.vert2), (Tri.vert3 - Tri.vert2));
	float d = Vector::dot(Vector{-normal.X, -normal.Y, -normal.Z}, Tri.vert2);

	float mag = normal.magnitude();
	Plane newPlane = {normal.X/mag, normal.Y/mag, normal.Z/mag, d/mag};
	return newPlane;
}
Frustum RenderingTools::CreateFrustum(CanvasWrapper canvas, CameraWrapper camera, float nearClip, float farClip)
{
	return CreateFrustum(canvas, RotatorToQuat(camera.GetRotation()), camera.GetLocation(), camera.GetFOV(), nearClip, farClip);
}
Frustum RenderingTools::CreateFrustum(CanvasWrapper canvas, Quat cameraQuat, Vector cameraLocation, float FOV, float nearClip, float farClip)
{
	//Generate the 6 planes of the viewing frustum from the 8 vertices of the frustum
	//https://www.lighthouse3d.com/tutorials/view-frustum-culling/geometric-approach-extracting-the-planes/

	int resX = canvas.GetSize().X;
	int resY = canvas.GetSize().Y;
	float aspectRatio = (float)resX/(float)resY;

	Quat fwdQuatBase = {0,1,0,0};
	Quat fwdQuat = (cameraQuat * fwdQuatBase) * cameraQuat.conjugate();
	Quat rightQuatBase = {0,0,1,0};
	Quat rightQuat = (cameraQuat * rightQuatBase) * cameraQuat.conjugate();
	Quat upQuatBase = {0,0,0,1};
	Quat upQuat = (cameraQuat * upQuatBase) * cameraQuat.conjugate();
	Vector camFwd = {fwdQuat.X, fwdQuat.Y, fwdQuat.Z};
	Vector camRight = {rightQuat.X, rightQuat.Y, rightQuat.Z};
	Vector camUp = {upQuat.X, upQuat.Y, upQuat.Z};
	camFwd.normalize(); camRight.normalize(); camUp.normalize();
	
	float farHeight = 2 * tan(FOV * 0.5f * (M_PI/180)) * farClip;
	float farWidth = farHeight * aspectRatio;
	float nearHeight = 2 * tan(FOV * 0.5f * (M_PI/180)) * nearClip;
	float nearWidth = nearHeight * aspectRatio;

	Vector farPlane = cameraLocation + camFwd * farClip;
	Vector nearPlane = cameraLocation + camFwd * nearClip;

	Vector upFarHeight = camUp * farHeight/2;
	Vector rightFarWidth = camRight * farWidth/2;
	Vector upNearHeight = camUp * nearHeight/2;
	Vector upNearWidth = camRight * nearWidth/2;

	#define FTL 0
	#define FTR 1
	#define FBR 2
	#define FBL 3
	#define NTL 4
	#define NTR 5
	#define NBR 6
	#define NBL 7

	Vector points[8];
	points[FTL] = farPlane + upFarHeight - rightFarWidth;
	points[FTR] = farPlane + upFarHeight + rightFarWidth;
	points[FBR] = farPlane - upFarHeight + rightFarWidth;
	points[FBL] = farPlane - upFarHeight - rightFarWidth;
	points[NTL] = nearPlane + upNearHeight - upNearWidth;
	points[NTR] = nearPlane + upNearHeight + upNearWidth;
	points[NBR] = nearPlane - upNearHeight + upNearWidth;
	points[NBL] = nearPlane - upNearHeight - upNearWidth;

	Plane planes[6];
	planes[0] = GetPlaneFromTriangle(Triangle{points[FTL],points[FTR],points[NTL]});//top
	planes[1] = GetPlaneFromTriangle(Triangle{points[FBR],points[FBL],points[NBR]});//bottom
	planes[2] = GetPlaneFromTriangle(Triangle{points[FTL],points[NTL],points[FBL]});//left
	planes[3] = GetPlaneFromTriangle(Triangle{points[NTR],points[FTR],points[NBR]});//right
	planes[4] = GetPlaneFromTriangle(Triangle{points[NTL],points[NTR],points[NBL]});//near
	planes[5] = GetPlaneFromTriangle(Triangle{points[FTR],points[FTL],points[FBR]});//far

	Frustum frustum;
	for(int i=0; i<6; i++)
		frustum.planes[i] = planes[i];
	for(int i=0; i<8; i++)
		frustum.points[i] = points[i];

	return frustum;
}
void RenderingTools::DrawCircle(CanvasWrapper canvas, Frustum frustum, Circle circle)
{
	vector<Vector> circlePoints;
	Vector start = {1,0,0};
	Vector axis = {0,0,1};
	float fullRot = 2 * M_PI;

	//Rename variables for easier readability
	float radius = circle.radius;
	Vector location = circle.location;
	Quat orientation = circle.orientation;
	float lineThickness = circle.lineThickness;
	int steps = circle.steps;
	float piePercentage = circle.piePercentage;

	//Get all the vertices that comprise the circle
	for(int i=0; i<steps; i++)
	{
		Vector newPoint = start;
		float angle = (fullRot/steps) * (float)i;
		Quat rotAmount = AngleAxisRotation(angle, axis);
		newPoint = RotateVectorWithQuat(newPoint, rotAmount);
		circlePoints.push_back(newPoint);
	}

	//Reorient all points of circle
	for(int i=0; i<circlePoints.size(); i++)
		circlePoints[i] = RotateVectorWithQuat(circlePoints[i], orientation);

	//Determine how many lines to draw
	int newPointAmount = ((float)circlePoints.size() * piePercentage);
	if(piePercentage != 0 && piePercentage != 1)
		newPointAmount += 1;
	
	//Calculate how much of the last line should be drawn
	float percentagePerLineSegment = 1/(float)steps;
	float fullLinePercentage = float(newPointAmount) * percentagePerLineSegment - percentagePerLineSegment;
	if(piePercentage == 0)
		fullLinePercentage = 0;
	if(piePercentage == 1)
		fullLinePercentage = 1;
	float lastLineRemainder = piePercentage - fullLinePercentage;
	float lastLinePercent = lastLineRemainder/percentagePerLineSegment;
	if(piePercentage == 1)
		lastLinePercent = 1;

	//Draw the line segments
	for(int i=0; i<newPointAmount; i++)
	{
		Vector startPoint;
		Vector originalEnd;
		Vector calculatedEnd;

		if(i < circlePoints.size()-1)
		{
			startPoint = location + circlePoints[i] * radius;
			originalEnd = location + circlePoints[i+1] * radius;
		}
		if(i == circlePoints.size()-1)
		{
			startPoint = location + circlePoints[i] * radius;
			originalEnd = location + circlePoints[0] * radius;
		}

		calculatedEnd = originalEnd;
		if(i == newPointAmount-1)
			calculatedEnd = ((originalEnd - startPoint) * lastLinePercent) + startPoint;

		if(IsInFrustum(frustum, startPoint, 0.f) && IsInFrustum(frustum, calculatedEnd, 0.f))
		{
			if(lineThickness == 1)
				canvas.DrawLine(canvas.Project(startPoint), canvas.Project(calculatedEnd));//avoid gaps between lines
			else
				canvas.DrawLine(canvas.Project(startPoint), canvas.Project(calculatedEnd), lineThickness);
		}
	}
}
void RenderingTools::DrawCube(CanvasWrapper canvas, Cube cube)
{
	float HS = cube.sideLength/2;

	Matrix3 matrix = QuatToMatrix(cube.orientation);
	Vector fwd = matrix.forward * HS;
	Vector right = matrix.right * HS;
	Vector up = matrix.up * HS;

	Vector2 points[8];
	points[0] = canvas.Project(cube.location + fwd + right + up);//front right top
	points[1] = canvas.Project(cube.location + fwd + right - up);//front right bottom
	points[2] = canvas.Project(cube.location + fwd - right - up);//front left bottom
	points[3] = canvas.Project(cube.location + fwd - right + up);//front left top
	points[4] = canvas.Project(cube.location - fwd + right + up);//back right top
	points[5] = canvas.Project(cube.location - fwd + right - up);//back right bottom
	points[6] = canvas.Project(cube.location - fwd - right - up);//back left bottom
	points[7] = canvas.Project(cube.location - fwd - right + up);//back left top

	for(int i=0; i<4; i++)
	{
		if(i == 3)
		{
			canvas.DrawLine(points[i], points[0], cube.lineThickness);
			canvas.DrawLine(points[i+4], points[4], cube.lineThickness);
			canvas.DrawLine(points[0], points[4], cube.lineThickness);
		}
		else
		{
			canvas.DrawLine(points[i], points[i+1], cube.lineThickness);
			canvas.DrawLine(points[i+4], points[i+5], cube.lineThickness);
			canvas.DrawLine(points[i+1], points[i+5], cube.lineThickness);
		}
	}
}

//Frustum cull
bool RenderingTools::IsInFrustum(Frustum frustum, Vector position, float radius)
{
	//Check if object is on positive side of all 6 planes of frustum
	for(int i=0; i<6; i++)
	{
		Vector planeNormal = {frustum.planes[i].x, frustum.planes[i].y, frustum.planes[i].z}; 
		if(Vector::dot(position, planeNormal) + frustum.planes[i].d + radius <= 0)
			return false;
	}
	return true;
}

//Line-Triangle intersection
bool RenderingTools::IsObscuredByObject(Vector drawingLocation, Vector cameraLocation, Vector objectLocation, float objectRadius)
{
	//Sphere based occlusion check.
	//If the distance from the center of the sphere
	//to the nearest point on the line created from the drawing to the camera
	//is less than the radius, the drawing is occluded

	Vector distanceToDrawing = drawingLocation - cameraLocation;
	Vector distanceToObject = objectLocation - cameraLocation;
	
	//if distance to the drawing is less than the distance to the object minus the object's radius, the object is behind the drawing
	if(distanceToDrawing.magnitude() < (distanceToObject.magnitude() - objectRadius))
		return false;

	Vector distanceToDrawingNormalized = distanceToDrawing / distanceToDrawing.magnitude();
	float dotVal = Vector::dot(distanceToObject, distanceToDrawingNormalized);
	Vector projectedLocation = cameraLocation + (Vector{distanceToDrawingNormalized.X * dotVal, distanceToDrawingNormalized.Y * dotVal, distanceToDrawingNormalized.Z * dotVal});
	if((projectedLocation - objectLocation).magnitude() < objectRadius)
		return true;
	else
		return false;
}
bool RenderingTools::LineTriangleIntersection(Line line, Triangle tri)
{
	//Check if line crosses through plane
	//If it does, check if the intersection point is within the line segment
	//If it is, check if the intersection point is within the barycentric coordinates of the triangle
	Plane plane = GetPlaneFromTriangle(tri);
	if(LinePlaneIntersection(line, plane))
	{
		Vector intersection = LinePlaneIntersectionPoint(line, plane);
		float intersectionSegmentLength = (intersection - line.lineBegin).magnitude();
		float lineSegmentLength = (line.lineEnd - line.lineBegin).magnitude();
		if(intersectionSegmentLength > lineSegmentLength)
			return false;
		else
		{
			if(IsWithinTriangleCoordinates(intersection, tri))
				return true;
			else
				return false;
		}
	}
	else
		return false;

	return true;
}
bool RenderingTools::LinePlaneIntersection(Line line, Plane plane)
{
	//if the line is parallel to the plane, it will not intersect
	if(Vector::dot(line.direction, Vector{plane.x, plane.y, plane.z}) != 0)
		return true;
	else
		return false;
}
Vector RenderingTools::LinePlaneIntersectionPoint(Line line, Plane plane)
{
	//if line crosses through plane, move on, else return false
	//if line segment is not long enough, return false, else return true
	

	//f = plane (4D in numerator, 3D in denominator)
	//p = lineStart (with additional 4D value of 1 in numerator)
	//v = direction (3D)
	//Vector q = p - (dot(f4D,p4D)/dot(f3D,v)) * v

	float dot4D = plane.x * line.lineBegin.X + plane.y * line.lineBegin.Y + plane.z * line.lineBegin.Z + plane.d;
	float dot3D = plane.x * line.direction.X + plane.y * line.direction.Y + plane.z * line.direction.Z;
	float t = dot4D/dot3D;
	Vector intersectionPoint = line.lineBegin - line.direction * t;
	return intersectionPoint;
}
bool RenderingTools::IsWithinTriangleCoordinates(Vector point, Triangle tri)
{
	//Found here: https://github.com/SebLague/Gamedev-Maths/blob/master/PointInTriangle.cs video: https://youtu.be/HYAgJN3x4GA
	//and here: https://github.com/1robertslattery/GeometricTestLibrary/blob/master/Source/GeometricTests/GeometricTestLibrary.h#L1607 video: https://youtu.be/bd_JwXYVc6c

	Vector A = tri.vert1;
	Vector B = tri.vert2;
	Vector C = tri.vert3;

	Plane plane = GetPlaneFromTriangle(tri);

	float w1 = -1;
	float w2 = -1;

	if(fabs(plane.x) >= fabs(plane.y) && fabs(plane.x) >= fabs(plane.z))
	{
		//discard X
		float s1 = C.Y - A.Y;
		float s2 = C.Z - A.Z;
		float s3 = B.Y - A.Y;
		float s4 = point.Y - A.Y;

		w1 = (A.Z*s1 + s4*s2 - point.Z*s1) / (s3*s2 - (B.Z - A.Z)*s1);
		w2 = (s4 - w1*s3) / s1;
	}
	else if(fabs(plane.y) >= fabs(plane.z))
	{
		//discard Y
		float s1 = C.Z - A.Z;
		float s2 = C.X - A.X;
		float s3 = B.Z - A.Z;
		float s4 = point.Z - A.Z;

		w1 = (A.X*s1 + s4*s2 - point.X*s1) / (s3*s2 - (B.X - A.X)*s1);
		w2 = (s4 - w1*s3) / s1;
	}
	else
	{
		//discard Z
		float s1 = C.Y - A.Y;
		float s2 = C.X - A.X;
		float s3 = B.Y - A.Y;
		float s4 = point.Y - A.Y;

		w1 = (A.X*s1 + s4*s2 - point.X*s1) / (s3*s2 - (B.X - A.X)*s1);
		w2 = (s4 - w1*s3) / s1;
	}
	
	return w1 >= 0 && w2 >= 0 && (w1 + w2) <= 1;
}

//Draw portions of line segments
int RenderingTools::LineCrossesFrustum(Line line, Frustum frustum, Vector &newLineBegin, Vector &newLineEnd)
{
	//return 0 if line is completely inside frustum
	//return 1 if line is completely outside frustum
	//return 2 if lineBegin is outside frustum
	//return 3 if lineEnd is outside frustum
	//return 4 if both are out of frustum but line still intersects

	int pointsOutsideFrustum = 0;
	int numIntersections = 0;
	bool beginInFrustum = false;
	bool endInFrustum = false;
	bool intersectsInFrustum[2] = {false, false};
	Vector intersects[2];

	if(IsInFrustum(frustum, line.lineBegin, 0.0f))
		beginInFrustum = true;
	else
		pointsOutsideFrustum++;
	if(IsInFrustum(frustum, line.lineEnd, 0.0f))
		endInFrustum = true;
	else
		pointsOutsideFrustum++;
	
	if(beginInFrustum && endInFrustum)
		return 0;//Line is completely inside frustum
	else
	{
		int intersectionIndex = 0;
		for(int i=0; i<6; i++)
		{
			if(LinePlaneIntersection(line, frustum.planes[i]))
			{
				Vector intersection = LinePlaneIntersectionPoint(line, frustum.planes[i]);
				if(IsInFrustum(frustum, intersection, 1.0f))
				{
					intersectsInFrustum[intersectionIndex] = true;
					intersects[intersectionIndex] = intersection;
					intersectionIndex++;
				}
			}
		}

		if(!intersectsInFrustum[0] && !intersectsInFrustum[1])
			return 1;//Line is completely outside frustum
		if(!beginInFrustum && endInFrustum)
		{
			if(IsWithinLineSegment(line, intersects[0], false))
				newLineBegin = Vector{intersects[0].X, intersects[0].Y, intersects[0].Z};
			else
				newLineBegin = Vector{intersects[1].X, intersects[1].Y, intersects[1].Z};
			return 2;//Only lineBegin is outside frustum and should be replaced by newLineBegin
		}
		if(beginInFrustum && !endInFrustum)
		{
			if(IsWithinLineSegment(line, intersects[0], true))
				newLineEnd = Vector{intersects[0].X, intersects[0].Y, intersects[0].Z};
			else
				newLineEnd = Vector{intersects[1].X, intersects[1].Y, intersects[1].Z};
			return 3;//Only lineEnd is outside frustum and should be replaced by newLineEnd
		}
		if(intersectsInFrustum[0] && intersectsInFrustum[1])
		{
			newLineBegin = Vector{intersects[0].X, intersects[0].Y, intersects[0].Z};
			newLineEnd = Vector{intersects[1].X, intersects[1].Y, intersects[1].Z};
			return 4;//Both lineBegin and lineEnd are both outside frustum and should be replaced by newLineBegin and newLineEnd
		}
	}
}
bool RenderingTools::IsWithinLineSegment(Line line, Vector point, bool beginOrEnd)
{
	//if beginOrEnd is true, calculate for lineBegin
	//if beginOrEnd is false, calculate for lineEnd

	if(beginOrEnd)
	{
		Vector endToBegin = line.lineEnd - line.lineBegin;
		Vector pointToBegin = point - line.lineBegin;

		float dot = Vector::dot(endToBegin, pointToBegin);
		if(dot > 0)
			return true;
		else
			return false;
	}
	else
	{
		Vector beginToEnd = line.lineBegin - line.lineEnd;
		Vector pointToEnd = point - line.lineEnd;

		float dot = Vector::dot(beginToEnd, pointToEnd);
		if(dot > 0)
			return true;
		else
			return false;
	}
}

//Miscellaneous canvas functions
CanvasColor RenderingTools::GetPercentageColor(float percent)
{
	char R, G, B=0, A=255;
	if(percent <= .5f)
	{
		R = 255;
		G = 0 + (255 * (percent*2));
	}
	else
	{
		R = 255 - (255 * (percent*2));
		G = 255;
	}

	return RenderingTools::CanvasColor{R,G,B,A};
}
float RenderingTools::GetVisualDistance(CanvasWrapper canvas, Frustum frustum, CameraWrapper camera, Vector objectLocation)
{
	//Concept: project a line of set 3D length to the 2D canvas.
	//Calculate the length of that 2D line to get perceived distance instead of linear distance

	Quat camQuat = RotatorToQuat(camera.GetRotation());
	Vector camUp = QuatToMatrix(camQuat).up;

	float testScalePerspectiveLineLength = 100;
	Vector2F perspScaleLineStartProjected = canvas.ProjectF(objectLocation);
	Vector2F perspScaleLineEndProjected;
	Vector testScalePerspective = objectLocation + (camUp * testScalePerspectiveLineLength);

	//check if scale projection is within the top plane of the frustum, if not then invert the scale test line
	Vector planeNormal = {frustum.planes[0].x, frustum.planes[0].y, frustum.planes[0].z}; 
	if(Vector::dot(testScalePerspective, planeNormal) + frustum.planes[0].d + 1.0f > 0)
		perspScaleLineEndProjected = canvas.ProjectF(objectLocation + (camUp * testScalePerspectiveLineLength));
	else
		perspScaleLineEndProjected = canvas.ProjectF(objectLocation - (camUp * testScalePerspectiveLineLength));

	Vector2F perspScaleLine = {perspScaleLineEndProjected.X - perspScaleLineStartProjected.X, perspScaleLineEndProjected.Y - perspScaleLineStartProjected.Y};
	float perspScale = sqrt(perspScaleLine.X * perspScaleLine.X + perspScaleLine.Y * perspScaleLine.Y);
	if(perspScale > 100)
		perspScale = 100;
	float distancePercentage = perspScale/100;//1 is close, 0 is infinitely far away
	if(distancePercentage > 1)
		distancePercentage = 1;
	if(distancePercentage < 0)
		distancePercentage = 0;

	return distancePercentage;
}
void RenderingTools::DrawMatrix(CanvasWrapper canvas, Matrix3 matrix, Vector location, float size)
{
	matrix.forward = (matrix.forward * size) + location;
	matrix.right = (matrix.right * size) + location;
	matrix.up = (matrix.up * size) + location;

	Vector2 root = canvas.Project(location);
	Vector2 X = canvas.Project(matrix.forward);
	Vector2 Y = canvas.Project(matrix.right);
	Vector2 Z = canvas.Project(matrix.up);

	float boxSizePercent = size/100;
	int boxSize = 10 * boxSizePercent;

	//Forward
	canvas.SetColor(255,0,0,255);
	canvas.DrawLine(root, X);
	canvas.SetPosition(X.minus({boxSize/2,boxSize/2}));
	canvas.FillBox(Vector2{boxSize,boxSize});

	//Right
	canvas.SetColor(0,255,0,255);
	canvas.DrawLine(root, Y);
	canvas.SetPosition(Y.minus({boxSize/2,boxSize/2}));
	canvas.FillBox(Vector2{boxSize,boxSize});

	//Up
	canvas.SetColor(0,0,255,255);
	canvas.DrawLine(root, Z);
	canvas.SetPosition(Z.minus({boxSize/2,boxSize/2}));
	canvas.FillBox(Vector2{boxSize,boxSize});

	//Root
	canvas.SetColor(255,255,255,255);
	canvas.SetPosition(root.minus({boxSize/2,boxSize/2}));
	canvas.FillBox(Vector2{boxSize,boxSize});
}

Quat operator*(float lhs, const Quat rhs)
{
	return { lhs * rhs.X, lhs * rhs.Y , lhs * rhs.Z , lhs * rhs.W };
}

Quat operator+(const Quat lhs, const Quat rhs)
{
	return {lhs.X + rhs.X, lhs.Y + rhs.Y, lhs.Z + rhs.Z, lhs.W + rhs.W};
}

Quat operator-(const Quat lhs, const Quat rhs)
{
	return { lhs.X - rhs.X, lhs.Y - rhs.Y, lhs.Z - rhs.Z, lhs.W - rhs.W };
}

Quat operator-(const Quat& obj)
{
	return { -obj.X, -obj.Y, -obj.Z, -obj.W };
}
