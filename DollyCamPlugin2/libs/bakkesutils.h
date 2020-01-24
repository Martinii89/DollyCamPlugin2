#pragma once

struct CameraMovement
{
	int Forward = 0;
	int Strafe = 0;
	int Up = 0;

	int Turn = 0;
	int LookUp = 0;
};

void LockCamera(void* _gamewrapper, void* _playerInput);
void MoveCamera(void* _gamewrapper, void* _playerInput, CameraMovement movement);