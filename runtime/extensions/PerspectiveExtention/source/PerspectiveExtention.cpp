#include "PerspectiveExtention.h"

#include <algorithm>

#include "Active.h"
#include "Application.h"
#include "GraphicsBackend.h"
#include "Frame.h"
#ifdef NUCLEAR_BACKEND_SDL3
#include "SDL3GraphicsBackend.h"
#endif

#ifdef __EMSCRIPTEN__
#define SHADER_PREFIX "#version 300 es\r\nprecision highp float;\r\n"
#else
#define SHADER_PREFIX "#version 330 core\r\n"
#endif

const std::string panoramaShader = R"(
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;

uniform float fB;
uniform int pDir;

void main() {
    vec2 posTex;
	float fC;
	
	if(pDir == 0)
	{
		fC =  max(0.02, 1.0+(fB - 1.0)*4.0*pow((TexCoord.s-0.5),2.0));
		posTex = TexCoord * vec2(1.0, fC) + vec2(0.0, (1.0-fC)/2.0);
	}
	else
	{
		fC =  max(0.05, 1.0+(fB - 1.0)*4.0*pow((TexCoord.t-0.5),2.0));
		posTex = TexCoord * vec2(fC, 1.0) + vec2((1.0-fC)/2.0, 0.0);
	}
	
	FragColor = texture(uTexture, posTex);
}
)";

const std::string perspectiveShader = R"(
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;

uniform float fA;
uniform float fB;
uniform int pDir;

void main()
{
	vec2 posTex;

	if (pDir != 0)
	{
    	float ScreenX = (fA-(fA-fB)*(1.0-TexCoord.y));
		posTex = TexCoord * vec2(ScreenX, 1.0) + vec2((1.0-ScreenX)/2.0, 0.0);
	}
	else
	{
		float ScreenY = (fA-(fA-fB)*TexCoord.x);
		posTex = TexCoord * vec2(1.0, ScreenY) + vec2(0.0, (1.0-ScreenY)/2.0);
	}
	
	FragColor = texture(uTexture, posTex);
}
)";

const std::string sineWavesShader = R"(
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;

uniform float Zoom;
uniform float WaveIncrement;
uniform float Offset;
uniform int pDir;

#define delta 3.141592/180.0

void main()
{
    vec2 posTex;

    if(pDir != 0)
    {
        float y= 1.0 - TexCoord.y;

        float ScreenX = 1.0 + sin((y*WaveIncrement+Offset)*delta)*Zoom-Zoom;
        posTex = TexCoord * vec2(ScreenX, 1.0) + vec2((1.0-ScreenX)/2.0, 0.0);
    }
    else
    {
        float ScreenY = 1.0 + sin((TexCoord.x*WaveIncrement+Offset)*delta)*Zoom-Zoom;
        posTex = TexCoord * vec2(1.0, ScreenY) + vec2(0.0, (1.0-ScreenY)/2.0);
    }

    FragColor = texture(uTexture, posTex);
}
)";

const std::string sineOffsetShader = R"(
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;

uniform float Zoom;
uniform float WaveIncrement;
uniform float Offset;
uniform int pDir;

#define delta 3.141592/180.0

void main()
{
    vec2 posTex;

    if(pDir == 0)
    {
        mediump float y = 1.0 - TexCoord.y;

        float ScreenX = 1.0 + sin((y*WaveIncrement+Offset)*delta)*Zoom;
        posTex = (TexCoord + vec2((1.0-ScreenX)/2.0, 0.0));
    }
    else
    {
        mediump float x = TexCoord.x*WaveIncrement+Offset;

        float ScreenY = 1.0 - sin(x*delta)*Zoom;
        posTex = (TexCoord+ vec2(0.0, (2.0-2.0*ScreenY)/2.0));
    }

    FragColor = texture(uTexture, posTex);
}
)";

void PerspectiveExtention::Initialize()
{
    SetEffectType(effect);
}

void PerspectiveExtention::SetEffectType(char effect)
{
	#ifdef NUCLEAR_BACKEND_SDL3
	auto* sdl3graphics = dynamic_cast<SDL3GraphicsBackend*>(Application::Instance().GetBackend()->graphics);
	if (!sdl3graphics) {
		return;
    }
	
	if (effectInstance) {
		delete effectInstance;
		effectInstance = nullptr;
	}

    switch (effect) {
		case 0: // Panorama
			sdl3graphics->LoadShader("panorama", SHADER_PREFIX + panoramaShader);
			effectInstance = new EffectInstance("panorama", {
				::EffectParameter("fB", 1, 0.0f),
				::EffectParameter("pDir", 0, 0),
			});
		break;
        case 1: // Perspective
			sdl3graphics->LoadShader("perspective", SHADER_PREFIX + perspectiveShader);
			effectInstance = new EffectInstance("perspective", {
				::EffectParameter("fA", 1, 0.0f),
				::EffectParameter("fB", 1, 0.0f),
				::EffectParameter("pDir", 0, 0),
			});
		break;
		case 2: // Sine Waves
			sdl3graphics->LoadShader("sineWaves", SHADER_PREFIX + sineWavesShader);
			effectInstance = new EffectInstance("sineWaves", {
				::EffectParameter("Zoom", 1, 0.0f),
				::EffectParameter("WaveIncrement", 1, 0.0f),
				::EffectParameter("Offset", 1, 0.0f),
				::EffectParameter("pDir", 0, 0),
			});
		break;
		case 3: // Sine Offset
			sdl3graphics->LoadShader("sineOffset", SHADER_PREFIX + sineOffsetShader);
			effectInstance = new EffectInstance("sineOffset", {
				::EffectParameter("Zoom", 1, 0.0f),
				::EffectParameter("WaveIncrement", 1, 0.0f),
				::EffectParameter("Offset", 1, 0.0f),
				::EffectParameter("pDir", 0, 0),
			});
		break;
    }
	#endif

	this->effect = effect;
	UpdateShaderParameters();
}

void PerspectiveExtention::SetDirection(bool direction)
{
    this->direction = direction;
	UpdateShaderParameters();
}

void PerspectiveExtention::SetPerspectiveDirection(bool perspectiveDir)
{
    this->perspectiveDir = perspectiveDir;
	UpdateShaderParameters();
}

void PerspectiveExtention::SetWidth(int width)
{
	this->width = width;
	UpdateShaderParameters();
}

void PerspectiveExtention::SetHeight(int height)
{
	this->height = height;
	UpdateShaderParameters();
}

void PerspectiveExtention::SetZoomValue(int zoomValue)
{
	this->zoomValue = zoomValue;
	UpdateShaderParameters();
}

void PerspectiveExtention::SetOffset(int offset)
{
	this->offset = offset;
	UpdateShaderParameters();
}

void PerspectiveExtention::SetSineWaveWaves(int sineWaveWaves)
{
	this->sineWaveWaves = sineWaveWaves;
	UpdateShaderParameters();
}

void PerspectiveExtention::UpdateShaderParameters()
{	
    if (effectInstance) {
		int objSize = direction == 0 ? height : width;

        effectInstance->SetParameter("pDir", direction ? 1 : 0);

		if (effect == 0)
		{
			effectInstance->SetParameter("fB", std::max(1.0f, (float)(objSize - zoomValue)) / std::max(1.0f, (float)objSize));
		}
		else if (effect == 1)
		{
			float slope1 = (((float) objSize / ((float) (zoomValue + objSize) / (float) objSize)) + 0.5) / (float) objSize;
			float slope2 = ((float) objSize + 0.5) / (float) objSize;
			effectInstance->SetParameter("fA", perspectiveDir ? slope1 : slope2);
			effectInstance->SetParameter("fB", perspectiveDir ? slope2 : slope1);
		}
		else if (effect == 2)
		{
			int objSize2 = direction == 0 ? width : height;

			effectInstance->SetParameter("Zoom", (float)zoomValue / (float)objSize);
			effectInstance->SetParameter("WaveIncrement", ((float) (sineWaveWaves * 360) / (float) height) * (float) objSize2);
			effectInstance->SetParameter("Offset", (float)offset);
		}
		else if (effect == 3)
		{
			effectInstance->SetParameter("Zoom", (float)zoomValue / (float)height);
			effectInstance->SetParameter("WaveIncrement", ((float) (sineWaveWaves * 360) / (float) height) * (float) objSize);
			effectInstance->SetParameter("Offset", (float)offset);
		}
    }
}

void PerspectiveExtention::Draw()
{
    Application::Instance().GetBackend()->graphics->DrawEffectRect(X, Y, width, height, 0, 0, 0, effectInstance);
}