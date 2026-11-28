#ifdef NUCLEAR_BACKEND_SDL2

#include "SDL2GraphicsBackend.h"

#include <iostream>

#include "Application.h"
#include "FontBank.h"
#include "Frame.h"
#include "ImageBank.h"
#include "lz4.h"
#include "SDL2Backend.h"
#include "Shape.h"

#include <SDL.h>
#include <SDL_ttf.h>

#ifdef NUCLEAR_DEBUG_UI
#include "DebugUI.h"
#include <imgui_impl_sdl2.h>
#endif

#ifdef __SWITCH__
#include <switch.h>
#endif

void SDL2GraphicsBackend::Initialize() {
	int windowWidth = Application::Instance().GetAppData()->GetWindowWidth();
	int windowHeight = Application::Instance().GetAppData()->GetWindowHeight();
	std::string windowTitle = Application::Instance().GetAppData()->GetAppName();
	
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return;
	}

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
}

void SDL2GraphicsBackend::Deinitialize()
{
#ifdef NUCLEAR_DEBUG_UI
	DEBUG_UI.Shutdown();
#endif

	if (renderer != nullptr) {
		SDL_SetRenderTarget(renderer, nullptr);
	}

	if (renderTargetTexture != nullptr) {
		SDL_DestroyTexture(renderTargetTexture);
		renderTargetTexture = nullptr;
	}
	renderTargetWidth = 0;
	renderTargetHeight = 0;

	for (auto& pair : textures) {
		if (pair.second != nullptr) {
			SDL_DestroyTexture(pair.second);
		}
	}
	textures.clear();

	for (auto& pair : fonts) {
		if (pair.second != nullptr) {
			TTF_CloseFont(pair.second);
		}
	}
	fonts.clear();
	fontBuffers.clear();

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

	TTF_Quit();
	
	SDL_Quit();
}

void SDL2GraphicsBackend::BeginDrawing()
{
	if (renderer == nullptr) {
		return;
	}

	auto currentFrame = Application::Instance().GetCurrentFrame().get();
	if (!currentFrame) {
		return;
	}

	// resize render target if needed
	int targetWidth = std::min(Application::Instance().GetAppData()->GetWindowWidth(), currentFrame->Width);
	int targetHeight = std::min(Application::Instance().GetAppData()->GetWindowHeight(), currentFrame->Height);

	if (targetWidth != renderTargetWidth || targetHeight != renderTargetHeight)
		CreateRenderTarget(targetWidth, targetHeight);

	SDL_Colour borderColor = RGBToSDLColor(Application::Instance().GetAppData()->GetBorderColor());
	SDL_SetRenderDrawColor(renderer, borderColor.r, borderColor.g, borderColor.b, borderColor.a);
	SDL_RenderClear(renderer);

	if (renderTargetTexture != nullptr) {
		SDL_SetRenderTarget(renderer, renderTargetTexture);
		SDL_RenderClear(renderer);
	}

#ifdef NUCLEAR_DEBUG_UI
	DEBUG_UI.BeginFrame();
#endif
}

void SDL2GraphicsBackend::EndDrawing()
{
	if (renderer == nullptr) {
		return;
	}

#ifdef NUCLEAR_DEBUG_UI
	DEBUG_UI.EndFrame();
#endif

	SDL_SetRenderTarget(renderer, nullptr);

	if (renderTargetTexture != nullptr) {
		SDL_FRect rect = CalculateRenderTargetRect();
		SDL_Rect destRect = { static_cast<int>(rect.x), static_cast<int>(rect.y), static_cast<int>(rect.w), static_cast<int>(rect.h) };
		SDL_RenderCopy(renderer, renderTargetTexture, nullptr, &destRect);
	}

	SDL_RenderPresent(renderer);
}

void SDL2GraphicsBackend::Clear(int color)
{
	SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
	SDL_RenderClear(renderer);
}

void SDL2GraphicsBackend::CreateRenderTarget(int width, int height)
{
	if (renderer == nullptr || width <= 0 || height <= 0) {
		return;
	}

	if (renderTargetTexture != nullptr) {
		SDL_DestroyTexture(renderTargetTexture);
		renderTargetTexture = nullptr;
	}

	renderTargetTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
	if (renderTargetTexture == nullptr) {
		std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
		renderTargetWidth = 0;
		renderTargetHeight = 0;
		return;
	}

	renderTargetWidth = width;
	renderTargetHeight = height;
}

void SDL2GraphicsBackend::LoadTexture(int id) {
	//Check if texture is already loaded
	if (textures.find(id) != textures.end()) {
		return;
	}

	auto imageInfo = ImageBank::Instance().GetImage(id);
	if (!imageInfo)
	{
		backend->GetPlatform()->Log("ImageBank::GetImage Error: Image with id " + std::to_string(id) + " not found");
		return;
	}

	char imageFileName[32];
	std::snprintf(imageFileName, sizeof(imageFileName), "images/%d.rgba", id);
	
	std::vector<uint8_t> data = backend->platform->GetPakFile().GetData(imageFileName);
	if (data.empty())
	{
		backend->GetPlatform()->Log("PakFile::GetData Error: Image " + std::string(imageFileName) + " not found");
		return;
	}

	const int width = imageInfo->Width;
	const int height = imageInfo->Height;
	const int decompressedSize = width * height * 4;
	std::vector<uint8_t> pixels(static_cast<size_t>(decompressedSize));

	const int decoded = LZ4_decompress_safe(reinterpret_cast<const char*>(data.data()),reinterpret_cast<char*>(pixels.data()), static_cast<int>(data.size()), decompressedSize);
	if (decoded != decompressedSize)
	{
		backend->GetPlatform()->Log("LZ4_decompress_safe Error: Failed to decompress image " + std::to_string(id));
		return;
	}

	SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
		pixels.data(), width, height, 32, width * 4, SDL_PIXELFORMAT_RGBA32);
	if (surface == nullptr)
	{
		backend->GetPlatform()->Log("SDL_CreateRGBSurfaceWithFormatFrom Error: " + std::string(SDL_GetError()));
		return;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
	if (texture == nullptr)
	{
		backend->GetPlatform()->Log("SDL_CreateTextureFromSurface Error: " + std::string(SDL_GetError()));
		return;
	}

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	textures[id] = texture;
}

void SDL2GraphicsBackend::UnloadTexture(int id) {
	auto it = textures.find(id);
	if (it == textures.end()) {
		return;
	}

	if (it->second != nullptr) {
		SDL_DestroyTexture(it->second);
	}
	textures.erase(it);
}

void SDL2GraphicsBackend::DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, float scaleY, int color, int effect, unsigned char effectParameter, EffectInstance *effectInstance)
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

void SDL2GraphicsBackend::DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance)
{
	//TODO: Borders
	//TODO: Ellipse masks
	if (effect == 1) // Semi-Transparent doesn't have rgb coeff
		rgbCoefficient = 0xFFFFFF;

	Uint8 coeffR = (rgbCoefficient >> 16) & 0xFF;
	Uint8 coeffG = (rgbCoefficient >> 8) & 0xFF;
	Uint8 coeffB = rgbCoefficient & 0xFF;
	Uint8 alpha = 255 - effectParameter;
	SDL_BlendMode blendMode = effect == 9 ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND;

	SDL_BlendMode origDrawBlendMode;
	SDL_GetRenderDrawBlendMode(renderer, &origDrawBlendMode);
	SDL_SetRenderDrawBlendMode(renderer, blendMode);

	auto setDrawColor = [&](Uint8 r, Uint8 g, Uint8 b) {
		SDL_SetRenderDrawColor(renderer, r * coeffR / 255, g * coeffG / 255, b * coeffB / 255, alpha);
	};

	if (shape->ShapeType == 1) { // Line
		setDrawColor((shape->BorderColor >> 16) & 0xFF, (shape->BorderColor >> 8) & 0xFF, shape->BorderColor & 0xFF);

		int x1 = shape->FlipX ? x + width : x;
		int y1 = shape->FlipY ? y + height : y;
		int x2 = shape->FlipX ? x : x + width;
		int y2 = shape->FlipY ? y : y + height;

		//TODO: BorderSize
		SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
	}
	else {
		if (shape->FillType == 1) { // Solid Color
			setDrawColor((shape->Color1 >> 16) & 0xFF, (shape->Color1 >> 8) & 0xFF, shape->Color1 & 0xFF);
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
					
					setDrawColor(r, g, b);
					SDL_RenderDrawLine(renderer, x, y + i, x + width - 1, y + i);
				}
			} else {
				// Horizontal gradient (left to right)
				for (int i = 0; i < width; i++) {
					float ratio = static_cast<float>(i) / static_cast<float>(width);
					
					Uint8 r = static_cast<Uint8>(r1 + (r2 - r1) * ratio);
					Uint8 g = static_cast<Uint8>(g1 + (g2 - g1) * ratio);
					Uint8 b = static_cast<Uint8>(b1 + (b2 - b1) * ratio);
					
					setDrawColor(r, g, b);
					SDL_RenderDrawLine(renderer, x + i, y, x + i, y + height - 1);
				}
			}
		}
		else if (shape->FillType == 3) { // Motif
			SDL_Texture* texture = textures[shape->Image];
			if (texture != nullptr) {
				Uint8 origR, origG, origB, origA;
				SDL_BlendMode origBlendMode;
				SDL_GetTextureColorMod(texture, &origR, &origG, &origB);
				SDL_GetTextureAlphaMod(texture, &origA);
				SDL_GetTextureBlendMode(texture, &origBlendMode);
				SDL_SetTextureColorMod(texture, coeffR, coeffG, coeffB);
				SDL_SetTextureAlphaMod(texture, alpha);
				SDL_SetTextureBlendMode(texture, blendMode);

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

				SDL_SetTextureColorMod(texture, origR, origG, origB);
				SDL_SetTextureAlphaMod(texture, origA);
				SDL_SetTextureBlendMode(texture, origBlendMode);
			}
		}
	}

	SDL_SetRenderDrawBlendMode(renderer, origDrawBlendMode);
}

void SDL2GraphicsBackend::DrawRectangle(int x, int y, int width, int height, int color)
{
	SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
	SDL_Rect rect = { x, y, width, height };
	SDL_RenderFillRect(renderer, &rect);
}

void SDL2GraphicsBackend::DrawRectangleLines(int x, int y, int width, int height, int color)
{
	SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
	SDL_Rect rect = { x, y, width, height };
	SDL_RenderDrawRect(renderer, &rect);
}

void SDL2GraphicsBackend::DrawLine(int x1, int y1, int x2, int y2, int color)
{
	SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
	SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
}

void SDL2GraphicsBackend::DrawPixel(int x, int y, int color)
{
	SDL_SetRenderDrawColor(renderer, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, (color >> 24) & 0xFF);
	SDL_RenderDrawPoint(renderer, x, y);
}

void SDL2GraphicsBackend::LoadFont(int id)
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
		std::shared_ptr<std::vector<uint8_t>> buffer = std::make_shared<std::vector<uint8_t>>(backend->platform->GetPakFile().GetData("fonts/" + fontInfo->FontFileName));
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

void SDL2GraphicsBackend::UnloadFont(int id)
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

void SDL2GraphicsBackend::DrawText(FontInfo *fontInfo, int x, int y, int width, int height, unsigned char horizontalAlignment, unsigned char verticalAlignment, int color, const std::string &text, int objectHandle, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance *effectInstance)
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
	if (surface == nullptr) {
		return;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (texture != nullptr) {
		SDL_Rect rect = { x, y, surface->w, surface->h };
		SDL_RenderCopy(renderer, texture, nullptr, &rect);
		SDL_DestroyTexture(texture);
	}
	SDL_FreeSurface(surface);
}

SDL_FRect SDL2GraphicsBackend::CalculateRenderTargetRect()
{
	int currentWindowWidth, currentWindowHeight;
	SDL_GetWindowSize(window, &currentWindowWidth, &currentWindowHeight);

	// get app size
	int renderTargetWidth = std::min(Application::Instance().GetAppData()->GetWindowWidth(), Application::Instance().GetCurrentFrame()->Width);
	int renderTargetHeight = std::min(Application::Instance().GetAppData()->GetWindowHeight(), Application::Instance().GetCurrentFrame()->Height);

	SDL_FRect rect = {0.0f, 0.0f, static_cast<float>(renderTargetWidth), static_cast<float>(renderTargetHeight)};

	if (Application::Instance().GetAppData()->GetResizeDisplay())
	{
		rect.w = static_cast<float>(currentWindowWidth);
		rect.h = static_cast<float>(currentWindowHeight);

		if (Application::Instance().GetAppData()->GetFitInside())
		{
			// keeps the aspect ratio of the application and fits inside the window while staying in the center
			float aspectRatio = static_cast<float>(renderTargetWidth) / static_cast<float>(renderTargetHeight);
			if (rect.w / rect.h > aspectRatio)
			{
				rect.w = rect.h * aspectRatio;
			}
			else
			{
				rect.h = rect.w / aspectRatio;
			}
			rect.x = static_cast<float>((currentWindowWidth - static_cast<int>(rect.w)) / 2);
			rect.y = static_cast<float>((currentWindowHeight - static_cast<int>(rect.h)) / 2);
		}
	}
	else if (!Application::Instance().GetAppData()->GetDontCenterFrame()) {
		rect.x = static_cast<float>((currentWindowWidth - static_cast<int>(rect.w)) / 2);
		rect.y = static_cast<float>((currentWindowHeight - static_cast<int>(rect.h)) / 2);
	}
	
	return rect;
}

SDL_Colour SDL2GraphicsBackend::RGBToSDLColor(int color)
{
	return SDL_Colour{
		static_cast<Uint8>((color >> 16) & 0xFF),
		static_cast<Uint8>((color >> 8) & 0xFF),
		static_cast<Uint8>(color & 0xFF),
		255
	};
}

SDL_Colour SDL2GraphicsBackend::RGBAToSDLColor(int color)
{
	return SDL_Colour{
		static_cast<Uint8>((color >> 16) & 0xFF),
		static_cast<Uint8>((color >> 8) & 0xFF),
		static_cast<Uint8>(color & 0xFF),
		static_cast<Uint8>((color >> 24) & 0xFF)
	};
}
#endif