#pragma once

#include <memory>
#include <vector>

#include "ObjectGlobalDataString.h"
#include "Paragraph.h"
#include "Shape.h"
#include "ObjectSelector.h"

class StringObject : public ObjectInstance {
public:
	StringObject(unsigned int objectInfoHandle, int type, std::string name)
		: ObjectInstance(objectInfoHandle, type, name) {}

	int Width;
	int Height;

	bool Visible = true;
	bool FollowFrame = false;

	std::vector<Paragraph> Paragraphs;

	int CurrentParagraph = 0;
	std::string AlterableText;
	
	std::string GetText()
	{
		return GetTextOfParagraph(CurrentParagraph);
	}

	std::string GetTextOfParagraph(int paragraph)
	{
		if (paragraph == -1)
		{
			return AlterableText;
		}
		else
		{
			return Paragraphs[paragraph].Text;
		}
	}

	static std::string GetTextOfParagraph(ObjectSelector* selector, int paragraph)
	{
		if (selector && selector->Count() > 0)
		{
			return ((StringObject*)*selector->begin())->GetTextOfParagraph(paragraph);
		}
		return ""; // default value
	}

	unsigned short GetFont()
	{
		if (CurrentParagraph == -1)
		{
			return Paragraphs[0].Font; // TODO: Verify
		}
		else
		{
			return Paragraphs[CurrentParagraph].Font;
		}
	}

	int GetColor()
	{
		if (CurrentParagraph == -1)
		{
			return Paragraphs[0].Color; // TODO: Verify
		}
		else
		{
			return Paragraphs[CurrentParagraph].Color;
		}
	}

	void SetCurrentParagraph(int currentParagraph)
	{
		CurrentParagraph = currentParagraph;
		if (CurrentParagraph >= Paragraphs.size())
		{
			CurrentParagraph = static_cast<int>(Paragraphs.size() - 1);
		}
		else if (CurrentParagraph < 0)
		{
			CurrentParagraph = 0;
		}
	}

	int GetNumberOfCurrentParagraph()
	{
		return CurrentParagraph;
	}

	void SetPreviousParagraph()
	{
		if (CurrentParagraph == -1)
		{
			CurrentParagraph = 0;
			return;
		}

		if (CurrentParagraph > 0)
		{
			CurrentParagraph--;
		}
	}

	void SetNextParagraph()
	{
		if (CurrentParagraph == -1)
		{
			CurrentParagraph = 0;
			return;
		}

		if (CurrentParagraph < Paragraphs.size() - 1)
		{
			CurrentParagraph++;
		}
	}

	void SetAlterableText(std::string alterableText)
	{
		AlterableText = alterableText;
		CurrentParagraph = -1;
	}

	int GetParagraphCount()
	{
		return Paragraphs.size();
	}

	std::vector<unsigned int> GetFontsUsed() override {
		std::vector<unsigned int> fontsUsed;
		for (auto& paragraph : Paragraphs)
		{
			fontsUsed.push_back(paragraph.Font);
		}
		return fontsUsed;
	}

	ObjectGlobalDataString* CreateGlobalData() override {
		ObjectGlobalDataString* globalData = new ObjectGlobalDataString(ObjectInfoHandle);

		globalData->alterableText = AlterableText;
		globalData->currentParagraph = CurrentParagraph;

		return globalData;
	}

	void ApplyGlobalData(ObjectGlobalData* globalData) override {
		ObjectGlobalDataString* stringData = (ObjectGlobalDataString*)globalData;

		AlterableText = stringData->alterableText;
		CurrentParagraph = stringData->currentParagraph;
	}
};