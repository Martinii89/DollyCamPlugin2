#include "gameapplier.h"
#include "bakkesmod\wrappers\GameObject\CameraWrapper.h"
#include <iostream>
#include <fstream>


RealGameApplier::RealGameApplier(std::shared_ptr<GameWrapper> gw) : gameWrapper(gw)
{
}

void RealGameApplier::SetPOV(Vector location, Rotator rotation, float FOV)
{
	gameWrapper->GetCamera().SetPOV({ location, rotation, FOV });

}

NewPOV RealGameApplier::GetPOV()
{
	CameraWrapper camera = gameWrapper->GetCamera();
	return{ camera.GetLocation(), camera.GetRotation(), camera.GetFOV() };
}


MockGameApplier::MockGameApplier(string filename)
{
	output.open(filename, ios::out | ios::trunc);
}

MockGameApplier::~MockGameApplier()
{
	output.flush();
	output.close();
}

void MockGameApplier::SetTime(float t)
{
	time = t;
}

void MockGameApplier::SetPOV(Vector location, Rotator rotation, float FOV)
{
	NewPOV newpov = { location, rotation, FOV };
	pov = newpov;
	output << time << "," << location.X << "," << location.Y << "," << location.Z << endl;
}

NewPOV MockGameApplier::GetPOV()
{
	return pov;
}