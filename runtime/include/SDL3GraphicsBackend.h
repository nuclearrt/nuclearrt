#pragma once

#ifdef NUCLEAR_BACKEND_SDL3

#include "GraphicsBackend.h"

#include <vector>
#include <unordered_map>
#include <set>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#if defined(PLATFORM_MACOS)
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

struct GLTexture {
	GLuint textureId = 0;
	int width = 0;
	int height = 0;
};

struct EffectShader {
	GLuint program = 0;
	GLint mvpLoc = -1;
	GLint texLoc = -1;
	GLint colorLoc = -1;
};

class SDL3Backend;

class SDL3GraphicsBackend : public GraphicsBackend {
public:
	void Initialize() override;
	void Deinitialize() override;

	void SetBackend(SDL3Backend* b) { backend = b; }

	void BeginDrawing() override;
	void EndDrawing() override;
	void Clear(int color) override;

	void BeginLayerDrawing() override;
	void EndLayerDrawing(int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance) override;

	EffectShader* LoadShader(const std::string& hash);
	EffectShader* LoadShader(const std::string& name, const std::string& data);

	void LoadTexture(int id) override;
	void UnloadTexture(int id) override;
	void DrawTexture(int id, int x, int y, int offsetX, int offsetY, int angle, float scaleX, float scaleY, int color, int effect, unsigned char effectParameter, EffectInstance* effectInstance = nullptr) override;
	void DrawQuickBackdrop(int x, int y, int width, int height, Shape* shape) override;
	void DrawBitmap(Bitmap& bitmap, int x, int y) override;
	void DrawEffectRect(int x, int y, int width, int height, int rgbCoefficient, int effect, unsigned char effectParameter, EffectInstance* effectInstance) override;

	void LoadFont(int id) override;
	void UnloadFont(int id) override;
	void DrawText(FontInfo* fontInfo, int x, int y, int color, const std::string& text, int objectHandle = -1, int rgbCoefficient = 0xFFFFFF, int effect = 0, unsigned char effectParameter = 0, EffectInstance* effectInstance = nullptr) override;

	SDL_FRect CalculateRenderTargetRect();
	SDL_Window* GetSDLWindow() const { return window; }
private:
	SDL3Backend* backend = nullptr;

	SDL_Window* window = nullptr;
	SDL_GLContext glContext = nullptr;
	bool renderedFirstFrame = false;
	
	GLuint quadVAO = 0;
	GLuint quadVBO = 0;
	GLuint renderTarget = 0;
	GLuint renderTargetTexture = 0;
	GLuint layerRenderTarget = 0;
	GLuint layerRenderTargetTexture = 0;
	int layerRenderTargetWidth = 0;
	int layerRenderTargetHeight = 0;
	int renderTargetWidth = 0;
	int renderTargetHeight = 0;
	bool drawingLayer = false;
	
	static const int STANDARD_EFFECT_COUNT = 13;
	EffectShader effectShaders[STANDARD_EFFECT_COUNT];
	int currentEffect = -1;
	
	GLuint colorShaderProgram = 0;
	GLint colorShaderMVPLoc = -1;
	GLint colorShaderColorLoc = -1;
	GLint colorShaderCircleClipLoc = -1;

	GLuint textureQuickbackdropShaderProgram = 0;
	GLint textureQuickbackdropShaderMVPLoc = -1;
	GLint textureQuickbackdropShaderColorLoc = -1;
	GLint textureQuickbackdropShaderTextureLoc = -1;
	GLint textureQuickbackdropShaderCircleClipLoc = -1;
	GLint textureQuickbackdropShaderTileScaleLoc = -1;

	GLuint gradientShaderProgram = 0;
	GLint gradientShaderMVPLoc = -1;
	GLint gradientShaderColor1Loc = -1;
	GLint gradientShaderColor2Loc = -1;
	GLint gradientShaderVerticalLoc = -1;
	GLint gradientShaderCircleClipLoc = -1;

	std::unordered_map<std::string, EffectShader> thirdPartyShaders;
	
	void CreateStandardShaders();
	void UseEffectShader(int effect);
	void ApplyEffectParameters(EffectInstance* effectInstance, int textureWidth, int textureHeight, int rgbCoefficient, int effect, unsigned char effectParameter, GLuint textureId);
	std::string LoadShaderSource(const std::string& filename);
	GLuint CompileShader(GLenum type, const char* source);
	GLuint CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc);
	void CreateRenderTarget(int width, int height);
	void CreateLayerRenderTarget(int width, int height);
	void RenderQuad(float x, float y, float w, float h, float angle = 0.0f, float pivotX = 0.0f, float pivotY = 0.0f, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);
	void SetOrthoProjection(GLuint program, GLint mvpLoc, float width, float height);

	SDL_Color RGBToSDLColor(int color);
	SDL_Color RGBAToSDLColor(int color);

	std::unordered_map<int, GLTexture> textures;
	std::unordered_map<int, TTF_Font*> fonts;
	std::unordered_map<std::string, std::shared_ptr<std::vector<uint8_t>>> fontBuffers;

	struct TextCacheKey {
		unsigned int fontHandle;
		std::string text;
		int color;
		int objectHandle;
		
		bool operator==(const TextCacheKey& other) const {
			return fontHandle == other.fontHandle && text == other.text && color == other.color && objectHandle == other.objectHandle;
		}
	};
	
	struct TextCacheKeyHash {
		std::size_t operator()(const TextCacheKey& key) const {
			std::size_t h1 = std::hash<unsigned int>{}(key.fontHandle);
			std::size_t h2 = std::hash<std::string>{}(key.text);
			std::size_t h3 = std::hash<int>{}(key.color);
			std::size_t h4 = std::hash<int>{}(key.objectHandle);
			return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
		}
	};
	
	struct CachedText {
		GLTexture texture;
		int width;
		int height;
	};
	
	std::unordered_map<TextCacheKey, CachedText, TextCacheKeyHash> textCache;

	void RemoveOldTextCache();
	void ClearTextCacheForFont(int fontHandle);
}; 
#endif