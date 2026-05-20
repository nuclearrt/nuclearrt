#pragma once

#include <vector>
#include <memory>

#include "ObjectInstance.h"

#include "AlterableValues.h"
#include "AlterableStrings.h"
#include "AlterableFlags.h"
#include "ImageBank.h"
#include "Movements.h"
#include "Shape.h"

class CounterBase : public ObjectInstance {
public:
	CounterBase(unsigned int objectInfoHandle, int type, std::string name)
		: ObjectInstance(objectInfoHandle, type, name) {}

	bool Visible = true;
	bool FollowFrame = false;

	AlterableValues Values;
	AlterableStrings Strings;
	AlterableFlags Flags;

	Movements movements;
	
	int Width;
	int Height;

	short Player;

	unsigned int DisplayType;

	bool IntDigitPadding;
	bool FloatWholePadding;
	bool FloatDecimalPadding;
	bool FloatPadding;
	bool BarDirection;

	char IntDigitCount;
	char FloatWholeCount;
	char FloatDecimalCount;

	Shape shape;

	std::vector<unsigned int> Frames;

	unsigned int Font = 0;

	std::vector<unsigned int> GetImagesUsed() override {
		return Frames;
	}
	
	std::vector<unsigned int> GetFontsUsed() override {
		return { Font };
	}

	virtual int GetValue() const { return 0; }

	int GetCounterWidth() {
		if (DisplayType == 1) // Numbers
		{
			std::string valueString = std::to_string(GetValue());
			int numDigits = static_cast<int>(valueString.size());

			if (IntDigitCount > 0)
				numDigits = IntDigitCount;

			bool negative = false;
			if (GetValue() < 0)
			{
				negative = true;
				valueString = valueString.substr(1);
			}

			// Fixed Size
			if (IntDigitCount > 0)
			{
				if (IntDigitCount > valueString.size()) // Add leading zeros
				{
					while (valueString.size() < IntDigitCount)
					{
						valueString = "0" + valueString;
					}
				}
				else // Remove extra digits
				{
					valueString = valueString.substr(valueString.size() - IntDigitCount);
				}
			}

			if (negative)
			{
				valueString = "-" + valueString;
				numDigits++;
			}
			
			std::string charMap = "0123456789-+.e";

			int totalWidth = 0;
			for (int i = 0; i < numDigits; i++)
			{
				if (i >= valueString.size())
				{
					break;
				}

				int imageIndex = static_cast<int>(charMap.find(valueString[i]));
				totalWidth += ImageBank::Instance().GetImage(Frames[imageIndex])->Width;
			}
			return totalWidth;
		}
		else
		{
			return Width;
		}
	}

	int GetCounterHeight(){
		if (DisplayType == 1) // Numbers
		{
			std::string valueString = std::to_string(GetValue());
			int numDigits = static_cast<int>(valueString.size());

			if (IntDigitCount > 0)
				numDigits = IntDigitCount;

			bool negative = false;
			if (GetValue() < 0)
			{
				negative = true;
				valueString = valueString.substr(1);
			}

			// Fixed Size
			if (IntDigitCount > 0)
			{
				if (IntDigitCount > valueString.size()) // Add leading zeros
				{
					while (valueString.size() < IntDigitCount)
					{
						valueString = "0" + valueString;
					}
				}
				else // Remove extra digits
				{
					valueString = valueString.substr(valueString.size() - IntDigitCount);
				}
			}

			if (negative)
			{
				valueString = "-" + valueString;
				numDigits++;
			}

			std::string charMap = "0123456789-+.e";

			int maxHeight = 0;
			for (int i = 0; i < numDigits; i++)
			{
				if (i >= valueString.size())
				{
					break;
				}

				int imageIndex = static_cast<int>(charMap.find(valueString[i]));
				int charHeight = ImageBank::Instance().GetImage(Frames[imageIndex])->Height;
				maxHeight = std::max(maxHeight, charHeight);
			}
			return maxHeight;
		}
		else
		{
			return Height;
		}
	}
};