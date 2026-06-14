#if defined(PLATFORM_IOS) && defined(NUCLEAR_BACKEND_SDL3)
#include <SDL3/SDL_main.h>
#endif

#include "Application.h"

#if defined(_MSC_VER) && !defined(_DEBUG)
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#endif

int main(int argc, char *argv[])
{
	Application& app = Application::Instance();
	app.Initialize();
	app.Run();
	return 0;
}
