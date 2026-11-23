#include "WindowControlExtension.h"
#include "Application.h"

#ifdef NUCLEAR_BACKEND_SDL2
#include "SDL2GraphicsBackend.h"
#elif defined(NUCLEAR_BACKEND_SDL3)
#include "SDL3GraphicsBackend.h"
#endif

static void GetFrameWindowSize(int& width, int& height)
{
	width = 0;
	height = 0;

	auto backend = Application::Instance().GetBackend();
#ifdef NUCLEAR_BACKEND_SDL2
	SDL_FRect rect = static_cast<SDL2GraphicsBackend*>(backend->graphics)->CalculateRenderTargetRect();
#elif defined(NUCLEAR_BACKEND_SDL3)
	SDL_FRect rect = static_cast<SDL3GraphicsBackend*>(backend->graphics)->CalculateRenderTargetRect();
#else
	return;
#endif

	width = static_cast<int>(rect.w);
	height = static_cast<int>(rect.h);
}

CValue WindowControlExtension::GetFrameWinXSize()
{
	int width, height;
	GetFrameWindowSize(width, height);
	return CValue(width);
}

CValue WindowControlExtension::GetFrameWinYSize()
{
	int width, height;
	GetFrameWindowSize(width, height);
	return CValue(height);
}
