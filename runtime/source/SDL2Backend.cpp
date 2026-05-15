#ifdef NUCLEAR_BACKEND_SDL2

#include "SDL2Backend.h"

#include <iostream>

#include "Application.h"
#include "FontBank.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#ifdef _DEBUG
#include "DebugUI.h"
#include <imgui_impl_sdl2.h>
#endif

#ifdef __SWITCH__
#include <switch.h>
#endif

SDL2Backend::SDL2Backend() {
}

SDL2Backend::~SDL2Backend() {
	Deinitialize();
}

void SDL2Backend::Initialize() {
	int windowWidth = Application::Instance().GetAppData()->GetWindowWidth();
	int windowHeight = Application::Instance().GetAppData()->GetWindowHeight();
	std::string windowTitle = Application::Instance().GetAppData()->GetAppName();

	#ifdef __SWITCH__
		romfsInit();
	#endif
	
	// Initialize SDL2
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return;
	}
	
	// Initialize SDL2_image
	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
		std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
		return;
	}

	// Initialize SDL2_mixer
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		std::cerr << "Mix_OpenAudio Error: " << Mix_GetError() << std::endl;
		return;
	}

	// Initialize SDL2_ttf
	if (TTF_Init() == -1) {
		std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
		return;
	}

	// Create the window
	window = SDL_CreateWindow(windowTitle.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowWidth, windowHeight, SDL_WINDOW_SHOWN);
	if (window == nullptr) {
		std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		return;
	}

	// Create the renderer
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == nullptr) {
		std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		return;
	}

	//load assets
	if (!pakFile.Load(GetAssetsFileName())) {
		std::cerr << "PakFile::Load Error: " << "Failed to load assets file" << std::endl;
		return;
	}

#ifdef _DEBUG
	DEBUG_UI.Initialize(window, renderer);
	
	DEBUG_UI.AddWindow(Application::Instance().GetAppData()->GetAppName(), [this]() {
		ImGui::Text("Platform: %s", GetPlatformName().c_str());
		ImGui::Text("Assets File: %s", GetAssetsFileName().c_str());
		
		if(ImGui::CollapsingHeader("Global Variables")) {
			if (ImGui::CollapsingHeader("Values")) {
				std::vector<int>& altValues = Application::Instance().GetAppData()->GetGlobalValues();
				for (int i = 0; i < altValues.size(); i++) {
					ImGui::InputInt(("Value " + std::to_string(i)).c_str(), &altValues[i]);
				}
			}
			if (ImGui::CollapsingHeader("Strings")) {
				std::vector<std::string>& altStrings = Application::Instance().GetAppData()->GetGlobalStrings();
				for (int i = 0; i < altStrings.size(); i++) {
					char buffer[256];
					strncpy(buffer, altStrings[i].c_str(), sizeof(buffer) - 1);
					buffer[sizeof(buffer) - 1] = '\0';
					if (ImGui::InputText(("String " + std::to_string(i)).c_str(), buffer, sizeof(buffer))) {
						altStrings[i] = buffer;
					}
				}
			}
		}

		//jump to frame
		static int frameIndex = 0;
		ImGui::InputInt("Frame Index", &frameIndex);
		if (ImGui::Button("Jump to Frame")) {
			Application::Instance().QueueStateChange(GameState::JumpToFrame, frameIndex);
		}

		if (ImGui::CollapsingHeader("Current Frame")) {
			Frame* currentFrame = Application::Instance().GetCurrentFrame().get();
			ImGui::Text("Current Frame: %s", currentFrame->Name.c_str());
			ImGui::Text("Current Frame Index: %d", currentFrame->Index);

			if (ImGui::TreeNode("Object Instances")) {
				int i = 0;
				for (auto& instance : currentFrame->ObjectInstances) {					
					if (ImGui::TreeNode(std::string(instance->OI->Name + "##" + std::to_string(i)).c_str())) {
						ImGui::Text("Handle: %d", instance->Handle);
						ImGui::Text("X: %d", instance->X);
						ImGui::Text("Y: %d", instance->Y);

						if (ImGui::TreeNode("OI")) {
							ImGui::Text("Handle: %d", instance->OI->Handle);
							ImGui::Text("Type: %d", instance->OI->Type);
							ImGui::Text("RGB Coefficient: %d", instance->OI->RGBCoefficient);
							ImGui::Text("Effect: %d", instance->OI->Effect);
							ImGui::Text("Effect Parameter: %d", instance->OI->GetEffectParameter());
							ImGui::TreePop();
						}

						ImGui::TreePop();
					}

					if (ImGui::IsItemHovered()) {
						if (instance->Type != 0 && instance->Type != 1 && instance->Type != 2) {
							DrawRectangle(instance->X, instance->Y, 32, 32, 0xFFFF0000);
						}
						else {
							unsigned int imageId = 0;
							if (instance->Type == 1) // Backdrop
							{
								imageId = ((Backdrop*)instance)->Image;
							}
							else if (instance->Type == 0) // Quick backdrop
							{
								imageId = ((QuickBackdrop*)instance)->shape.Image;
							}
							else {
								imageId = ((Active*)instance)->animations.GetCurrentImageHandle();
							}
							auto imageInfo = ImageBank::Instance().GetImage(imageId);

							if (imageInfo) {
								DrawRectangle(instance->X, instance->Y, imageInfo->Width, imageInfo->Height, 0xFFFF0000);
							}
							else {
								DrawRectangle(instance->X, instance->Y, 32, 32, 0xFFFF0000);
							}

						}
					}

					i++;
				}
				ImGui::TreePop();
			}
		}
	});
#endif
}

void SDL2Backend::Deinitialize()
{
#ifdef _DEBUG
	DEBUG_UI.Shutdown();
#endif

	// Destroy the renderer
	if (renderer != nullptr) {
		SDL_DestroyRenderer(renderer);
		renderer = nullptr;
	}
	
	// Destroy the window
	if (window != nullptr) {
		SDL_DestroyWindow(window);
		window = nullptr;
	}

	Mix_CloseAudio();
	
	IMG_Quit();

	TTF_Quit();
	
	SDL_Quit();
}

bool SDL2Backend::ShouldQuit()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
#ifdef _DEBUG
		// Process ImGui events
		if (DEBUG_UI.IsEnabled()) {
			ImGui_ImplSDL2_ProcessEvent(&event);
		}
		
		// Toggle debug UI with F1 key
		if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F1 && event.key.repeat == 0) {
			DEBUG_UI.ToggleEnabled();
		}
#endif

#ifdef INPUT_TOUCH
		// handle touch events
		if (event.type == SDL_FINGERDOWN || event.type == SDL_FINGERMOTION) {
			int windowWidth, windowHeight;
			SDL_GetWindowSize(window, &windowWidth, &windowHeight);
			touchX = static_cast<int>(event.tfinger.x * windowWidth);
			touchY = static_cast<int>(event.tfinger.y * windowHeight);
			touchDown = true;
		}
		else if (event.type == SDL_FINGERUP) {
			touchDown = false;
		}
#endif

		if (event.type == SDL_QUIT) {
			return true;
		}
	}
	return false;
}

std::string SDL2Backend::GetPlatformName()
{
#if defined(PLATFORM_WINDOWS)
	return "Windows";
#elif defined(PLATFORM_MACOS)
	return "macOS";
#elif defined(PLATFORM_LINUX)
	return "Linux";
#elif defined(PLATFORM_SWITCH)
	return "Switch";
#else
	return "Unknown";
#endif
}

std::string SDL2Backend::GetAssetsFileName()
{
#if defined(PLATFORM_SWITCH)
	return "romfs:/assets.pak";
#else
	return "assets.pak";
#endif
}

void SDL2Backend::BeginDrawing()
{
	SDL_Colour borderColor = RGBToSDLColor(Application::Instance().GetAppData()->GetBorderColor());
	SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
	SDL_RenderClear(renderer);

#ifdef _DEBUG
	DEBUG_UI.BeginFrame();
#endif
}

void SDL2Backend::EndDrawing()
{
#ifdef _DEBUG
	DEBUG_UI.EndFrame();
#endif

	SDL_RenderPresent(renderer);
}

void SDL2Backend::Clear(int color)
{
	SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
	SDL_RenderClear(renderer);
}

void SDL2Backend::LoadTexture(int id) {
	//Check if texture is already loaded
	if (textures.find(id) != textures.end()) {
		return;
	}

	//load texture from pak file
	std::vector<uint8_t> data = pakFile.GetData("images/" + std::to_string(id) + ".png");
	if (data.empty()) {
		std::cerr << "PakFile::GetData Error: " << "Image with id " << id << " not found" << std::endl;
		return;
	}

	//create surface from data
	SDL_RWops* stream = SDL_RWFromMem(data.data(), data.size());
	SDL_Surface* surface = IMG_Load_RW(stream, true);
	if (surface == nullptr) {
		std::cerr << "IMG_Load Error: " << IMG_GetError() << std::endl;
		return;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	textures[id] = texture;
}

void SDL2Backend::UnloadTexture(int id) {
	SDL_DestroyTexture(textures[id]);
	textures.erase(id);
}

void SDL2Backend::DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scale, int color, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
{
	SDL_Texture* texture = textures[id];
	if (texture == nullptr) {
		return;
	}
	
	// Save original texture properties
	Uint8 origR, origG, origB, origA;
	SDL_BlendMode origBlendMode;
	SDL_GetTextureColorMod(texture, &origR, &origG, &origB);
	SDL_GetTextureAlphaMod(texture, &origA);
	SDL_GetTextureBlendMode(texture, &origBlendMode);
	
	// Apply new color
	Uint8 r = (color >> 16) & 0xFF;
	Uint8 g = (color >> 8) & 0xFF;
	Uint8 b = color & 0xFF;
	SDL_SetTextureColorMod(texture, r, g, b);
	
	//get texture dimensions
	int width, height;
	SDL_QueryTexture(texture, nullptr, nullptr, &width, &height);
	SDL_Rect rect = { x - offsetX, y - offsetY, width, height };
	
	//Effects
	switch (effect) {
		case 4096:
		case 0:
			SDL_SetTextureAlphaMod(texture, 255 - effectParameter);
			break;
		case 1: // Semi-Transparent:
			SDL_SetTextureColorMod(texture, 255, 255, 255);
			SDL_SetTextureAlphaMod(texture, 255 - effectParameter);
			break;
		case 9: // Additive
			SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_ADD);
			SDL_SetTextureAlphaMod(texture, 255 - effectParameter);
			break;
	}

	SDL_Point center = { offsetX, offsetY };
	SDL_RenderCopyEx(renderer, texture, nullptr, &rect, 360 - angle, &center, SDL_FLIP_NONE);
	
	// Restore original texture properties
	SDL_SetTextureColorMod(texture, origR, origG, origB);
	SDL_SetTextureAlphaMod(texture, origA);
	SDL_SetTextureBlendMode(texture, origBlendMode);
}

void SDL2Backend::DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape)
{
	//TODO: Borders
	//TODO: Ellipse masks
	if (shape->ShapeType == 1) { // Line
		SDL_SetRenderDrawColor(renderer, (shape->BorderColor >> 16) & 0xFF, (shape->BorderColor >> 8) & 0xFF, shape->BorderColor & 0xFF, SDL_ALPHA_OPAQUE);

		int x1 = shape->FlipX ? x + width : x;
		int y1 = shape->FlipY ? y + height : y;
		int x2 = shape->FlipX ? x : x + width;
		int y2 = shape->FlipY ? y : y + height;

		//TODO: BorderSize
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}
	else {
		if (shape->FillType == 1) { // Solid Color
			SDL_SetRenderDrawColor(renderer, (shape->Color1 >> 16) & 0xFF, (shape->Color1 >> 8) & 0xFF, shape->Color1 & 0xFF, SDL_ALPHA_OPAQUE);
			SDL_Rect rect = { x, y, width, height };
			SDL_RenderFillRect(renderer, &rect);
		}
		else if (shape->FillType == 2) { // Gradient
			Uint8 r1 = (shape->Color1 >> 16) & 0xFF;
			Uint8 g1 = (shape->Color1 >> 8) & 0xFF;
			Uint8 b1 = shape->Color1 & 0xFF;
			
			Uint8 r2 = (shape->Color2 >> 16) & 0xFF;
			Uint8 g2 = (shape->Color2 >> 8) & 0xFF;
			Uint8 b2 = shape->Color2 & 0xFF;
			
			if (shape->VerticalGradient) {
				// Vertical gradient (top to bottom)
				for (int i = 0; i < height; i++) {
					float ratio = static_cast<float>(i) / static_cast<float>(height);
					
					Uint8 r = static_cast<Uint8>(r1 + (r2 - r1) * ratio);
					Uint8 g = static_cast<Uint8>(g1 + (g2 - g1) * ratio);
					Uint8 b = static_cast<Uint8>(b1 + (b2 - b1) * ratio);
					
					SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
					SDL_RenderDrawLine(renderer, x, y + i, x + width - 1, y + i);
				}
			} else {
				// Horizontal gradient (left to right)
				for (int i = 0; i < width; i++) {
					float ratio = static_cast<float>(i) / static_cast<float>(width);
					
					Uint8 r = static_cast<Uint8>(r1 + (r2 - r1) * ratio);
					Uint8 g = static_cast<Uint8>(g1 + (g2 - g1) * ratio);
					Uint8 b = static_cast<Uint8>(b1 + (b2 - b1) * ratio);
					
					SDL_SetRenderDrawColor(renderer, r, g, b, SDL_ALPHA_OPAQUE);
					SDL_RenderDrawLine(renderer, x + i, y, x + i, y + height - 1);
				}
			}
		}
		else if (shape->FillType == 3) { // Motif
			SDL_Texture* texture = textures[shape->Image];
			if (texture == nullptr) {
				return;
			}
			
			int textureWidth, textureHeight;
			SDL_QueryTexture(texture, nullptr, nullptr, &textureWidth, &textureHeight);
			
			// Tile the texture across the entire area
			for (int tileY = y; tileY < y + height; tileY += textureHeight) {
				for (int tileX = x; tileX < x + width; tileX += textureWidth) {
					// Calculate the width and height of this tile (might be smaller at edges)
					int tileW = std::min(textureWidth, x + width - tileX);
					int tileH = std::min(textureHeight, y + height - tileY);
					
					SDL_Rect destRect = { tileX, tileY, tileW, tileH };
					SDL_Rect srcRect = { 0, 0, tileW, tileH };
					SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
				}
			}
		}
	}
}

void SDL2Backend::DrawRectangle(int x, int y, int width, int height, int color)
{
	SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
	SDL_Rect rect = { x, y, width, height };
	SDL_RenderFillRect(renderer, &rect);
}

void SDL2Backend::DrawRectangleLines(int x, int y, int width, int height, int color)
{
	SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
	SDL_Rect rect = { x, y, width, height };
	SDL_RenderDrawRect(renderer, &rect);
}

void SDL2Backend::DrawLine(int x1, int y1, int x2, int y2, int color)
{
	SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
	SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

void SDL2Backend::DrawPixel(int x, int y, int color)
{
	SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
	SDL_RenderDrawPoint(renderer, x, y);
}

void SDL2Backend::LoadFont(int id)
{
	//check if font already exists
	if (fonts.find(id) != fonts.end()) {
		return;
	}

	//get font info
	FontInfo* fontInfo = FontBank::Instance().GetFont(id);
	if (fontInfo == nullptr) {
		std::cerr << "FontBank::GetFont Error: " << "Font with id " << id << " not found" << std::endl;
		return;
	}

	SDL_RWops* stream;

	//if buffer is already loaded, use it
	if (fontBuffers.find(fontInfo->FontFileName) != fontBuffers.end()) {
		stream = SDL_RWFromMem(fontBuffers[fontInfo->FontFileName]->data(), fontBuffers[fontInfo->FontFileName]->size());
	}
	else {
		//load buffer from pak file
		std::shared_ptr<std::vector<uint8_t>> buffer = std::make_shared<std::vector<uint8_t>>(pakFile.GetData("fonts/" + fontInfo->FontFileName));
		if (buffer->empty()) {
			std::cerr << "PakFile::GetData Error: " << "Font with file name " << fontInfo->FontFileName << " not found" << std::endl;
			return;
		}
		stream = SDL_RWFromMem(buffer->data(), buffer->size());
		fontBuffers[fontInfo->FontFileName] = buffer;
	}

	TTF_Font* font = TTF_OpenFontRW(stream, true, static_cast<float>(fontInfo->Height));
	if (!font) {
		std::cerr << "TTF_OpenFontRW Error: " << SDL_GetError() << std::endl;
		return;
	}
	
	//render flags
	int renderFlags = TTF_STYLE_NORMAL;
	if (fontInfo->Weight > 500) {
		renderFlags |= TTF_STYLE_BOLD;
	}
	if (fontInfo->Italic) {
		renderFlags |= TTF_STYLE_ITALIC;
	}
	if (fontInfo->Underline) {
		renderFlags |= TTF_STYLE_UNDERLINE;
	}
	if (fontInfo->Strikeout) {
		renderFlags |= TTF_STYLE_STRIKETHROUGH;
	}	

	TTF_SetFontStyle(font, renderFlags);

	fonts[id] = font;
}

void SDL2Backend::UnloadFont(int id)
{
	auto it = fonts.find(id);
	if (it != fonts.end()) {
		// Find the FontInfo associated with this font id to remove buffer
		FontInfo* fontInfo = FontBank::Instance().GetFont(id);
		if (fontInfo != nullptr) {
			// Check if any other loaded font is using the same buffer
			bool bufferUsedByOtherFont = false;
			for (const auto& pair : fonts) {
				if (pair.first != id) {
					FontInfo* otherFontInfo = FontBank::Instance().GetFont(pair.first);
					if (otherFontInfo && otherFontInfo->FontFileName == fontInfo->FontFileName) {
						bufferUsedByOtherFont = true;
						break;
					}
				}
			}
			if (!bufferUsedByOtherFont) {
				fontBuffers.erase(fontInfo->FontFileName);
			}
		}
		
		TTF_CloseFont(it->second);
		fonts.erase(it);
	}
}

void SDL2Backend::DrawText(FontInfo* fontInfo, int x, int y, int color, const std::string& text, int objectHandle, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
{
	(void)objectHandle;
	(void)rgbCoefficient;
	(void)effect;
	(void)effectParameter;
	(void)effectInstance;

	TTF_Font* font = fonts[fontInfo->Handle];
	if (font == nullptr) {
		return;
	}

	//remove \r from text
	std::string modifiedText = text;
	modifiedText.erase(std::remove(modifiedText.begin(), modifiedText.end(), '\r'), modifiedText.end());

	//make tabs 4 spaces
	for (size_t i = 0; i < modifiedText.size(); i++) {
		if (modifiedText[i] == '\t') {
			modifiedText.replace(i, 1, "    ");
		}
	}

	//Check if text is empty/just whitespace
	if (modifiedText.find_first_not_of(" \n\r\t") == std::string::npos) {
		return;
	}

	SDL_Surface* surface = TTF_RenderUTF8_Blended_Wrapped(font, modifiedText.c_str(), RGBToSDLColor(color), fontInfo->Width);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_Rect rect = { x, y, surface->w, surface->h };
	SDL_RenderCopy(renderer, texture, nullptr, &rect);
	SDL_FreeSurface(surface);
	SDL_DestroyTexture(texture);
}

void SDL2Backend::GetKeyboardState(uint8_t* outBuffer)
{
	//return the keyboard state in a new array which matches the Fusion key codes
	const uint8_t* keyboardState = SDL_GetKeyboardState(nullptr);
	for (int i = 0; i < 256; i++)
	{
		outBuffer[i] = keyboardState[FusionToSDLKey(i)];
	}
}

int SDL2Backend::GetMouseX()
{
#ifdef INPUT_TOUCH
	if (touchDown) {
		return touchX;
	}
#endif
	int x, windowX;
	SDL_GetWindowPosition(window, &windowX, nullptr);
	SDL_GetGlobalMouseState(&x, nullptr);
	return x - windowX;
}

int SDL2Backend::GetMouseY()
{
#ifdef INPUT_TOUCH
	if (touchDown) {
		return touchY;
	}
#endif
	int y, windowY;
	SDL_GetWindowPosition(window, nullptr, &windowY);
	SDL_GetGlobalMouseState(nullptr, &y);
	return y - windowY;
}

int SDL2Backend::GetMouseWheelMove()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_MOUSEWHEEL) {
			return event.wheel.y;
		}
	}
	return 0;
}

uint32_t SDL2Backend::GetMouseState()
{
#ifdef INPUT_TOUCH
	if (touchDown) {
		return SDL_BUTTON(SDL_BUTTON_LEFT);
	}
#endif
	return SDL_GetMouseState(nullptr, nullptr);
}

void SDL2Backend::HideMouseCursor()
{
	SDL_ShowCursor(SDL_DISABLE);
}

void SDL2Backend::ShowMouseCursor()
{
	SDL_ShowCursor(SDL_ENABLE);
}

SDL_Colour SDL2Backend::RGBToSDLColor(int color)
{
	return SDL_Colour{
		static_cast<Uint8>((color >> 16) & 0xFF),
		static_cast<Uint8>((color >> 8) & 0xFF),
		static_cast<Uint8>(color & 0xFF),
		255
	};
}

SDL_Colour SDL2Backend::RGBAToSDLColor(int color)
{
	return SDL_Colour{
		static_cast<Uint8>((color >> 16) & 0xFF),
		static_cast<Uint8>((color >> 8) & 0xFF),
		static_cast<Uint8>(color & 0xFF),
		static_cast<Uint8>((color >> 24) & 0xFF)
	};
}

int SDL2Backend::FusionToSDLKey(short key)
{
	switch (key)
	{
		default:
			return SDL_SCANCODE_UNKNOWN;
		case 0x08:
			return SDL_SCANCODE_BACKSPACE;
		case 0x09:
			return SDL_SCANCODE_TAB;
		case 0x0D:
			return SDL_SCANCODE_RETURN;
		case 0x10:
			return SDL_SCANCODE_LSHIFT;
		case 0x11:
			return SDL_SCANCODE_LCTRL;
		case 0x13:
			return SDL_SCANCODE_PAUSE;
		case 0x14:
			return SDL_SCANCODE_CAPSLOCK;
		case 0x1B:
			return SDL_SCANCODE_ESCAPE;
		case 0x20:
			return SDL_SCANCODE_SPACE;
		case 0x21:
			return SDL_SCANCODE_PAGEUP;
		case 0x22:
			return SDL_SCANCODE_PAGEDOWN;
		case 0x23:
			return SDL_SCANCODE_END;
		case 0x24:
			return SDL_SCANCODE_HOME;
		case 0x25:
			return SDL_SCANCODE_LEFT;
		case 0x26:
			return SDL_SCANCODE_UP;
		case 0x27:
			return SDL_SCANCODE_RIGHT;
		case 0x28:
			return SDL_SCANCODE_DOWN;
		case 0x2D:
			return SDL_SCANCODE_INSERT;
		case 0x2E:
			return SDL_SCANCODE_DELETE;
		case 0x30:
			return SDL_SCANCODE_0;
		case 0x31:
			return SDL_SCANCODE_1;
		case 0x32:
			return SDL_SCANCODE_2;
		case 0x33:
			return SDL_SCANCODE_3;
		case 0x34:
			return SDL_SCANCODE_4;
		case 0x35:
			return SDL_SCANCODE_5;
		case 0x36:
			return SDL_SCANCODE_6;
		case 0x37:
			return SDL_SCANCODE_7;
		case 0x38:
			return SDL_SCANCODE_8;
		case 0x39:
			return SDL_SCANCODE_9;
		case 0x41:
			return SDL_SCANCODE_A;
		case 0x42:
			return SDL_SCANCODE_B;
		case 0x43:
			return SDL_SCANCODE_C;
		case 0x44:
			return SDL_SCANCODE_D;
		case 0x45:
			return SDL_SCANCODE_E;
		case 0x46:
			return SDL_SCANCODE_F;
		case 0x47:
			return SDL_SCANCODE_G;
		case 0x48:
			return SDL_SCANCODE_H;
		case 0x49:
			return SDL_SCANCODE_I;
		case 0x4A:
			return SDL_SCANCODE_J;
		case 0x4B:
			return SDL_SCANCODE_K;
		case 0x4C:
			return SDL_SCANCODE_L;
		case 0x4D:
			return SDL_SCANCODE_M;
		case 0x4E:
			return SDL_SCANCODE_N;
		case 0x4F:
			return SDL_SCANCODE_O;
		case 0x50:
			return SDL_SCANCODE_P;
		case 0x51:
			return SDL_SCANCODE_Q;
		case 0x52:
			return SDL_SCANCODE_R;
		case 0x53:
			return SDL_SCANCODE_S;
		case 0x54:
			return SDL_SCANCODE_T;
		case 0x55:
			return SDL_SCANCODE_U;
		case 0x56:
			return SDL_SCANCODE_V;
		case 0x57:
			return SDL_SCANCODE_W;
		case 0x58:
			return SDL_SCANCODE_X;
		case 0x59:
			return SDL_SCANCODE_Y;
		case 0x5A:
			return SDL_SCANCODE_Z;
		case 0x60:
			return SDL_SCANCODE_KP_0;
		case 0x61:
			return SDL_SCANCODE_KP_1;
		case 0x62:
			return SDL_SCANCODE_KP_2;
		case 0x63:
			return SDL_SCANCODE_KP_3;
		case 0x64:
			return SDL_SCANCODE_KP_4;
		case 0x65:
			return SDL_SCANCODE_KP_5;
		case 0x66:
			return SDL_SCANCODE_KP_6;
		case 0x67:
			return SDL_SCANCODE_KP_7;
		case 0x68:
			return SDL_SCANCODE_KP_8;
		case 0x69:
			return SDL_SCANCODE_KP_9;
		case 0x6A:
			return SDL_SCANCODE_KP_MULTIPLY;
		case 0x6B:
			return SDL_SCANCODE_KP_PLUS;
		case 0x6D:
			return SDL_SCANCODE_KP_MINUS;
		case 0x6E:
			return SDL_SCANCODE_KP_PERIOD;
		case 0x6F:
			return SDL_SCANCODE_KP_DIVIDE;
		case 0x70:
			return SDL_SCANCODE_F1;
		case 0x71:
			return SDL_SCANCODE_F2;
		case 0x72:
			return SDL_SCANCODE_F3;
		case 0x73:
			return SDL_SCANCODE_F4;
		case 0x74:
			return SDL_SCANCODE_F5;
		case 0x75:
			return SDL_SCANCODE_F6;
		case 0x76:
			return SDL_SCANCODE_F7;
		case 0x77:
			return SDL_SCANCODE_F8;
		case 0x78:
			return SDL_SCANCODE_F9;
		case 0x79:
			return SDL_SCANCODE_F10;
		case 0x7A:
			return SDL_SCANCODE_F11;
		case 0x7B:
			return SDL_SCANCODE_F12;
		case 0x90:
			return SDL_SCANCODE_NUMLOCKCLEAR;
		case 0xBA:
			return SDL_SCANCODE_SEMICOLON;
		case 0xBB:
			return SDL_SCANCODE_EQUALS;
		case 0xBC:
			return SDL_SCANCODE_COMMA;
		case 0xBD:
			return SDL_SCANCODE_MINUS;
		case 0xBE:
			return SDL_SCANCODE_PERIOD;
		case 0xBF:
			return SDL_SCANCODE_SLASH;
		case 0xC0:
			return SDL_SCANCODE_GRAVE;
		case 0xDB:
			return SDL_SCANCODE_LEFTBRACKET;
		case 0xDC:
			return SDL_SCANCODE_BACKSLASH;
		case 0xDD:
			return SDL_SCANCODE_RIGHTBRACKET;
		case 0xDE:
			return SDL_SCANCODE_APOSTROPHE;
	}
}

float SDL2Backend::GetTimeDelta()
{
    static Uint32 previousTicks = SDL_GetTicks();
    Uint32 currentTicks = SDL_GetTicks();
    float delta = (currentTicks - previousTicks) / 1000.0f;
    previousTicks = currentTicks;
    return delta;
}

void SDL2Backend::Delay(unsigned int ms)
{
    SDL_Delay(ms);
}
#endif