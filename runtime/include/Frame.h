#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>
#include <vector>

#include "CounterBase.h"
#include "Layer.h"
#include "ObjectFactory.h"
#include "ObjectInstance.h"
#include "ObjectSelector.h"
#include "Timer.h"

class Frame {
public:
	Frame() = default;
    virtual ~Frame() = default;

	int Index = -1;
	std::string Name = "";

	int Width = 0;
	int Height = 0;

	int BackgroundColor = 0;

	std::vector<Layer> Layers;

	std::unordered_map<unsigned int, ObjectInstance*> ObjectInstances;
	unsigned int MaxObjectInstanceHandle = 0;
	
	Timer GameTimer;

	bool IsGroupActive(unsigned int groupId) {
		if (groupId < ActiveGroups.size()) {
			return ActiveGroups[groupId];
		}
		return false;
	}

	void SetGroupActive(unsigned int groupId, bool active) {
		if (groupId >= ActiveGroups.size()) {
			ActiveGroups.resize(groupId + 1, false);
		}
		ActiveGroups[groupId] = active;
	}

	virtual void Initialize();
	void PostInitialize();
	virtual void Update();
	virtual void Draw();

	void SetScroll(int x, int y, int layer = -1);
	void SetScrollX(int x);
	void SetScrollY(int y);
	int GetXLeftEdge();
	int GetXRightEdge();
	int GetYTopEdge();
	int GetYBottomEdge();

	//mark an instance for deletion
	void MarkForDeletion(ObjectInstance* instance) {
		if (instance) {
			instancesMarkedForDeletion.push_back(instance->Handle);
		}
	}

	void DeleteMarkedInstances() {
		for (auto& handle : instancesMarkedForDeletion) {
			for (auto& layer : Layers) {
				auto it = std::find_if(layer.instances.begin(), layer.instances.end(), [handle](ObjectInstance* instance) {
					return instance->Handle == handle;
				});
				if (it != layer.instances.end()) {
					layer.instances.erase(it);
					break;
				}
			}
			ObjectInstances.erase(handle);
		}
		instancesMarkedForDeletion.clear();
	}

	void DrawLayer(Layer& layer);
	void DrawCounterNumbers(CounterBase *counter, int value, int x, int y);

	std::vector<unsigned int> GetImagesUsed();
	std::vector<unsigned int> GetFontsUsed();

	ObjectInstance* CreateInstance(ObjectInstance* createdInstance, short x, short y, unsigned int layer, short instanceValue, unsigned int objectInfoHandle, short angle, ObjectInstance* parentInstance = nullptr);

	std::vector<ObjectGlobalData*> GetGlobalObjectData();
	void ApplyGlobalObjectData(std::vector<ObjectGlobalData*> globalData);

	void MoveObjectToLayer(ObjectInstance* instance, unsigned int layer);
	void MoveObjectToFront(ObjectInstance* instance);
	void MoveObjectToBack(ObjectInstance* instance);
	void MoveObjectInFrontOf(ObjectInstance* instance, unsigned int oiHandle);
	void MoveObjectBehindOf(ObjectInstance* instance, unsigned int oiHandle);

	int GetMouseX();
	int GetMouseY();

	inline int StringLength(std::string str) {
		return (int)str.length();
	}

	inline std::string StringLeft(std::string str, int length) {
		return str.substr(0, length);
	}

	inline std::string StringRight(std::string str, int length) {
		return str.substr(str.length() - length);
	}

    inline std::string Hex(int v) {
        char buf[16];
        sprintf_s(buf, "%X", v);
        return std::string(buf);
    }

    inline std::string Bin(int v) {
        std::string r;
        for (int i = 31; i >= 0; i--) {
            if (v & (1 << i)) r += '1';
            else if (!r.empty()) r += '0';
        }
        return r.empty() ? "0" : r;
    }

    inline std::string Mid(const std::string& str, int start, int length) {
        if (start < 0) start = 0;
        if (start >= (int)str.length()) return "";
        return str.substr(start, length);
    }

    inline std::string Lower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }

    inline std::string Upper(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
    }

    inline int Find(const std::string& str, const std::string& find, int start) {
        if (start < 0) start = 0;
        auto pos = str.find(find, start);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }

    inline int ReverseFind(const std::string& str, const std::string& find, int start) {
        auto pos = str.rfind(find, (start < 0 || start >= str.length()) ? std::string::npos : start);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }

    inline std::string ReplaceString(std::string str, const std::string& find, const std::string& replace) {
        if (find.empty()) return str;
        size_t pos = 0;
        while ((pos = str.find(find, pos)) != std::string::npos) {
            str.replace(pos, find.length(), replace);
            pos += replace.length();
        }
        return str;
    }

    inline std::string NewLine() { return "\n"; }
	int OAngle(ObjectInstance* instance, int xTarget, int yTarget) {
		int distanceX  = xTarget - instance->X;
		int distanceY  = yTarget - instance->Y;
		int angle = static_cast<int>(atan2(-distanceY, distanceX) * 180 / 3.14159265358979323846);
		angle = (angle + 360) % 360;
		return angle;
	}

	int OAngle(std::shared_ptr<ObjectSelector> selector, int xTarget, int yTarget) {
		if (!selector || selector->Count() == 0) {
			return 0;
		}
		return OAngle(*(selector->begin()), xTarget, yTarget);
	}

	int ODistance(ObjectInstance* instance, int xTarget, int yTarget) {
		int distanceX = xTarget - instance->X;
		int distanceY = yTarget - instance->Y;
		return static_cast<int>(sqrt(distanceX * distanceX + distanceY * distanceY));
	}

	int ODistance(std::shared_ptr<ObjectSelector> selector, int xTarget, int yTarget) {
		if (!selector || selector->Count() == 0) {
			return 0;
		}
		return ODistance(*(selector->begin()), xTarget, yTarget);
	}

	struct LoopState {
		bool running = false;
		int index = 0;
	};

	static std::string ToLowerStr(const std::string& str) {
		std::string result = str;
		std::transform(result.begin(), result.end(), result.begin(),
			[](unsigned char c) { return std::tolower(c); });
		return result;
	}

	static bool LoopNameEquals(const std::string& a, const std::string& b) {
		if (a.size() != b.size()) return false;
		for (size_t i = 0; i < a.size(); i++) {
			if (std::tolower(static_cast<unsigned char>(a[i])) !=
				std::tolower(static_cast<unsigned char>(b[i])))
				return false;
		}
		return true;
	}

	void StartLoop(const std::string& name, int count) {
		std::string key = ToLowerStr(name);
		activeLoops[key] = {true, 0};
		while (activeLoops[key].running && activeLoops[key].index < count) {
			OnLoop(name);
			if (!activeLoops[key].running) break;
			activeLoops[key].index++;
		}
		activeLoops.erase(key);
	}

	void StopLoop(const std::string& name) {
		std::string key = ToLowerStr(name);
		auto it = activeLoops.find(key);
		if (it != activeLoops.end()) {
			it->second.running = false;
		}
	}

	int Loopindex(const std::string& loopName) {
		std::string key = ToLowerStr(loopName);
		auto it = activeLoops.find(key);
		if (it != activeLoops.end()) {
			return it->second.index;
		}
		return 0;
	}

	virtual void OnLoop(const std::string& loopName) {}

	//Collision detection
	bool IsCollidingWithBackground(ObjectInstance* instance);
	bool IsColliding(ObjectInstance* instance1, ObjectInstance* instance2);
	bool IsColliding(ObjectInstance* instance, int x, int y);

	void ClearBoundsCache();

private:
	std::vector<unsigned int> instancesMarkedForDeletion;
	std::vector<bool> ActiveGroups;
	std::unordered_map<std::string, LoopState> activeLoops;

	int scrollX = 0;
	int scrollY = 0;
};
