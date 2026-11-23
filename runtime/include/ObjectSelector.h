#pragma once

#include "ObjectInstance.h"
#include <unordered_set>
#include <vector>
#include <algorithm>

class ObjectIterator;

class ObjectSelector {
public:
	using iterator = std::vector<ObjectInstance*>::iterator;
	using const_iterator = std::vector<ObjectInstance*>::const_iterator;

	ObjectSelector() = default;

	// called after all object instances are created on frame initialization and when a new object is created that needs to be added to the selector
	void Initialize(const std::unordered_map<unsigned int, ObjectInstance*>& allInstances, unsigned int objectInfoId, bool isQualifier = false) {
		ObjectInfoId = objectInfoId;
		IsQualifier = isQualifier;
		AllSelectorObjectInstances.clear();
		for (const auto& [handle, instance] : allInstances) {
			instance->isSelected = false;
			if (Matches(instance)) {
				AllSelectorObjectInstances.push_back(instance);
			}
		}
		std::sort(AllSelectorObjectInstances.begin(), AllSelectorObjectInstances.end(),
			[](ObjectInstance* a, ObjectInstance* b) { return a->Handle < b->Handle; });
	}

	// true if this selector's object type or qualifier group covers the instance
	bool Matches(const ObjectInstance* instance) const {
		if (!IsQualifier) return instance->ObjectInfoHandle == ObjectInfoId;
		if (instance->Type != (ObjectInfoId >> 16)) return false;
		for (short qualifier : instance->Qualifiers) {
			if (qualifier == (ObjectInfoId & 0x7FFF)) return true;
		}
		return false;
	}

	// Called for every newly created instance: adds it to the pool without changing the current selection
	void Track(ObjectInstance* instance) {
		if (!Matches(instance)) return;
		auto it = std::lower_bound(AllSelectorObjectInstances.begin(), AllSelectorObjectInstances.end(), instance,
			[](ObjectInstance* a, ObjectInstance* b) { return a->Handle < b->Handle; });
		if (it == AllSelectorObjectInstances.end() || (*it)->Handle != instance->Handle) {
			AllSelectorObjectInstances.insert(it, instance);
		}
	}

	std::vector<ObjectInstance*> SaveSelection() {
		EnsureSorted();
		return SelectedInstances;
	}

	void RestoreSelection(const std::vector<ObjectInstance*>& snap) {
		for (auto* obj : SelectedInstances) {
			obj->isSelected = false;
		}
		SelectedInstances.clear();
		AllSelected = false;
		std::unordered_set<ObjectInstance*> wanted(snap.begin(), snap.end());
		for (auto* obj : AllSelectorObjectInstances) {
			if (wanted.count(obj) == 0) continue;
			obj->isSelected = true;
			SelectedInstances.push_back(obj);
		}
		IsDirty = false;
	}

	// Called during a event to scope all objects that are relevant to the event
	void Reset(ObjectInstance* restrictTo = nullptr) {
		if (restrictTo) {
			auto it = std::lower_bound(AllSelectorObjectInstances.begin(), AllSelectorObjectInstances.end(), restrictTo,
				[](ObjectInstance* a, ObjectInstance* b) { return a->Handle < b->Handle; });

			if (it != AllSelectorObjectInstances.end() && *it == restrictTo) {
				SelectOnly(restrictTo);
				return;
			}
		}

		for (auto* obj : SelectedInstances) {
			obj->isSelected = false;
		}
		
		SelectedInstances.clear();
		AllSelected = true;
		IsDirty = false;
	}

	// Called when an object is destroyed
	void RemoveInstance(unsigned int handle) {
		auto it = std::lower_bound(AllSelectorObjectInstances.begin(), AllSelectorObjectInstances.end(), handle,
			[](ObjectInstance* p, unsigned int h) { return p->Handle < h; });

		if (it != AllSelectorObjectInstances.end() && (*it)->Handle == handle) {
			(*it)->isSelected = false;
			AllSelectorObjectInstances.erase(it);
		}

		auto itSel = std::find_if(SelectedInstances.begin(), SelectedInstances.end(),
			[handle](ObjectInstance* p) { return p->Handle == handle; });
		
		if (itSel != SelectedInstances.end()) {
			*itSel = SelectedInstances.back();
			SelectedInstances.pop_back();
			IsDirty = true;
		}
	}

	void AddInstance(ObjectInstance* instance) {
		if (AllSelected) {
			auto it = std::lower_bound(AllSelectorObjectInstances.begin(), AllSelectorObjectInstances.end(), instance,
				[](ObjectInstance* a, ObjectInstance* b) { return a->Handle < b->Handle; });

			if (it == AllSelectorObjectInstances.end() || (*it)->Handle != instance->Handle) {
				AllSelectorObjectInstances.insert(it, instance);
			}

			return;
		}
		if (!instance->isSelected) {
			instance->isSelected = true;
			SelectedInstances.push_back(instance);
			IsDirty = true;
			
			auto it = std::lower_bound(AllSelectorObjectInstances.begin(), AllSelectorObjectInstances.end(), instance,
				[](ObjectInstance* a, ObjectInstance* b) { return a->Handle < b->Handle; });
			
			if (it == AllSelectorObjectInstances.end() || (*it)->Handle != instance->Handle) {
				AllSelectorObjectInstances.insert(it, instance);
			}
		}
	}

	// Deselect an instance by index in the selected array
	void DeselectAt(size_t index) {
		ApplyPendingReset();
		if (index < SelectedInstances.size()) {
			SelectedInstances[index]->isSelected = false;
			SelectedInstances[index] = SelectedInstances.back();
			SelectedInstances.pop_back();
			IsDirty = true;
		}
	}

	// Add an instance back to selection
	void Select(ObjectInstance* instance) {
		if (AllSelected) return;
		if (instance->ObjectInfoHandle == ObjectInfoId && !instance->isSelected) {
			instance->isSelected = true;
			SelectedInstances.push_back(instance);
			IsDirty = true;
		}
	}

	void SelectOnly(ObjectInstance* instance) {
		AllSelected = false;
		for (auto* obj : SelectedInstances) obj->isSelected = false;
		SelectedInstances.clear();
		
		instance->isSelected = true;
		SelectedInstances.push_back(instance);
		IsDirty = false; 
	}

	void SelectRandom() {
		ApplyPendingReset();
		if (!SelectedInstances.empty()) {
			size_t randomIndex = Application::Instance().RandomRange(0, static_cast<short>(SelectedInstances.size() - 1)).GetIntValue();
			ObjectInstance* instance = SelectedInstances[randomIndex];
			SelectOnly(instance);
		}
	}

	void DeselectAll() {
		for (auto* obj : SelectedInstances) obj->isSelected = false;
		SelectedInstances.clear();
		AllSelected = false;
		IsDirty = false;
	}

	// Get number of selected instances
	size_t Count() const { return AllSelected ? AllSelectorObjectInstances.size() : SelectedInstances.size(); }
	
	// Get number of all object instances in the selector
	size_t Size() const { return AllSelectorObjectInstances.size(); }

	// Access instance by index
	ObjectInstance* At(size_t index) {
		EnsureSorted();
		if (index < SelectedInstances.size()) {
			return SelectedInstances[index];
		}
		return nullptr;
	}

	// Iterator methods with improved signatures
	iterator begin() { EnsureSorted(); return SelectedInstances.begin(); }
	iterator end() { ApplyPendingReset(); return SelectedInstances.end(); }

	const_iterator begin() const { 
		const_cast<ObjectSelector*>(this)->EnsureSorted(); 
		return SelectedInstances.begin(); 
	}
	const_iterator end() const { const_cast<ObjectSelector*>(this)->ApplyPendingReset(); return SelectedInstances.end(); }

	// Friend declaration for the iterator
	friend class ObjectIterator;

private:
	void ApplyPendingReset() {
		if (!AllSelected) return;
		AllSelected = false;
		SelectedInstances = AllSelectorObjectInstances;

		for (auto* obj : SelectedInstances) {
			obj->isSelected = true;
		}
		
		IsDirty = false;
	}

	void EnsureSorted() {
		ApplyPendingReset();
		if (IsDirty) {
			std::sort(SelectedInstances.begin(), SelectedInstances.end(), 
				[](ObjectInstance* a, ObjectInstance* b) { return a->Handle < b->Handle; });
			IsDirty = false;
		}
	}

	std::vector<ObjectInstance*> SelectedInstances;
	std::vector<ObjectInstance*> AllSelectorObjectInstances; // list of all object instance handles for this selector
	unsigned int ObjectInfoId = 0;
	bool IsQualifier = false;
	bool IsDirty = false;
	bool AllSelected = false;
};

// Add ObjectIterator class to support the requested iteration pattern
class ObjectIterator {
public:
	ObjectIterator(ObjectSelector& selector) 
		: selector_(selector), currentIndex_(0), skipIncrement_(false) {
		selector_.EnsureSorted();
	}
	
	// Check if we've reached the end of iteration
	bool end() const { return currentIndex_ >= selector_.SelectedInstances.size(); }
	
	// Increment operator to move to next object
	ObjectIterator& operator++() {
		if (!end()) {
			// Only increment if we're not supposed to skip
			if (!skipIncrement_) currentIndex_++;
			else skipIncrement_ = false; // Reset the flag after using it
		}
		return *this;
	}
	
	// Dereference operator to get the current object
	ObjectInstance* operator*() {
		return end() ? nullptr : selector_.SelectedInstances[currentIndex_];
	}
	
	// Deselect the current object during iteration
	void deselect() {
		if (!end()) {
			selector_.DeselectAt(currentIndex_);
			skipIncrement_ = true;
		}
	}

	size_t index() const { return currentIndex_; }
	
private:
	ObjectSelector& selector_;
	size_t currentIndex_;
	bool skipIncrement_; // Flag to track if we should skip the next increment
};