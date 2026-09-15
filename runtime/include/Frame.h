#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>
#include <vector>

#include "CValue.h"
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

	void SetScroll(const CValue& x, const CValue& y, int layer = -1);
	void SetScrollX(const CValue& x);
	void SetScrollY(const CValue& y);
	CValue GetXLeftEdge();
	CValue GetXRightEdge();
	CValue GetYTopEdge();
	CValue GetYBottomEdge();

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

	ObjectInstance* CreateInstance(ObjectInstance* createdInstance, short x, short y, unsigned int layer, short instanceValue, unsigned int objectInfoHandle, short angle, bool postInitialize = false, ObjectInstance* parentInstance = nullptr);

	ObjectInstance* GetInstanceByFixedValue(unsigned int fixedValue) {
		for (auto& [handle, instance] : ObjectInstances) {
			if (instance->FixedValue == fixedValue) {
				return instance;
			}
		}
		return nullptr;
	}

	std::vector<ObjectGlobalData*> GetGlobalObjectData();
	void ApplyGlobalObjectData(std::vector<ObjectGlobalData*> globalData);

	void MoveObjectToLayer(ObjectInstance* instance, unsigned int layer);
	void MoveObjectToFront(ObjectInstance* instance);
	void MoveObjectToBack(ObjectInstance* instance);
	void MoveObjectInFrontOf(ObjectInstance* instance, unsigned int oiHandle);
	void MoveObjectBehindOf(ObjectInstance* instance, unsigned int oiHandle);

	int GetMouseX();
	int GetMouseY();

	int GetRGB(int red, int green, int blue) {
		return 0xFF000000 | (red << 16) | (green << 8) | blue;
	}

	inline CValue StringLength(const CValue& str) {
		return CValue((int)str.GetStringValue().length());
	}

	inline CValue StringLeft(const CValue& str, const CValue& length) {
		const std::string& s = str.GetStringValue();
		int n = length.GetIntValue();
		if (n <= 0)
			return CValue("");
		if (n >= (int)s.size())
			return CValue(s);
		return CValue(s.substr(0, n));
	}

	inline CValue StringRight(const CValue& str, const CValue& length) {
		const std::string& s = str.GetStringValue();
		int n = length.GetIntValue();
		if (n <= 0)
			return CValue("");
		if (n >= (int)s.size())
			return CValue(s);
		return CValue(s.substr(s.size() - n));
	}

    inline CValue Hex(const CValue& v) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%X", v.GetIntValue());
        return CValue(buf);
    }

    inline std::string Bin(int v) {
        std::string r;
        for (int i = 31; i >= 0; i--) {
            if (v & (1 << i)) r += '1';
            else if (!r.empty()) r += '0';
        }
        return r.empty() ? "0" : r;
    }

    inline CValue Bin(const CValue& v) {
        return CValue(Bin(v.GetIntValue()));
    }

    inline std::string Mid(const std::string& str, int start, int length) {
        if (start < 0) start = 0;
        if (start >= (int)str.length()) return "";
        return str.substr(start, length);
    }

    inline CValue Mid(const CValue& str, const CValue& start, const CValue& length) {
        return CValue(Mid(str.GetStringValue(), start.GetIntValue(), length.GetIntValue()));
    }

    inline std::string Lower(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }

    inline CValue Lower(const CValue& value) {
        return CValue(Lower(value.GetStringValue()));
    }

    inline std::string Upper(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
    }

    inline CValue Upper(const CValue& value) {
        return CValue(Upper(value.GetStringValue()));
    }

    inline int Find(const std::string& str, const std::string& find, int start) {
        if (start < 0) start = 0;
        auto pos = str.find(find, start);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }

    inline CValue Find(const CValue& str, const CValue& find, const CValue& start) {
        return CValue(Find(str.GetStringValue(), find.GetStringValue(), start.GetIntValue()));
    }

    inline int ReverseFind(const std::string& str, const std::string& find, int start) {
        auto pos = str.rfind(find, (start < 0 || start >= str.length()) ? std::string::npos : start);
        return (pos == std::string::npos) ? -1 : (int)pos;
    }

    inline CValue ReverseFind(const CValue& str, const CValue& find, const CValue& start) {
        return CValue(ReverseFind(str.GetStringValue(), find.GetStringValue(), start.GetIntValue()));
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

	CValue ReplaceString(const CValue& str, const CValue& find, const CValue& replace) {
		return CValue(ReplaceString(str.GetStringValue(), find.GetStringValue(), replace.GetStringValue()));
	}

    inline CValue NewLine() { return CValue("\n"); }
	CValue OAngle(ObjectInstance* instance, const CValue& xTarget, const CValue& yTarget) {
		int distanceX  = xTarget.GetIntValue() - instance->GetX().GetIntValue();
		int distanceY  = yTarget.GetIntValue() - instance->GetY().GetIntValue();
		int angle = static_cast<int>(atan2(-distanceY, distanceX) * 180 / 3.14159265358979323846);
		angle = (angle + 360) % 360;
		return CValue(angle);
	}

	CValue OAngle(ObjectSelector& selector, const CValue& xTarget, const CValue& yTarget) {
		if (selector.Count() == 0) {
			return CValue(0);
		}
		return OAngle(*(selector.begin()), xTarget, yTarget);
	}

	CValue ODistance(ObjectInstance* instance, const CValue& xTarget, const CValue& yTarget) {
		int distanceX = xTarget.GetIntValue() - instance->GetX().GetIntValue();
		int distanceY = yTarget.GetIntValue() - instance->GetY().GetIntValue();
		return CValue(static_cast<int>(sqrt(distanceX * distanceX + distanceY * distanceY)));
	}

	CValue ODistance(ObjectSelector& selector, const CValue& xTarget, const CValue& yTarget) {
		if (selector.Count() == 0) {
			return CValue(0);
		}
		return ODistance(*(selector.begin()), xTarget, yTarget);
	}

	CValue GetAlterableValueByIndex(ObjectInstance* instance, int index) {
		if (instance->Type == 2) {
			return static_cast<Active*>(instance)->Values.GetValue(index);
		}
		else if (instance->Type == 5 || instance->Type == 6 || instance->Type == 7) {
			return static_cast<CounterBase*>(instance)->Values.GetValue(index);
		}

		return CValue(0);
	}

	CValue GetAlterableValueByIndex(ObjectSelector& selector, int index) {
		if (selector.Count() == 0) {
			return CValue(0);
		}
		return GetAlterableValueByIndex(*(selector.begin()), index);
	}

	int GetAlterableFlagValue(ObjectInstance* instance, int index) {
		if (instance->Type == 2) {
			return static_cast<Active*>(instance)->Flags.GetFlagValue(CValue(index)).GetIntValue();
		}
		else if (instance->Type == 5 || instance->Type == 6 || instance->Type == 7) {
			return  static_cast<CounterBase*>(instance)->Flags.GetFlagValue(CValue(index)).GetIntValue();
		}
		return 0;
	}

	int GetAlterableFlagValue(ObjectSelector& selector, int index) {
		if (selector.Count() == 0) {
			return 0;
		}
		return GetAlterableFlagValue(*(selector.begin()), index);
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

	static bool LoopNameEquals(const CValue& a, const CValue& b) {
		return LoopNameEquals(a.GetStringValue(), b.GetStringValue());
	}

	void StartLoop(const std::string& name, int count) {
		std::string key = ToLowerStr(name);
		activeLoops[key] = {true, 0};
		while (activeLoops[key].running && (activeLoops[key].index < count || count < 0)) {
			OnLoop(name);
			if (!activeLoops[key].running) break;
			activeLoops[key].index++;
		}
		activeLoops.erase(key);
	}

	void StartLoop(const CValue& name, const CValue& count) {
		StartLoop(name.GetStringValue(), count.GetIntValue());
	}

	void StopLoop(const std::string& name) {
		std::string key = ToLowerStr(name);
		auto it = activeLoops.find(key);
		if (it != activeLoops.end()) {
			it->second.running = false;
		}
	}

	void StopLoop(const CValue& name) {
		StopLoop(name.GetStringValue());
	}

	CValue Loopindex(const std::string& loopName) {
		std::string key = ToLowerStr(loopName);
		auto it = activeLoops.find(key);
		if (it != activeLoops.end()) {
			return CValue(it->second.index);
		}
		return CValue(0);
	}

	CValue Loopindex(const CValue& name) {
		return Loopindex(name.GetStringValue());
	}

	virtual void OnLoop(const std::string& loopName) {}
	virtual void OnTimerEvent(const std::string& eventName) {}
	virtual void GenerateEvent(int objectType, int conditionNum, ObjectInstance* source = nullptr) {}

	//Collision detection
	CollisionInstanceBounds GetInstanceBounds(ObjectInstance* instance);
	bool IsPointInRotatedBox(int worldX, int worldY, const CollisionInstanceBounds& bounds);
	bool IsPixelSolid(const std::vector<uint8_t>& maskData, int width, int height, int x, int y);

	bool IsCollidingWithBackground(ObjectInstance* instance);
	bool IsColliding(ObjectInstance* instance1, ObjectInstance* instance2);
	bool IsColliding(ObjectInstance* instance, int x, int y);

protected:
	ObjectInstance* trueEventSource = nullptr;

	unsigned int updateCount = 0;
private:
	void DispatchTrueEvents();
	void UpdateScrolling(int newX, int newY);

	std::vector<unsigned int> instancesMarkedForDeletion;
	std::vector<bool> ActiveGroups;
	std::unordered_map<std::string, LoopState> activeLoops;

	int scrollX = 0;
	int scrollY = 0;
	bool scrollDirty = true;
};
