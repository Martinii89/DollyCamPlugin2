#include "bakkesmod\wrappers\wrapperstructs.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"

namespace RenderingTools
{
	//STRUCTS
	//Geometric Structs
	struct Line
	{
		Vector lineBegin, lineEnd, direction;
	};
	struct Triangle
	{
		Vector vert1, vert2, vert3;
	};
	struct Plane
	{
		float x, y, z, d;
	};
	struct Circle
	{
		float radius;
		Vector location;
		Quat orientation;
		float lineThickness = 1.f;
		int steps = 16;
		float piePercentage = 1.f;
	};
	struct Cube
	{
		float sideLength=100;
		Vector location;
		Quat orientation;
		float lineThickness=1;
	};
	struct Frustum
	{
		Plane planes[6];//top, bottom, left, right, near, far
		Vector points[8];//FTL, FTR, FBR, FBL, NTL, NTR, NBR, NBL
	};
	struct Matrix3
	{
		Vector forward = {1,0,0};
		Vector right = {0,1,0};
		Vector up = {0,0,1};

		inline const static Matrix3 identity()
		{
			return Matrix3{ {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
		}

		inline void normalize()
		{
			forward.normalize();
			right.normalize();
			up.normalize();
		}
	};

	//Canvas structs
	struct CanvasColor
	{
		char R, G, B, A=255;
	};


	//FUNCTIONS
	//Quat conversions
	Rotator QuatToRotator(Quat q);
	Quat RotatorToQuat(Rotator rot);
	Matrix3 QuatToMatrix(Quat quat);
	Quat MatrixToQuat(Matrix3 matrix);

	//Rotations
	Quat AngleAxisRotation(float angle, Vector axis);
	Vector RotateVectorWithQuat(Vector v, Quat q, bool normalize=false);
	Matrix3 RotateMatrixWithQuat(Matrix3, Quat q, bool normalize=false);
	Matrix3 SingleAxisAlignment(Matrix3 matrix, Vector targetDirection, int lookAtAxis, int rotationOrder, int step);
	Matrix3 GetRotationOrder(Matrix3 inMatrix, int lookAtAxis, int rotationOrder);
	Quat LookAt(Matrix3 matrix, Vector targetDirection, int lookAtAxis, int rotationOrder);
	Quat Slerp(Quat q1, Quat q2, float percent);
	Quat NormalizeQuat(Quat q);

	enum LookAtAxis
	{
		AXIS_FORWARD = 1,
		AXIS_RIGHT = 2,
		AXIS_UP = 3
	};

	//Projection and reflection
	Vector VectorProjection(Vector vec1, Vector vec2);
	Vector VectorRejection(Vector vec1, Vector vec2);
	Vector VectorReflection(Vector incident, Vector normal);

	//Geometric objects
	Line GetLineFromPoints(Vector lineBegin, Vector lineEnd);
	Plane GetPlaneFromTriangle(Triangle tri);
	Frustum CreateFrustum(CanvasWrapper canvas, CameraWrapper camera, float nearClip=50, float farClip=20000);
	Frustum CreateFrustum(CanvasWrapper canvas, Quat cameraQuat, Vector cameraLocation, float FOV, float nearClip=50, float farClip=20000);
	void DrawCircle(CanvasWrapper canvas, Frustum frustum, Circle circle);
	void DrawCube(CanvasWrapper canvas, Cube cube);
	
	//Frustum cull
	bool IsInFrustum(Frustum frustum, Vector position, float radius);
	
	//Line-Triangle intersection
	bool IsObscuredByObject(Vector drawingLocation, Vector cameraLocation, Vector objectLocation, float objectRadius);
	bool LineTriangleIntersection(Line line, Triangle tri);
	bool LinePlaneIntersection(Line line, Plane plane);
	Vector LinePlaneIntersectionPoint(Line line, Plane plane);
	bool IsWithinTriangleCoordinates(Vector point, Triangle tri);
	
	//Draw portions of line segments
	int LineCrossesFrustum(Line line, Frustum frustum, Vector &newLineBegin, Vector &newLineEnd);
	bool IsWithinLineSegment(Line line, Vector point, bool beginOrEnd);

	//Miscellaneous canvas functions
	CanvasColor GetPercentageColor(float percent);
	float GetVisualDistance(CanvasWrapper canvas, Frustum frustum, CameraWrapper camera, Vector objectLocation);
	void DrawMatrix(CanvasWrapper canvas, Matrix3 matrix, Vector location, float size=100);
}