#pragma once

#include "CValue.h"
#include "ObjectInstance.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <cmath>

#include "Animations.h"
#include "AlterableValues.h"
#include "AlterableStrings.h"
#include "AlterableFlags.h"
#include "Movements.h"

class Active : public ObjectInstance {
public:
	Active(unsigned int objectInfoHandle, int type, std::string name)
		: ObjectInstance(objectInfoHandle, type, name) {}
	~Active() override { movements.Deinitialize(); }

	Animations animations;
	AlterableValues Values;
	AlterableStrings Strings;
	AlterableFlags Flags;
	Movements movements;

	bool Visible = true;
	bool AutomaticRotation = false;
	bool FineDetection = false;

	std::vector<unsigned int> GetImagesUsed() override {
		return animations.GetImagesUsed();
	}

	ObjectGlobalData* CreateGlobalData() override {
		ObjectGlobalData* globalData = new ObjectGlobalData(ObjectInfoHandle);
		
		globalData->flags = Flags;
		globalData->values = Values;
		globalData->strings = Strings;

		return globalData;
	}

	void ApplyGlobalData(ObjectGlobalData* globalData) override {
		Flags = globalData->flags;
		Values = globalData->values;
		Strings = globalData->strings;
	}

	CValue GetXActionPoint() const {
		return GetX() + animations.GetXActionPoint() - animations.GetXHotspot();
	}

	CValue GetYActionPoint() const {
		return GetY() + animations.GetYActionPoint() - animations.GetYHotspot();
	}

	CValue GetWidth() const {
		return CValue(animations.GetWidth());
	}

	CValue GetHeight() const {
		return CValue(animations.GetHeight());
	}

	CValue GetXScale() const {
		return xScale;
	}

	CValue GetYScale() const {
		return yScale;
	}

	CValue GetXLeft() const {
		return GetX() + Edge(false, false);
	}

	CValue GetXRight() const {
		return GetX() + Edge(false, true);
	}

	CValue GetYTop() const {
		return GetY() + Edge(true, false);
	}

	CValue GetYBottom() const {
		return GetY() + Edge(true, true);
	}
	
	void SetXScale(const CValue& xScale) {
		if (xScale.GetDoubleValue() == this->xScale) return;
		this->xScale = (float)xScale.GetDoubleValue();
		collisionBoundsDirty = true;
	}
	void SetYScale(const CValue& yScale) {
		if (yScale.GetDoubleValue() == this->yScale) return;
		this->yScale = (float)yScale.GetDoubleValue();
		collisionBoundsDirty = true;
	}

private:
	int Edge(bool vertical, bool max) const {
		float a = GetAngle() * (3.14159265358979f / 180.0f);
		float x0 = -animations.GetXHotspot() * xScale, x1 = x0 + animations.GetWidth() * xScale;
		float y0 = -animations.GetYHotspot() * yScale, y1 = y0 + animations.GetHeight() * yScale;
		float p = vertical ? -std::sin(a) : std::cos(a);
		float q = vertical ? std::cos(a) : std::sin(a);
		float e = max ? std::max(x0 * p, x1 * p) + std::max(y0 * q, y1 * q)
		              : std::min(x0 * p, x1 * p) + std::min(y0 * q, y1 * q);
		return (int)std::lround(e);
	}

	float xScale = 1.0f;
	float yScale = 1.0f;
};

 