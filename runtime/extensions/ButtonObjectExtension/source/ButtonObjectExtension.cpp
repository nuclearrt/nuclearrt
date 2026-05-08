#include "ButtonObjectExtension.h"
#include "Application.h"
#include "GraphicsBackend.h"

void ButtonObjectExtension::Initialize() {
	if (Flags & 1) {
		Shown = false;
	}

	if (Flags & 2) {
		Enabled = false;
	}

	bitmap.Resize(Width, Height);
}

void ButtonObjectExtension::Update(float deltaTime) {
	auto input = Application::Instance().GetInput();
	int mouseX = input->GetMouseX();
	int mouseY = input->GetMouseY();
	Hovered = (mouseX >= X && mouseX < X + Width &&
				mouseY >= Y && mouseY < Y + Height) && Shown;
	Clicked = Hovered && input->IsMouseButtonPressed(1, false) && Enabled;
	HeldDown = Hovered && input->IsMouseButtonDown(1) && Enabled;

	if (!Clicked) {
		return;
	}

	if (ButtonType == 1) { // checkbox
		Checked = !Checked;
	}
	else if (ButtonType == 2) { // radio button
		Checked = true;
	}
}

void ButtonObjectExtension::Draw() {
	if (!Shown) return;

	bitmap.Clear(0xFFFFFFFF);

	switch (ButtonType) {
		case 0: // button
		case 3: // Bitmap button
		case 4: // Bitmap with text button
			ButtonDraw();
			break;
		case 1: // checkbox
			CheckboxDraw();
			break;
		case 2: // radio button
			RadioButtonDraw();
			break;
	}

	Application::Instance().GetBackend()->graphics->DrawBitmap(bitmap, X, Y);
}

void ButtonObjectExtension::ButtonDraw() {
	int borderColor = 0xFFADADAD;
	int fillColor = 0xFFE1E1E1;

	if (!Enabled) {
		fillColor = 0xFFCCCCCC;
		borderColor = 0xFFBFBFBF;
	}
	else if (HeldDown) {
		fillColor = 0xFFCCE4F7;
		borderColor = 0xFF005499;
	} else if (Hovered) {
		fillColor = 0xFFE5F1FB;
		borderColor = 0xFF0078D7;
	}

	bitmap.DrawRectangle(1, 1, Width - 2, Height - 2, fillColor);
	bitmap.DrawRectangleLines(1, 1, Width - 2, Height - 2, borderColor);
}

void ButtonObjectExtension::CheckboxDraw() {
	int boxX = 0;

	int borderColor = 0xFF626262;
	int fillColor = 0xFFFFFFFF;

	if (!Enabled) {
		fillColor = 0xFFF9F9F9;
		borderColor = 0xFFC3C3C3;
	}
	else if (Checked) {
		fillColor = 0xFF005FB8;
		borderColor = 0xFF005FB8;
	}

	if (Flags & 4) {
		boxX = Width - 12;
	}

	bitmap.DrawRectangle(boxX, Height - 18, 12, 12, fillColor);
	bitmap.DrawRectangleLines(boxX, Height - 18, 12, 12, borderColor);
}

void ButtonObjectExtension::RadioButtonDraw() {
	int radioX = 0;

	int borderColor = 0xFF626262;
	int fillColor = 0xFFFFFFFF;

	if (!Enabled) {
		fillColor = 0xFFF9F9F9;
		borderColor = 0xFFC3C3C3;
	}
	else if (Checked) {
		fillColor = 0xFF005FB8;
		borderColor = 0xFF005FB8;
	}

	if (Flags & 4) {
		radioX = Width - 12;
	}

	bitmap.DrawRectangle(radioX, Height - 18, 12, 12, fillColor);
	bitmap.DrawRectangleLines(radioX, Height - 18, 12, 12, borderColor);
}

bool ButtonObjectExtension::IsClicked() const {
	return Clicked;
}

void ButtonObjectExtension::SetText(const std::string& text) {
	Text = text;
}

void ButtonObjectExtension::SetEnabled(bool enabled) {
	Enabled = enabled;
}

void ButtonObjectExtension::SetShown(bool shown) {
	Shown = shown;
}