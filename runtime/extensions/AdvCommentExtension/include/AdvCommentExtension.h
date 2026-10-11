#pragma once

#include "Extension.h"
#include "ObjectInstance.h"
	
class AdvCommentExtension : public Extension {
public:
	AdvCommentExtension(unsigned int objectInfoHandle, int type, std::string name)
		: Extension(objectInfoHandle, type, name) {}

	void Initialize() override;

	// Actions
	void ActComment();

	void ActTextRegComment();
	void ActTextRegNote();
	void ActTextRegReminder();
	void ActTextRegImportant();
	void ActTextRegAAA();
	void ActTextRegAAB();

	void ActTextCapComment();
	void ActTextCapNote();
	void ActTextCapReminder();
	void ActTextCapImportant();

	// Conditions
	bool CndComment();
	bool CndCommentObject();

	bool CndTextRegComment();
	bool CndTextRegNote();
	bool CndTextRegReminder();
	bool CndTextRegImportant();
	bool CndTextRegAAE();

	bool CndTextCapComment();
	bool CndTextCapNote();
	bool CndTextCapReminder();
	bool CndTextCapImportant();

private:
	// NaN
};