#pragma once
#include "interpstrategy.h"
class LinearInterpStrategy : public InterpStrategy
{
public:
	LinearInterpStrategy(std::shared_ptr<savetype> _camPath, int degree);
	virtual NewPOV GetPOV(float latestFrame);
	virtual std::string GetName();
};