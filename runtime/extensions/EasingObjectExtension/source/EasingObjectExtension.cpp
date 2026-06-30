#include "EasingObjectExtension.h"

#include <cmath>
#include <algorithm>

#include "Frame.h"
#include "PlatformBackend.h"

namespace {
	#define PI 3.14159265358979323846f
	
	double linear(double step, EaseVars vars) { return step; }
	double quad(double step, EaseVars vars) { return pow(step, 2.0); }
	double cubic(double step, EaseVars vars) { return pow(step, 3.0); }
	double quart(double step, EaseVars vars) { return pow(step, 4.0); }
	double quint(double step, EaseVars vars) { return pow(step, 5.0); }
	double sine(double step, EaseVars vars) { return 1.0 - sin((1 - step) * 90.0 * PI / 180.0); }
	double expo(double step, EaseVars vars) { return pow(2.0, step * 10.0) / 1024.0; }
	double circ(double step, EaseVars vars) { return 1.0f - sqrt(1.0 - pow(step, 2.0)); }
	double back(double step, EaseVars vars) { return (vars.overshoot + 1.0) * pow(step, 3.0) - vars.overshoot * pow(step, 2.0); }
	double elastic(double step, EaseVars vars)
	{
		step -= 1.0;
		float amp = (float)std::max(1.0f, (float)vars.amplitude);
		float s = (float)(vars.period / (2.0 * PI) * asin(1.0 / amp));
		return -(amp * pow(2.0, 10 * step) * sin((step - s) * (2 * PI) / vars.period));
	}
	double bounce(double step, EaseVars vars)
	{
		step = 1 - step;
		if (step < (8 / 22.0))
			return 1 - 7.5625 * step * step;
		else if (step < (16 / 22.0))
		{
			step -= 12 / 22.0;
			return 1 - vars.amplitude * (7.5625 * step * step + 0.75) - (1 - vars.amplitude);
		}
		else if (step < (20 / 22.0))
		{
			step -= 18 / 22.0;
			return 1 - vars.amplitude * (7.5625 * step * step + 0.9375) - (1 - vars.amplitude);
		}
		else
		{
			step -= 21 / 22.0;
			return 1 - vars.amplitude * (7.5625 * step * step + 0.984375) - (1 - vars.amplitude);
		}
	}

	double doFunction(int number, double step, EaseVars vars)
	{
		switch (number)
		{
			default:
			case 0:
				return linear(step, vars);
			case 1:
				return quad(step, vars);
			case 2:
				return cubic(step, vars);
			case 3:
				return quart(step, vars);
			case 4:
				return quint(step, vars);
			case 5:
				return sine(step, vars);
			case 6:
				return expo(step, vars);
			case 7:
				return circ(step, vars);
			case 8:
				return back(step, vars);
			case 9:
				return elastic(step, vars);
			case 10:
				return bounce(step, vars);
		}
	}

	double easeIn(int function, double step, EaseVars vars)
	{
		return doFunction(function, step, vars);
	}

	double easeOut(int function, double step, EaseVars vars)
	{
		return 1.0 - doFunction(function, 1.0 - step, vars);
	}

	double easeInOut(int functionA, int functionB, double step, EaseVars vars)
	{
		if (step < 0.5)
			return easeIn(functionA, step * 2.0, vars) / 2.0;
		else
			return easeOut(functionB, (step - 0.5) * 2.0, vars) / 2.0 + 0.5;
	}

	double easeOutIn(int functionA, int functionB, double step, EaseVars vars)
	{
		if (step < 0.5)
			return easeOut(functionA, step * 2.0, vars) / 2.0;
		else
			return easeIn(functionB, (step - 0.5) * 2.0, vars) / 2.0 + 0.5;
	}

	long GetCurrentTime()
	{
		return static_cast<long>(Application::Instance().GetBackend()->platform->GetTicks());
	}
}

void EasingObjectExtension::Update(float deltaTime)
{
	int fixedvalue = 0;
	bool finishedMoving = false;
	float step;

	for (unsigned int i = 0; i < controlled.size(); i++)
	{
		fixedvalue = 0;
		ObjectInstance* object = nullptr;
		MoveStruct &moved = controlled.at(i);

		if (i >= 0 && i < controlled.size())
		{
			fixedvalue = moved.fixedValue;
			object = Application::Instance().GetCurrentFrame().get()->GetInstanceByFixedValue(fixedvalue);
		}

		if (object != nullptr)
		{
			if (moved.pausecount != 0)
				continue;
				
			if (moved.timeMode == 0)
			{
				long currentTime = GetCurrentTime();
				long diff = currentTime - moved.starttime;

				step = diff / (float)moved.timespan;

				if (diff >= moved.timespan)
					finishedMoving = true;
			}
			else
			{
				moved.eventloop_step++;
				step = moved.eventloop_step / (float)moved.timespan;

				if (moved.eventloop_step >= moved.timespan)
					finishedMoving = true;
			}

			float easeStep = (float)CalculateEasingValue(moved.easingMode, moved.functionA, moved.functionB, step, moved.vars);

			object->SetX((int)(moved.startX + (moved.destX - moved.startX) * easeStep + 0.5f));
			object->SetY((int)(moved.startY + (moved.destY - moved.startY) * easeStep + 0.5f));

			if (finishedMoving)
			{
				finishedMoving = false;

				object->SetX(moved.destX);
				object->SetY(moved.destY);

				controlled.erase(controlled.begin() + i);
				i--;
			}
		}
		else
		{
			controlled.erase(controlled.begin() + i);
			i--;
		}
	}
}

void EasingObjectExtension::MoveObject(ObjectInstance *instance, uint8_t easingMethod, uint8_t firstFunction, uint8_t secondFunction, int x, int y, uint8_t timeMode, int time)
{
	if (instance == nullptr)
		return;

	//Remove object from controlled list if it is already there
	controlled.erase(std::remove_if(controlled.begin(), controlled.end(), [instance](const MoveStruct& m) {
		return m.fixedValue == instance->FixedValue;
	}), controlled.end());

	MoveStruct move;
	move.startX = instance->GetX();
	move.startY = instance->GetY();
	move.fixedValue = instance->FixedValue;
	move.destX = x;
	move.destY = y;
	move.starttime = 0;
	move.easingMode = easingMethod;
	move.functionA = firstFunction;
	move.functionB = secondFunction;
	move.timeMode = timeMode;
	move.timespan = time;
	move.eventloop_step = 0;
	move.pausetime = 0;
	move.pausecount = 0;

	if (timeMode == 0) {
		move.starttime = GetCurrentTime();
	}

	//set to defaults
	move.vars.overshoot = Overshoot;
	move.vars.amplitude = Amplitude;
	move.vars.period = Period;

	controlled.push_back(move);
}

void EasingObjectExtension::MoveObjectExplicit(uint32_t fixedValue, uint8_t easingMethod, uint8_t firstFunction, uint8_t secondFunction, int x, int y, uint8_t timeMode, int time)
{
	ObjectInstance* instance = Application::Instance().GetCurrentFrame().get()->GetInstanceByFixedValue(fixedValue);
	MoveObject(instance, easingMethod, firstFunction, secondFunction, x, y, timeMode, time);
}

double EasingObjectExtension::CalculateEasingValue(int mode, int functionA, int functionB, double step, EaseVars vars)
{
	switch (mode)
	{
		default:
		case 0:
			return easeIn(functionA, step, vars);
		case 1:
			return easeOut(functionA, step, vars);
		case 2:
			return easeInOut(functionA, functionB, step, vars);
		case 3:
			return easeOutIn(functionA, functionB, step, vars);
	}
}