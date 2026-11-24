#pragma once

#include "CValue.h"
#include "ObjectInstance.h"
#include <vector>
#include <memory>

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
		return GetX() - animations.GetXHotspot();
	}

	CValue GetXRight() const {
		return GetX() - animations.GetXHotspot() + animations.GetWidth();
	}

	CValue GetYTop() const {
		return GetY() - animations.GetYHotspot();
	}

	CValue GetYBottom() const {
		return GetY() - animations.GetYHotspot() + animations.GetHeight();
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
	float xScale = 1.0f;
	float yScale = 1.0f;
};

 