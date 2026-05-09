#pragma once

#include "Application.h"
#include "Extension.h"
#include "ObjectInstance.h"
#include <string>
#include "Bitmap.h"
	
class ButtonObjectExtension : public Extension {
public:
	ButtonObjectExtension(unsigned int objectInfoHandle, int type, std::string name, short width, short height, short buttonType, short flags)
	: Extension(objectInfoHandle, type, name), Width(width), Height(height), ButtonType(buttonType), Flags(flags) {}
	
	void Initialize() override;
	void Update(float deltaTime) override;
	void Draw() override;
	
	void ButtonDraw();
	void CheckboxDraw();
	void RadioButtonDraw();
	
	bool IsClicked() const;
	void SetText(const std::string& text);
	void SetEnabled(bool enabled);
	void SetShown(bool shown);
	
private:
	short Width = 0;
	short Height = 0;
	short ButtonType = 0;
	short Flags = 0;
	std::string Text = "";
	bool Enabled = true;
	bool Clicked = false;
	bool HeldDown = false;
	bool Checked = false; // used for checkboxes and radio buttons
	bool Hovered = false;
	bool Shown = true;

	Bitmap bitmap;
};