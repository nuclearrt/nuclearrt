#include "Transition.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>

#include "GraphicsBackend.h"

namespace
{
	constexpr float TWO_PI = 6.28318f;

	enum { LEFT_RIGHT = 0, RIGHT_LEFT = 1, TOP_BOTTOM = 2, BOTTOM_TOP = 3 };
	enum { CENTER_LEFTRIGHT = 0, LEFTRIGHT_CENTER = 1, CENTER_TOPBOTTOM = 2, TOPBOTTOM_CENTER = 3 };
	enum { TOP_LEFT = 0, TOP_RIGHT = 1, BOTTOM_LEFT = 2, BOTTOM_RIGHT = 3, CENTER = 4 };
	enum { DIR_HORZ = 0, DIR_VERT = 1 };

	constexpr int S1 = TRANSITION_SURFACE_START;
	constexpr int S2 = TRANSITION_SURFACE_END;
}

int Transition::ParameterReader::ReadInt()
{
	if (position + 4 > data.size())
	{
		position = data.size();
		return 0;
	}
	int value = static_cast<int>(static_cast<unsigned int>(data[position]) |
		(static_cast<unsigned int>(data[position + 1]) << 8) |
		(static_cast<unsigned int>(data[position + 2]) << 16) |
		(static_cast<unsigned int>(data[position + 3]) << 24));
	position += 4;
	return value;
}

short Transition::ParameterReader::ReadShort()
{
	if (position + 2 > data.size())
	{
		position = data.size();
		return 0;
	}
	short value = static_cast<short>(data[position] | (data[position + 1] << 8));
	position += 2;
	return value;
}

void Transition::Start(const TransitionData& data, GraphicsBackend* gfx, bool isFadeOut, unsigned int nowMs, int displayWidth, int displayHeight)
{
	graphics = gfx;
	fadeOut = isFadeOut;
	width = static_cast<float>(std::max(1, displayWidth));
	height = static_cast<float>(std::max(1, displayHeight));
	duration = static_cast<float>(std::max(1, data.Duration));
	startTime = nowMs;
	rng.seed(static_cast<unsigned int>(std::rand()) ^ nowMs);

	ParameterReader reader(data.Parameters);
	ReadParameters(reader);

	Blit(S1);
	OnStart();
}

void Transition::Step(unsigned int nowMs)
{
	float elapsed = static_cast<float>(nowMs - startTime);
	StepDraw(std::clamp(elapsed, 0.0f, duration));
}

void Transition::Finish()
{
	Blit(S2);
}

bool Transition::IsCompleted(unsigned int nowMs) const
{
	return static_cast<float>(nowMs - startTime) >= duration;
}

int Transition::RandomInt(int max)
{
	if (max <= 1) return 0;
	return std::uniform_int_distribution<int>(0, max - 1)(rng);
}

void Transition::Blit(int surface)
{
	Stretch(surface, 0.0f, 0.0f, width, height, 0.0f, 0.0f, width, height);
}

void Transition::Blit(int surface, float xDest, float yDest, float xSrc, float ySrc, float w, float h)
{
	Stretch(surface, xDest, yDest, w, h, xSrc, ySrc, w, h);
}

void Transition::Stretch(int surface, float xDest, float yDest, float wDest, float hDest, float xSrc, float ySrc, float wSrc, float hSrc, float alpha)
{
	if (graphics == nullptr || wDest <= 0.0f || hDest <= 0.0f || wSrc <= 0.0f || hSrc <= 0.0f)
		return;

	float scaleX = wDest / wSrc;
	float scaleY = hDest / hSrc;
	if (xSrc < 0.0f) { xDest -= xSrc * scaleX; wDest += xSrc * scaleX; wSrc += xSrc; xSrc = 0.0f; }
	if (ySrc < 0.0f) { yDest -= ySrc * scaleY; hDest += ySrc * scaleY; hSrc += ySrc; ySrc = 0.0f; }
	if (xSrc + wSrc > width) { float over = xSrc + wSrc - width; wSrc -= over; wDest -= over * scaleX; }
	if (ySrc + hSrc > height) { float over = ySrc + hSrc - height; hSrc -= over; hDest -= over * scaleY; }
	if (wDest <= 0.0f || hDest <= 0.0f || wSrc <= 0.0f || hSrc <= 0.0f)
		return;

	graphics->DrawTransitionSurface(surface, xDest, yDest, wDest, hDest,
		xSrc / width, ySrc / height, (xSrc + wSrc) / width, (ySrc + hSrc) / height,
		alpha, 0, 0.0f, 0.0f, width, height);
}

void Transition::BlitPattern(int surface, int pattern, float revealWidth, float revealHeight)
{
	if (graphics == nullptr) return;
	graphics->DrawTransitionSurface(surface, 0.0f, 0.0f, width, height, 0.0f, 0.0f, 1.0f, 1.0f,
		1.0f, pattern, revealWidth, revealHeight, width, height);
}

namespace
{

// Fade
class TransFade : public Transition
{
protected:
	void StepDraw(float elapsed) override
	{
		Blit(S1);
		Stretch(S2, 0, 0, width, height, 0, 0, width, height, Progress(elapsed));
	}
};

// Bands
class TransBand : public Transition
{
	int bands = 1;
	int direction = 0;
	float bandSize = 1.0f;
protected:
	void ReadParameters(ParameterReader& r) override
	{
		bands = r.ReadShort();
		direction = r.ReadShort();
	}
	void OnStart() override
	{
		if (bands <= 0) bands = 1;
		float size = (direction == LEFT_RIGHT || direction == RIGHT_LEFT) ? width : height;
		bandSize = std::ceil(size / bands);
		if (bandSize < 1.0f)
		{
			bandSize = 1.0f;
			bands = static_cast<int>(size);
		}
	}
	void StepDraw(float elapsed) override
	{
		float rw = bandSize * Progress(elapsed);
		for (int n = 0; n < bands; n++)
		{
			float offset = n * bandSize;
			switch (direction)
			{
				case LEFT_RIGHT: Blit(S2, offset, 0, offset, 0, rw, height); break;
				case RIGHT_LEFT: Blit(S2, width - offset - rw, 0, width - offset - rw, 0, rw, height); break;
				case TOP_BOTTOM: Blit(S2, 0, offset, 0, offset, width, rw); break;
				default: Blit(S2, 0, height - offset - rw, 0, height - offset - rw, width, rw); break;
			}
		}
	}
};

// Advanced scrolling
class TransAdvancedScrolling : public Transition
{
	int style = 0;
protected:
	void ReadParameters(ParameterReader& r) override { style = r.ReadInt(); }
	void OnStart() override { if (style == 8) style = RandomInt(8); }
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		float w3 = width / 3.0f;
		float w, h;

		if (style >= 0 && style <= 5)
		{
			// left and right thirds scroll in horizontally
			w = w3 * p;
			Blit(S2, 0, 0, w3 - w, 0, w, height);
			Blit(S2, width - w, 0, 2 * w3, 0, w, height);
		}

		switch (style)
		{
			case 0: // middle scrolls down
				h = height * p;
				Blit(S2, w3, 0, w3, height - h, w3, h);
				break;
			case 1: // middle scrolls up
				h = height * p;
				Blit(S2, w3, height - h, w3, 0, w3, h);
				break;
			case 2: // middle wipes down
				h = height * p;
				Blit(S2, w3, 0, w3, 0, w3, h);
				break;
			case 3: // middle wipes up
				h = height * p;
				Blit(S2, w3, height - h, w3, height - h, w3, h);
				break;
			case 4: // middle scrolls in from top and bottom
				h = height / 2 * p;
				Blit(S2, w3, 0, w3, height / 2 - h, w3, h);
				Blit(S2, w3, height - h, w3, height / 2, w3, h);
				break;
			case 5: // middle wipes in from top and bottom
				h = height / 2 * p;
				Blit(S2, w3, 0, w3, 0, w3, h);
				Blit(S2, w3, height - h, w3, height - h, w3, h);
				break;
			case 6:
			case 7:
			{
				// alternating vertical bands scrolling up and down
				int count = style == 6 ? 3 : 7;
				float bw = width / count;
				h = height * p;
				for (int i = 0; i < count; i++)
				{
					float bandWidth = (i == count - 1) ? width - bw * i : bw;
					if (i % 2 == 0)
						Blit(S2, bw * i, height - h, bw * i, 0, bandWidth, h);
					else
						Blit(S2, bw * i, 0, bw * i, height - h, bandWidth, h);
				}
				break;
			}
			default:
				Blit(S2);
				break;
		}
	}
};

// Back
class TransBack : public Transition
{
	int style = 0;
protected:
	void ReadParameters(ParameterReader& r) override { style = r.ReadInt(); }
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		float hw = width / 2, hh = height / 2;
		float w, h;
		Blit(S2);

		switch (style)
		{
			case 0: // the four quarters shrink into the corners
				w = hw - hw * p; h = hh - hh * p;
				Stretch(S1, 0, 0, w, h, 0, 0, hw, hh);
				Stretch(S1, hw + hw * p, 0, w, h, hw, 0, hw, hh);
				Stretch(S1, 0, hh + hh * p, w, h, 0, hh, hw, hh);
				Stretch(S1, hw + hw * p, hh + hh * p, w, h, hw, hh, hw, hh);
				break;
			case 1: // slide to the top left
				w = width - width * p; h = height - height * p;
				Blit(S1, 0, 0, width - w, height - h, w, h);
				break;
			case 2: // slide to the top right
				w = width * p; h = height - height * p;
				Blit(S1, w, 0, 0, height - h, width - w, h);
				break;
			case 3: // slide to the bottom left
				w = width - width * p; h = height * p;
				Blit(S1, 0, h, width - w, 0, w, height - h);
				break;
			case 4: // slide to the bottom right
				w = width * p; h = height * p;
				Blit(S1, w, h, 0, 0, width - w, height - h);
				break;
			case 5: // the four quarters scroll into the corners
				w = hw * p; h = hh * p;
				Blit(S1, -w, -h, 0, 0, hw, hh);
				Blit(S1, hw + w, -h, hw, 0, hw, hh);
				Blit(S1, -w, hh + h, 0, hh, hw, hh);
				Blit(S1, hw + w, hh + h, hw, hh, hw, hh);
				break;
			case 6: // the halves scroll up and down
				h = hh * p;
				Blit(S1, 0, -h, 0, 0, width, hh);
				Blit(S1, 0, hh + h, 0, hh, width, hh);
				break;
			case 7: // the halves scroll left and right
				w = hw * p;
				Blit(S1, -w, 0, 0, 0, hw, height);
				Blit(S1, hw + w, 0, hw, 0, hw, height);
				break;
		}
	}
};

// Cell
class TransCell : public Transition
{
	int columns = 1, rows = 1;
protected:
	void ReadParameters(ParameterReader& r) override
	{
		columns = std::max(1, r.ReadInt());
		rows = std::max(1, r.ReadInt());
	}
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		float w = width / columns, h = height / rows;
		float w2 = w * p, h2 = h * p;
		for (int i = 0; i < columns; i++)
		{
			for (int j = 0; j < rows; j++)
			{
				float x = i * w + (w - w2) / 2;
				float y = j * h + (h - h2) / 2;
				Blit(S2, x, y, x, y, w2, h2);
			}
		}
	}
};

// Door
class TransDoor : public Transition
{
	int direction = 0;
protected:
	void ReadParameters(ParameterReader& r) override { direction = r.ReadShort(); }
	void StepDraw(float elapsed) override
	{
		bool horizontal = direction == CENTER_LEFTRIGHT || direction == LEFTRIGHT_CENTER;
		float half = (horizontal ? width : height) / 2;
		float rw = half * Progress(elapsed);
		switch (direction)
		{
			case CENTER_LEFTRIGHT: Blit(S2, half - rw, 0, half - rw, 0, rw * 2, height); break;
			case LEFTRIGHT_CENTER:
				Blit(S2, 0, 0, 0, 0, rw, height);
				Blit(S2, width - rw, 0, width - rw, 0, rw, height);
				break;
			case CENTER_TOPBOTTOM: Blit(S2, 0, half - rw, 0, half - rw, width, rw * 2); break;
			default:
				Blit(S2, 0, 0, 0, 0, width, rw);
				Blit(S2, 0, height - rw, 0, height - rw, width, rw);
				break;
		}
	}
};

// Line
class TransLine : public Transition
{
	int lines = 1, style = 0, scrolling = 0;
protected:
	void ReadParameters(ParameterReader& r) override
	{
		lines = std::max(1, r.ReadInt());
		style = r.ReadInt();
		scrolling = r.ReadInt();
	}
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		if (style == 0) // horizontal lines
		{
			float lineSize = height / lines;
			float w = width * p;
			for (int i = 0; i < lines; i++)
			{
				float y = i * lineSize;
				float h = (i == lines - 1) ? height - y : lineSize + 1.0f;
				if (i % 2 == 0)
					Blit(S2, 0, y, scrolling ? width - w : 0, y, w, h);
				else
					Blit(S2, width - w, y, scrolling ? 0 : width - w, y, w, h);
			}
		}
		else // vertical lines
		{
			float lineSize = width / lines;
			float h = height * p;
			for (int i = 0; i < lines; i++)
			{
				float x = i * lineSize;
				float w = (i == lines - 1) ? width - x : lineSize + 1.0f;
				if (i % 2 == 0)
					Blit(S2, x, 0, x, scrolling ? height - h : 0, w, h);
				else
					Blit(S2, x, height - h, x, scrolling ? 0 : height - h, w, h);
			}
		}
	}
};

// Mosaic
class TransMosaic : public Transition
{
	int spotPercent = 0;
	int spotSize = 1, blocksPerLine = 0, blocksPerColumn = 0;
	std::vector<int> order;
	size_t shown = 0;
protected:
	void ReadParameters(ParameterReader& r) override { spotPercent = r.ReadInt(); }
	void OnStart() override
	{
		spotSize = std::max(1, static_cast<int>((width * spotPercent / 100 + height * spotPercent / 100) / 2));
		blocksPerLine = (static_cast<int>(width) + spotSize - 1) / spotSize;
		blocksPerColumn = (static_cast<int>(height) + spotSize - 1) / spotSize;
		order.resize(static_cast<size_t>(blocksPerLine) * blocksPerColumn);
		std::iota(order.begin(), order.end(), 0);
		for (size_t i = order.size(); i > 1; i--)
			std::swap(order[i - 1], order[RandomInt(static_cast<int>(i))]);
		shown = 0;
	}
	void StepDraw(float elapsed) override
	{
		if (blocksPerLine < 2 || blocksPerColumn < 2)
		{
			Blit(S2);
			return;
		}
		size_t target = std::min(order.size(), static_cast<size_t>(order.size() * Progress(elapsed)));
		for (; shown < target; shown++)
		{
			float x = static_cast<float>((order[shown] % blocksPerLine) * spotSize);
			float y = static_cast<float>((order[shown] / blocksPerLine) * spotSize);
			Blit(S2, x, y, x, y, static_cast<float>(spotSize), static_cast<float>(spotSize));
		}
	}
};

// Open
class TransOpen : public Transition
{
protected:
	void ReadParameters(ParameterReader& r) override { r.ReadInt(); }
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		float x, y, w, h;
		if (p < 0.25f)
		{
			w = width * 4 * p;
			h = height / 7;
			x = width / 2 - w / 2;
			y = height / 2 - h / 2;
			Blit(S2, x, y, x, y, w, h);

			w = width / 7;
			h = height * 4 * p;
			x = width / 2 - w / 2;
			y = height / 2 - h / 2;
			Blit(S2, x, y, x, y, w, h);
		}
		else
		{
			float hw = width / 2, hh = height / 2;
			w = width * p - hw;
			Blit(S2, hw, 0, hw, 0, w, hh);
			h = height * p - hh;
			Blit(S2, hw, hh, hw, hh, hw, h);
			Blit(S2, hw - w, hh, hw - w, hh, w, hh);
			Blit(S2, 0, hh - h, 0, hh - h, hw, h);
		}
	}
};

// Push
class TransPush : public Transition
{
	int style = 0;
	bool refreshed = false;
protected:
	void ReadParameters(ParameterReader& r) override { style = r.ReadInt(); }
	void OnStart() override { refreshed = false; }
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		float hh = height / 2;
		if (p <= 0.5f)
		{
			float w = width * p * 2;
			float x = width - w;
			switch (style)
			{
				case 0: Blit(S2, 0, 0, x, hh, w, hh); break;
				case 1: Blit(S2, x, 0, 0, hh, w, hh); break;
				case 2: Blit(S2, 0, hh, x, 0, w, hh); break;
				case 3: Blit(S2, x, hh, 0, 0, w, hh); break;
			}
		}
		else
		{
			if (!refreshed)
			{
				if (style <= 1)
					Blit(S2, 0, 0, 0, hh, width, hh);
				else
					Blit(S2, 0, hh, 0, 0, width, hh);
				refreshed = true;
			}

			float h = hh * (p - 0.5f) * 2;
			if (style <= 1)
			{
				Stretch(S2, 0, h, width, hh, 0, hh, width, hh);
				Stretch(S2, 0, 0, width, h, 0, hh - h, width, h);
			}
			else
			{
				Stretch(S2, 0, hh - h, width, hh, 0, 0, width, hh);
				Stretch(S2, 0, height - h, width, h, 0, hh, width, h);
			}
		}
	}
};

// Scroll
class TransScroll : public Transition
{
	int direction = 0;
protected:
	void ReadParameters(ParameterReader& r) override { direction = r.ReadInt(); }
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		switch (direction)
		{
			case LEFT_RIGHT: Blit(S2, width * p - width, 0, 0, 0, width, height); break;
			case RIGHT_LEFT: Blit(S2, width - width * p, 0, 0, 0, width, height); break;
			case TOP_BOTTOM: Blit(S2, 0, height * p - height, 0, 0, width, height); break;
			default: Blit(S2, 0, height - height * p, 0, 0, width, height); break;
		}
	}
};

// Square
class TransSquare : public Transition
{
	int style = 0, position = 0, stretch = 0;
protected:
	void ReadParameters(ParameterReader& r) override
	{
		style = r.ReadInt();
		position = r.ReadInt();
		stretch = r.ReadInt();
	}
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);

		// inside square
		float sqW = width * position / 100;
		float sqH = height * position / 100;
		float w = sqW * p, h = sqH * p;
		float x = width / 2 - w / 2, y = height / 2 - h / 2;
		if (stretch == 0)
			Blit(S2, x, y, x, y, w, h);
		else
			Stretch(S2, x, y, w, h, width / 2 - sqW / 2, height / 2 - sqH / 2, sqW, sqH);

		// outside borders
		float outside = static_cast<float>(100 - position);
		w = width * outside / 100 / 2 * p;
		h = height * outside / 100 / 2 * p;
		Blit(S2, 0, 0, 0, 0, width, h);
		Blit(S2, 0, 0, 0, 0, w, height);
		Blit(S2, 0, height - h, 0, height - h, width, h);
		Blit(S2, width - w, 0, width - w, 0, w, height);
	}
};

// Stretch
class TransStretch : public Transition
{
	int style = 0;
protected:
	void ReadParameters(ParameterReader& r) override { style = r.ReadInt(); }
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		float w = width * p, h = height * p;
		float hw = width / 2, hh = height / 2;
		switch (style)
		{
			case 0: Stretch(S2, 0, 0, w, h, 0, 0, width, height); break;
			case 1: Stretch(S2, width - w, 0, w, h, 0, 0, width, height); break;
			case 2: Stretch(S2, 0, height - h, w, h, 0, 0, width, height); break;
			case 3: Stretch(S2, width - w, height - h, w, h, 0, 0, width, height); break;
			case 4: // four corners
			{
				float qw = hw * p, qh = hh * p;
				float qhTop = std::max(qh, 5.0f);
				Stretch(S2, 0, 0, qw, qhTop, 0, 0, hw, hh);
				Stretch(S2, width - qw, 0, qw, qhTop, hw, 0, hw, hh);
				Stretch(S2, 0, height - qh, qw, qh, 0, hh, hw, hh);
				Stretch(S2, width - qw, height - qh, qw, qh, hw, hh, hw, hh);
				break;
			}
			case 5: // centre
			{
				float qw = hw * p, qh = hh * p;
				float qhTop = std::max(qh, 5.0f);
				Stretch(S2, hw - qw, hh - qhTop, qw, qhTop, 0, 0, hw, hh);
				Stretch(S2, hw, hh - qhTop, qw, qhTop, hw, 0, hw, hh);
				Stretch(S2, hw - qw, hh, qw, qh, 0, hh, hw, hh);
				Stretch(S2, hw, hh, qw, qh, hw, hh, hw, hh);
				break;
			}
			case 6: Stretch(S2, 0, 0, width, h, 0, 0, width, height); break;
			case 7: Stretch(S2, 0, 0, w, height, 0, 0, width, height); break;
			case 8: Stretch(S2, width - w, 0, w, height, 0, 0, width, height); break;
			case 9: Stretch(S2, 0, height - h, width, h, 0, 0, width, height); break;
		}
	}
};

// Stretch 2
class TransStretch2 : public Transition
{
	int style = 0;
protected:
	void ReadParameters(ParameterReader& r) override { style = r.ReadInt(); }
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		bool shrinking = p < 0.5f;
		int surface = shrinking ? S1 : S2;
		float f = shrinking ? 1.0f - 2.0f * p : 2.0f * p - 1.0f; // size factor of the image being drawn
		float w = width * f, h = height * f;

		float x = 0, y = 0;
		switch (style)
		{
			case 0: break;                                          // top left
			case 1: w = width; break;                               // top middle
			case 2: x = width - w; break;                           // top right
			case 3: h = height; break;                              // middle left
			case 4: h = height; x = width / 2 - w / 2; break;       // centre horizontal
			case 5: w = width; y = height / 2 - h / 2; break;       // centre vertical
			case 6: x = width / 2 - w / 2; y = height / 2 - h / 2; break; // centre
			case 7: h = height; x = width - w; break;               // middle right
			case 8: y = height - h; break;                          // bottom left
			case 9: w = width; y = height - h; break;               // bottom middle
			case 10: x = width - w; y = height - h; break;          // bottom right
			default: Blit(S2); return;
		}
		Stretch(surface, x, y, w, h, 0, 0, width, height);
	}
};

// Trame
class TransTrame : public Transition
{
	int style = 0;
protected:
	void ReadParameters(ParameterReader& r) override { style = r.ReadInt(); }
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		BlitPattern(S2, std::clamp(style, 0, 2) + 1, width * p, height * p);
	}
};

// Turn
class TransTurn : public Transition
{
	int turns = 0, fullImage = 0, inverse = 0;
protected:
	void ReadParameters(ParameterReader& r) override
	{
		turns = r.ReadInt();
		fullImage = r.ReadInt();
		inverse = r.ReadInt();
	}
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		float angle = turns * TWO_PI * p;
		if (inverse == 1) angle = TWO_PI - angle;

		float dist = width / 2 - width / 2 * p;
		float x = std::floor(width / 2 + std::cos(angle) * dist);
		float y = std::floor(height / 2 + std::sin(angle) * dist);
		float w = width * p, h = height * p;

		Blit(S1);
		if (fullImage == 1)
			Stretch(S2, x - w / 2, y - h / 2, w, h, 0, 0, width, height);
		else
			Stretch(S2, x - w / 2, y - h / 2, w, h, width - w, height - h, w, h);
	}
};

// Turn 2
class TransTurn2 : public Transition
{
	int turns = 0, inverse = 0;
protected:
	void ReadParameters(ParameterReader& r) override
	{
		turns = r.ReadInt();
		inverse = r.ReadInt();
	}
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		float angle = std::fmod(turns * TWO_PI * p, TWO_PI);
		if (inverse == 1) angle = TWO_PI - angle;

		float dist = width * p;
		float x = std::floor(width / 2 + std::cos(angle) * dist);
		float y = std::floor(height / 2 + std::sin(angle) * dist);

		Blit(S2);
		Blit(S1, x - width / 2, y - height / 2, 0, 0, width, height);
	}
};

// Zigzag
class TransZigZag : public Transition
{
	int spotPercent = 0, startPoint = 0, direction = 0;
	int spotSize = 1, blockCount = 0, blocksShown = 0;
	int curX = 0, curY = 0, currentStart = 0;
	int left = 0, top = 0, right = 0, bottom = 0;
protected:
	void ReadParameters(ParameterReader& r) override
	{
		spotPercent = r.ReadInt();
		startPoint = r.ReadShort();
		direction = r.ReadShort();
	}
	void OnStart() override
	{
		int sw = static_cast<int>(width), sh = static_cast<int>(height);
		spotSize = std::max(1, static_cast<int>((width * spotPercent / 100 + height * spotPercent / 100) / 2));
		int perLine = (sw + spotSize - 1) / spotSize;
		int perColumn = (sh + spotSize - 1) / spotSize;
		currentStart = startPoint;

		switch (startPoint)
		{
			case TOP_LEFT: curX = curY = 0; break;
			case TOP_RIGHT: curX = sw - spotSize; curY = 0; break;
			case BOTTOM_LEFT: curX = 0; curY = sh - spotSize; break;
			case BOTTOM_RIGHT: curX = sw - spotSize; curY = sh - spotSize; break;
			case CENTER:
				curX = sw / 2 - spotSize;
				curY = sh / 2 - spotSize;
				currentStart = direction == DIR_HORZ ? TOP_LEFT : TOP_RIGHT;
				left = curX - spotSize;
				top = curY - spotSize;
				bottom = curY + spotSize * 2;
				right = curX + spotSize * 2;
				perLine = 2 + 2 * (curX + spotSize - 1) / spotSize;
				perColumn = 2 + 2 * (curY + spotSize - 1) / spotSize;
				break;
		}
		blockCount = perLine * perColumn;
		blocksShown = 0;
	}
	void StepDraw(float elapsed) override
	{
		int sw = static_cast<int>(width), sh = static_cast<int>(height);
		if (spotSize >= sw || spotSize >= sh)
		{
			Blit(S2);
			return;
		}

		int target = static_cast<int>(blockCount * Progress(elapsed));
		for (; blocksShown < target; blocksShown++)
		{
			float s = static_cast<float>(spotSize);
			Blit(S2, static_cast<float>(curX), static_cast<float>(curY), static_cast<float>(curX), static_cast<float>(curY), s, s);
			Advance(sw, sh);
		}
	}
private:
	void Advance(int sw, int sh)
	{
		if (startPoint == CENTER)
		{
			// spiral outwards
			switch (currentStart)
			{
				case TOP_LEFT:
					curX += spotSize;
					if (curX >= right) { curX -= spotSize; curY += spotSize; currentStart = TOP_RIGHT; right += spotSize; }
					break;
				case TOP_RIGHT:
					curY += spotSize;
					if (curY >= bottom) { curY -= spotSize; curX -= spotSize; currentStart = BOTTOM_RIGHT; bottom += spotSize; }
					break;
				case BOTTOM_RIGHT:
					curX -= spotSize;
					if (curX + spotSize <= left) { curX += spotSize; curY -= spotSize; currentStart = BOTTOM_LEFT; left -= spotSize; }
					break;
				case BOTTOM_LEFT:
					curY -= spotSize;
					if (curY + spotSize <= top) { curY += spotSize; curX += spotSize; currentStart = TOP_LEFT; top -= spotSize; }
					break;
			}
			return;
		}

		if (direction == DIR_HORZ)
		{
			switch (currentStart)
			{
				case TOP_LEFT:
					curX += spotSize;
					if (curX >= sw) { curX -= spotSize; curY += spotSize; currentStart = TOP_RIGHT; }
					break;
				case TOP_RIGHT:
					curX -= spotSize;
					if (curX + spotSize <= 0) { curX += spotSize; curY += spotSize; currentStart = TOP_LEFT; }
					break;
				case BOTTOM_LEFT:
					curX += spotSize;
					if (curX >= sw) { curX -= spotSize; curY -= spotSize; currentStart = BOTTOM_RIGHT; }
					break;
				case BOTTOM_RIGHT:
					curX -= spotSize;
					if (curX + spotSize <= 0) { curX += spotSize; curY -= spotSize; currentStart = BOTTOM_LEFT; }
					break;
			}
		}
		else
		{
			switch (currentStart)
			{
				case TOP_LEFT:
					curY += spotSize;
					if (curY >= sh) { curY -= spotSize; curX += spotSize; currentStart = BOTTOM_LEFT; }
					break;
				case TOP_RIGHT:
					curY += spotSize;
					if (curY >= sh) { curY -= spotSize; curX -= spotSize; currentStart = BOTTOM_RIGHT; }
					break;
				case BOTTOM_LEFT:
					curY -= spotSize;
					if (curY + spotSize <= 0) { curY += spotSize; curX += spotSize; currentStart = TOP_LEFT; }
					break;
				case BOTTOM_RIGHT:
					curY -= spotSize;
					if (curY + spotSize <= 0) { curY += spotSize; curX -= spotSize; currentStart = TOP_RIGHT; }
					break;
			}
		}
	}
};

// Zigzag 2
class TransZigZag2 : public Transition
{
	int style = 0, lines = 1;
	int linePos = 0, dir = 0;
protected:
	void ReadParameters(ParameterReader& r) override
	{
		style = r.ReadInt();
		lines = std::max(1, r.ReadInt());
	}
	void OnStart() override { linePos = 0; dir = 0; }
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		if (style == 0)
		{
			float lineSize = height / lines;
			float h = std::floor(linePos * lineSize) + std::floor(lineSize);
			float w = width * p * lines / 2 - width * linePos;
			float x = dir == 0 ? 0 : width - w;
			Blit(S2, x, 0, x, 0, w, h);
			x = dir == 1 ? 0 : width - w;
			Blit(S2, x, height - h, x, height - h, w, h);
			if (w >= width)
			{
				linePos++;
				dir ^= 1;
			}
		}
		else
		{
			float lineSize = width / lines;
			float w = std::floor(linePos * lineSize) + std::floor(lineSize);
			float h = height * p * lines / 2 - height * linePos;
			float y = dir == 0 ? 0 : height - h;
			Blit(S2, 0, y, 0, y, w, h);
			y = dir == 1 ? 0 : height - h;
			Blit(S2, width - w, y, width - w, y, w, h);
			if (h >= height)
			{
				linePos++;
				dir ^= 1;
			}
		}
	}
};

// Zoom
class TransZoom : public Transition
{
protected:
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		if (fadeOut)
		{
			float nw = std::floor(width - width * p), nh = std::floor(height - height * p);
			Blit(S2);
			Stretch(S1, (width - nw) / 2, (height - nh) / 2, nw, nh, 0, 0, width, height);
		}
		else
		{
			float nw = std::floor(width * p), nh = std::floor(height * p);
			Blit(S1);
			Stretch(S2, (width - nw) / 2, (height - nh) / 2, nw, nh, 0, 0, width, height);
		}
	}
};

// Zoom 2
class TransZoom2 : public Transition
{
	int style = 0;
protected:
	void ReadParameters(ParameterReader& r) override { style = r.ReadInt(); }
	void StepDraw(float elapsed) override
	{
		float p = Progress(elapsed);
		if (style == 0)
		{
			float w = width * p, h = height * p;
			Stretch(S2, 0, 0, width, height, width / 2 - w / 2, height / 2 - h / 2, w, h);
		}
		else
		{
			float w = width - width * p, h = height - height * p;
			Stretch(S1, 0, 0, width, height, width / 2 - w / 2, height / 2 - h / 2, w, h);
		}
	}
};

}

std::unique_ptr<Transition> Transition::Create(const TransitionData& data)
{
	const std::string& id = data.Id;
	if (id == "BAND") return std::make_unique<TransBand>();
	if (id == "SE00") return std::make_unique<TransAdvancedScrolling>();
	if (id == "SE10") return std::make_unique<TransBack>();
	if (id == "SE12") return std::make_unique<TransCell>();
	if (id == "DOOR") return std::make_unique<TransDoor>();
	if (id == "SE03") return std::make_unique<TransLine>();
	if (id == "MOSA") return std::make_unique<TransMosaic>();
	if (id == "SE05") return std::make_unique<TransOpen>();
	if (id == "SE06") return std::make_unique<TransPush>();
	if (id == "SCRL") return std::make_unique<TransScroll>();
	if (id == "SE01") return std::make_unique<TransSquare>();
	if (id == "SE07") return std::make_unique<TransStretch>();
	if (id == "SE09") return std::make_unique<TransStretch2>();
	if (id == "SE13") return std::make_unique<TransTrame>();
	if (id == "SE08") return std::make_unique<TransTurn>();
	if (id == "SE02") return std::make_unique<TransTurn2>();
	if (id == "ZIGZ") return std::make_unique<TransZigZag>();
	if (id == "SE04") return std::make_unique<TransZigZag2>();
	if (id == "ZOOM") return std::make_unique<TransZoom>();
	if (id == "SE11") return std::make_unique<TransZoom2>();
	if (id == "FADE") return std::make_unique<TransFade>();

	// fallback to fade
	return std::make_unique<TransFade>();
}
