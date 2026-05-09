#pragma once

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

	Animations animations;
	AlterableValues Values;
	AlterableStrings Strings;
	AlterableFlags Flags;
	Movements movements;

	bool Visible = true;
	bool FollowFrame = false;
	bool AutomaticRotation = false;
	bool FineDetection = false;

	float xScale = 1.0f;
	float yScale = 1.0f;

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

	int GetXActionPoint() const {
		return X + animations.GetXActionPoint() - animations.GetXHotspot();
	}

	int GetYActionPoint() const {
		return Y + animations.GetYActionPoint() - animations.GetYHotspot();
	}
};

 