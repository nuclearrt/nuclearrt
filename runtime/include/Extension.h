#pragma once

#include "ObjectInstance.h"
#include <vector>
#include <memory>

class Extension : public ObjectInstance {
public:
	Extension(unsigned int objectInfoHandle, int type, std::string name)
		: ObjectInstance(objectInfoHandle, type, name) {}
	virtual ~Extension() = default;

	virtual void Initialize() {}
	virtual void Update(float deltaTime) {}
	virtual void Draw() {}

	bool Visible = true;
};

 