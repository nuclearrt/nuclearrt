#pragma once

#include <memory>
#include <random>
#include <string>
#include <vector>

class GraphicsBackend;

constexpr int TRANSITION_FLAG_COLOR = 0x0001;

struct TransitionData
{
	TransitionData(std::string id, int duration, int flags, int color, std::vector<unsigned char> parameters)
		: Id(std::move(id)), Duration(duration), Flags(flags), Color(color), Parameters(std::move(parameters)) {}

	std::string Id;
	int Duration = 0;
	int Flags = 0;
	int Color = 0;
	std::vector<unsigned char> Parameters;
};

enum TransitionSurface
{
	TRANSITION_SURFACE_START = 0,
	TRANSITION_SURFACE_END = 1
};

class Transition
{
public:
	virtual ~Transition() = default;

	static std::unique_ptr<Transition> Create(const TransitionData& data);

	void Start(const TransitionData& data, GraphicsBackend* graphics, bool fadeOut, unsigned int nowMs, int width, int height);
	void Step(unsigned int nowMs);
	void Finish();
	bool IsCompleted(unsigned int nowMs) const;
	bool IsFadeOut() const { return fadeOut; }

protected:
	class ParameterReader
	{
	public:
		explicit ParameterReader(const std::vector<unsigned char>& data) : data(data) {}
		int ReadInt();
		short ReadShort();
	private:
		const std::vector<unsigned char>& data;
		size_t position = 0;
	};

	virtual void ReadParameters(ParameterReader& reader) {}
	virtual void OnStart() {}
	virtual void StepDraw(float elapsed) = 0;

	void Blit(int surface);
	void Blit(int surface, float xDest, float yDest, float xSrc, float ySrc, float width, float height);
	void Stretch(int surface, float xDest, float yDest, float wDest, float hDest, float xSrc, float ySrc, float wSrc, float hSrc, float alpha = 1.0f);
	void BlitPattern(int surface, int pattern, float revealWidth, float revealHeight);
	float Progress(float elapsed) const { return elapsed / duration; }
	int RandomInt(int max);

	GraphicsBackend* graphics = nullptr;
	float width = 0.0f;
	float height = 0.0f;
	float duration = 1.0f;
	bool fadeOut = false;

private:
	unsigned int startTime = 0;
	std::mt19937 rng;
};
