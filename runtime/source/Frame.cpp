#include "Frame.h"

#include <map>
#include <algorithm>
#include <math.h>
#include <unordered_map>

#include "Application.h"
#include "Counter.h"
#include "Extension.h"
#include "FontBank.h"
#include "ImageBank.h"
#include "ObjectGlobalDataCounter.h"
#include "GraphicsBackend.h"
#include "InputBackend.h"
#include "PlatformBackend.h"

constexpr float PI = 3.14159265358979323846f;

void Frame::Initialize()
{
}

void Frame::PostInitialize()
{
}

void Frame::Update()
{
	ClearBoundsCache();
	float deltaTime = Application::Instance().GetBackend()->platform->GetTimeDelta();
	GameTimer.Update(deltaTime);

	for (auto& [handle, instance] : ObjectInstances)
	{
		//Animation update
		if (instance->Type == 2) // Common object with possible animation
		{
			((Active*)instance)->movements.Update(deltaTime);
			((Active*)instance)->animations.Update(deltaTime);
		}
		else if (instance->Type == 5 || instance->Type == 6 || instance->Type == 7) // Counter
		{
			((CounterBase*)instance)->movements.Update(deltaTime);
		}
		else if (instance->Type >= 32) // Extension
		{
			((Extension*)instance)->Update(deltaTime);
		}
	}
}

void Frame::Draw()
{
	Application::Instance().GetBackend()->graphics->Clear(BackgroundColor);

	for (unsigned int i = 0; i < Layers.size(); i++)
	{
		Layer& layer = Layers[i];
		bool hasLayerEffect = (layer.usePreviousLayerEffect && i > 0) || layer.Effect != 0 || layer.GetEffectParameter() != 0 || layer.RGBCoefficient != 0xFFFFFFFF;
		if (!hasLayerEffect)
		{
			DrawLayer(layer);
			continue;
		}

		Application::Instance().GetBackend()->graphics->BeginLayerDrawing();
		DrawLayer(layer);

		EffectInstance* effectInstance = nullptr;
		int effect = 0;
		int effectParameter = 0;
		int rgbCoefficient = 0xFFFFFF;

		if (hasLayerEffect)
		{
			effectInstance = layer.effectInstance;
			effect = layer.Effect;
			effectParameter = layer.GetEffectParameter();
			rgbCoefficient = layer.RGBCoefficient;

			if (layer.usePreviousLayerEffect && i > 0)
			{
				int previousLayerIndex = i;
				while (previousLayerIndex > 0 && Layers[previousLayerIndex].usePreviousLayerEffect)
				{
					previousLayerIndex--;
				}
				effect = Layers[previousLayerIndex].Effect;
				effectInstance = Layers[previousLayerIndex].effectInstance;
				effectParameter = Layers[previousLayerIndex].GetEffectParameter();
				rgbCoefficient = Layers[previousLayerIndex].RGBCoefficient;
			}
		}

		Application::Instance().GetBackend()->graphics->EndLayerDrawing(rgbCoefficient, effect, effectParameter, effectInstance);
	}
}

void Frame::SetScroll(int x, int y, int layer)
{
	int windowWidth = Application::Instance().GetAppData()->GetWindowWidth();
	int windowHeight = Application::Instance().GetAppData()->GetWindowHeight();

	x -= windowWidth / 2;
	y -= windowHeight / 2;

	if (layer != -1)
	{
		x = x / Layers[layer].XCoefficient;
		y = y / Layers[layer].YCoefficient;
	}

	x = std::max(0, x);
	y = std::max(0, y);
	x = std::min(Width - windowWidth, x);
	y = std::min(Height - windowHeight, y);

	scrollX = x;
	scrollY = y;
}

void Frame::SetScrollX(int x)
{
	int windowWidth = Application::Instance().GetAppData()->GetWindowWidth();
	x -= windowWidth / 2;
	x = std::max(0, x);
	x = std::min(Width - windowWidth, x);
	scrollX = x;
}

void Frame::SetScrollY(int y)
{
	int windowHeight = Application::Instance().GetAppData()->GetWindowHeight();
	y -= windowHeight / 2;
	y = std::max(0, y);
	y = std::min(Height - windowHeight, y);
	scrollY = y;
}

int Frame::GetXLeftEdge()
{
	return scrollX;
}

int Frame::GetXRightEdge()
{
	return scrollX + Application::Instance().GetAppData()->GetWindowWidth();
}

int Frame::GetYTopEdge()
{
	return scrollY;
}

int Frame::GetYBottomEdge()
{
	return scrollY + Application::Instance().GetAppData()->GetWindowHeight();
}

void Frame::DrawLayer(Layer& layer)
{
	for (auto& instance : layer.instances)
	{
		if (instance->Type == 1)
		{
			auto& imageBank = ImageBank::Instance();
			unsigned int imageId = ((Backdrop*)instance)->Image;

			Application::Instance().GetBackend()->graphics->DrawTexture(
				imageId, instance->X - (scrollX * layer.XCoefficient), instance->Y - (scrollY * layer.YCoefficient),
				0, 0, 0, 1.0f, 1.0f, instance->RGBCoefficient, instance->Effect, instance->GetEffectParameter(), instance->effectInstance);
		}
		else if (instance->Type == 2)
		{
			if (!((Active*)instance)->Visible) continue;
			if (((Active*)instance)->xScale <= 0.0f || ((Active*)instance)->yScale <= 0.0f) continue;

			auto& imageBank = ImageBank::Instance();
			auto& animations = ((Active*)instance)->animations;
			unsigned int imageId = animations.GetCurrentImageHandle();

			int scrollXOffset = 0;
			int scrollYOffset = 0;
			if (((Active*)instance)->FollowFrame)
			{
				scrollXOffset = scrollX * layer.XCoefficient;
				scrollYOffset = scrollY * layer.YCoefficient;
			}

			auto imageInfo = imageBank.GetImage(imageId);
			if (imageInfo)
			{
				int angle = ((Active*)instance)->GetAngle();
				if (((Active*)instance)->AutomaticRotation)
				{
					auto movement = ((Active*)instance)->movements.GetCurrentMovement();
					if (movement != nullptr && !animations.IsDirectionForced())
					{
						angle += movement->GetMovementDirection() * 180 / 16;
					}
					else
					{
						angle += animations.GetAutomaticRotationDirection() * 180 / 16;
					}
				}

				Application::Instance().GetBackend()->graphics->DrawTexture(
					imageId, instance->X - scrollXOffset, instance->Y - scrollYOffset,
					imageInfo->HotspotX, imageInfo->HotspotY, 
					angle, ((Active*)instance)->xScale, ((Active*)instance)->yScale, instance->RGBCoefficient, instance->Effect, instance->GetEffectParameter(), instance->effectInstance);
			}
		}
		else if (instance->Type == 3) // Text
		{
			if (!((StringObject*)instance)->Visible) continue;

			int scrollXOffset = 0;
			int scrollYOffset = 0;

			if (((StringObject*)instance)->FollowFrame)
			{
				scrollXOffset = scrollX * layer.XCoefficient;
				scrollYOffset = scrollY * layer.YCoefficient;
			}

			std::string text = ((StringObject*)instance)->GetText();
			Application::Instance().GetBackend()->graphics->DrawText(
				FontBank::Instance().GetFont(((StringObject*)instance)->GetFont()),
				instance->X - scrollXOffset,
				instance->Y - scrollYOffset,
				((StringObject*)instance)->GetColor(),
				text,
				instance->Handle,
				instance->RGBCoefficient,
				instance->Effect,
				instance->GetEffectParameter(),
				instance->effectInstance
			);
		}
		else if (instance->Type == 5 || instance->Type == 6 || instance->Type == 7) // Score, Lives, Counter
		{
			CounterBase* counter = (CounterBase*)instance;
			if (!counter->Visible) continue;
			
			int scrollXOffset = 0;
			int scrollYOffset = 0;
			if (counter->FollowFrame)
			{
				scrollXOffset = scrollX * layer.XCoefficient;
				scrollYOffset = scrollY * layer.YCoefficient;
			}

			//TODO: Add support for other display types
			if (counter->DisplayType == 1) // Numbers
			{
				auto appdata = Application::Instance().GetAppData();

				int value = 0;
				if (instance->Type == 5) // Score
				{
					value = Application::Instance().GetAppData()->GetPlayerScores()[counter->Player];
				}
				else if (instance->Type == 6) // Lives
				{
					value = Application::Instance().GetAppData()->GetPlayerLives()[counter->Player];
				}
				else
				{
					value = ((Counter*)instance)->GetValue();
				}
				
				DrawCounterNumbers(counter, value, instance->X - scrollXOffset, instance->Y - scrollYOffset);
			}
		}
		else if (instance->Type == 0) // Quick backdrop
		{
			int scrollXOffset = scrollX * layer.XCoefficient;
			int scrollYOffset = scrollY * layer.YCoefficient;
			Application::Instance().GetBackend()->graphics->DrawQuickBackdrop(instance->X - scrollXOffset, instance->Y - scrollYOffset, ((QuickBackdrop*)instance)->Width, ((QuickBackdrop*)instance)->Height, &((QuickBackdrop*)instance)->shape);
		}
		else if (instance->Type >= 32) // Extension
		{
			if (!((Extension*)instance)->Visible) continue;
			((Extension*)instance)->Draw();
		}
	}
}

void Frame::DrawCounterNumbers(CounterBase *counter, int value, int x, int y)
{
	std::string valueString = std::to_string(value);
	int numDigits = valueString.size();

	if (counter->IntDigitCount > 0) numDigits = counter->IntDigitCount;

	bool negative = false;
	if (value < 0)
	{
		negative = true;
		valueString = valueString.substr(1);
	}

	//Fixed Size
	if (counter->IntDigitCount > 0)
	{
		if (counter->IntDigitCount > valueString.size()) //Add leading zeros
		{
			while (valueString.size() < counter->IntDigitCount)
			{
				valueString = "0" + valueString;
			}
		}
		else //Remove extra digits
		{
			valueString = valueString.substr(valueString.size() - counter->IntDigitCount);
		}
	}		

	if (negative)
	{
		valueString = "-" + valueString;
		numDigits++;
	}

	//Counter char map
	std::string charMap = "0123456789-+.e";

	//get the total width of the string to be displayed
	int totalWidth = 0;
	for (int i = 0; i < numDigits; i++)
	{
		if (i >= valueString.size())
		{
			break;
		}

		int imageIndex = charMap.find(valueString[i]);
		totalWidth += ImageBank::Instance().GetImage(counter->Frames[imageIndex])->Width;
	}

	//Get height of the tallest character displayed
	int MaxHeight = 0;
	for (int i = 0; i < numDigits; i++)
	{
		if (i >= valueString.size())
		{
			break;
		}

		int imageIndex = charMap.find(valueString[i]);
		MaxHeight = std::max(MaxHeight, ImageBank::Instance().GetImage(counter->Frames[imageIndex])->Height);
	}

	int currentX = x - totalWidth;

	//Draw
	for (int i = 0; i < numDigits; i++)
	{
		if (i >= valueString.size())
		{
			break;
		}

		int imageIndex = charMap.find(valueString[i]);
		auto imageInfo = ImageBank::Instance().GetImage(counter->Frames[imageIndex]);
		if (imageInfo)
		{
			Application::Instance().GetBackend()->graphics->DrawTexture(
				counter->Frames[imageIndex], currentX, y - MaxHeight,
				0, 0, 
				0, 1.0f, 1.0f, 0xFFFFFFFF, 0, 0);
			currentX += imageInfo->Width;
		}
	}
}

std::vector<unsigned int> Frame::GetImagesUsed()
{
	std::vector<unsigned int> imagesUsed;
	
	for (auto& [handle, instance] : ObjectInstances)
	{
		std::vector<unsigned int> instanceImages = instance->GetImagesUsed();
		for (unsigned int image : instanceImages)
		{
			if (std::find(imagesUsed.begin(), imagesUsed.end(), image) == imagesUsed.end())
			{
				imagesUsed.push_back(image);
			}
		}
	}

	return imagesUsed;
}

std::vector<unsigned int> Frame::GetFontsUsed()
{
	std::vector<unsigned int> fontsUsed;

	for (auto& [handle, instance] : ObjectInstances)
	{
		std::vector<unsigned int> instanceFonts = instance->GetFontsUsed();
		for (unsigned int font : instanceFonts)
		{
			if (std::find(fontsUsed.begin(), fontsUsed.end(), font) == fontsUsed.end())
			{
				fontsUsed.push_back(font);
			}
		}
	}

	//erase any duplicates
	std::sort(fontsUsed.begin(), fontsUsed.end());
	fontsUsed.erase(std::unique(fontsUsed.begin(), fontsUsed.end()), fontsUsed.end());

	return fontsUsed;
}

ObjectInstance* Frame::CreateInstance(ObjectInstance* createdInstance, short x, short y, unsigned int layer, short instanceValue, unsigned int objectInfoHandle, short angle, bool postInitialize, ObjectInstance* parentInstance)
{
	createdInstance->Handle = ++MaxObjectInstanceHandle;
	createdInstance->X = x;
	createdInstance->Y = y;
	createdInstance->Layer = layer;
	createdInstance->InstanceValue = instanceValue;
	createdInstance->ObjectInfoHandle = objectInfoHandle;
	createdInstance->SetAngle(angle);

	//TODO: move this to a separate function
	// Load any textures needed for this instance
	if (postInitialize) // objs created on init have textures loaded in application
	{
		std::vector<unsigned int> texturesToLoad = createdInstance->GetImagesUsed();
		auto backend = Application::Instance().GetBackend();
		for (unsigned int textureId : texturesToLoad) {
			backend->graphics->LoadTexture(textureId);
		}
	}
	
	ObjectInstances[createdInstance->Handle] = createdInstance;
	if (parentInstance) {
		createdInstance->X += parentInstance->X;
		createdInstance->Y += parentInstance->Y;
		createdInstance->Layer = parentInstance->Layer;
	}

	//init movement and extensions
	if (createdInstance->Type == 2) // Common object
	{
		for (auto& [handle, movement] : ((Active*)createdInstance)->movements.items)
		{
			movement->Instance = createdInstance;
			movement->Initialize();

			if (handle == 0) movement->OnEnabled();
		}

		((Active*)createdInstance)->animations.AutomaticRotation = ((Active*)createdInstance)->AutomaticRotation;
	}
	else if (createdInstance->Type == 5 || createdInstance->Type == 6 || createdInstance->Type == 7) // Counter
	{
		for (auto& [handle, movement] : ((CounterBase*)createdInstance)->movements.items)
		{
			movement->Instance = createdInstance;
			movement->Initialize();

			if (handle == 0) movement->OnEnabled();
		}
	}
	else if (createdInstance->Type >= 32) // Extension
	{
		((Extension*)createdInstance)->Initialize();
	}

	Layers[createdInstance->Layer].instances.push_back(createdInstance);

	return createdInstance;
}

std::vector<ObjectGlobalData*> Frame::GetGlobalObjectData()
{
	std::map<unsigned int, std::vector<ObjectInstance*>> instancesByHandle;
	for (auto& [handle, instance] : ObjectInstances)
	{
		if (instance->global)
		{
			instancesByHandle[instance->ObjectInfoHandle].push_back(instance);
		}
	}
	
	for (auto& [objInfoHandle, instances] : instancesByHandle)
	{
		std::sort(instances.begin(), instances.end(), 
			[](ObjectInstance* a, ObjectInstance* b) { return a->Handle < b->Handle; });
	}
	
	std::vector<ObjectGlobalData*> result;
	for (auto& [objInfoHandle, instances] : instancesByHandle)
	{
		for (auto* instance : instances)
		{
			ObjectGlobalData* data = instance->CreateGlobalData();
			if (data != nullptr)
			{
				result.push_back(data);
			}
		}
	}
	
	return result;
}

void Frame::MoveObjectToLayer(ObjectInstance* instance, unsigned int layer)
{
	if (instance->Layer == layer) return;
	if (layer >= Layers.size()) return;

	Layers[instance->Layer].instances.erase(std::find(Layers[instance->Layer].instances.begin(), Layers[instance->Layer].instances.end(), instance));
	Layers[layer].instances.push_back(instance);
	instance->Layer = layer;
}

void Frame::MoveObjectToFront(ObjectInstance* instance)
{	
	Layers[instance->Layer].instances.erase(std::find(Layers[instance->Layer].instances.begin(), Layers[instance->Layer].instances.end(), instance));
	Layers[instance->Layer].instances.push_back(instance);
}

void Frame::MoveObjectToBack(ObjectInstance* instance)
{
	Layers[instance->Layer].instances.erase(std::find(Layers[instance->Layer].instances.begin(), Layers[instance->Layer].instances.end(), instance));
	Layers[instance->Layer].instances.insert(Layers[instance->Layer].instances.begin(), instance);
}

void Frame::MoveObjectInFrontOf(ObjectInstance* instance, unsigned int oiHandle)
{	
	int maxIndex = -1;
	for (int i = 0; i < Layers[instance->Layer].instances.size(); i++)
	{
		if (Layers[instance->Layer].instances[i]->ObjectInfoHandle == oiHandle)
		{
			maxIndex = i;
		}
	}

	if (maxIndex == -1) return;
	Layers[instance->Layer].instances.erase(std::find(Layers[instance->Layer].instances.begin(), Layers[instance->Layer].instances.end(), instance));
	Layers[instance->Layer].instances.insert(Layers[instance->Layer].instances.begin() + maxIndex + 1, instance);
}

void Frame::MoveObjectBehindOf(ObjectInstance* instance, unsigned int oiHandle)
{
	
	int minIndex = -1;
	for (int i = Layers[instance->Layer].instances.size() - 1; i >= 0; i--)
	{
		if (Layers[instance->Layer].instances[i]->ObjectInfoHandle == oiHandle)
		{
			minIndex = i;
		}
	}
	
	if (minIndex == -1) return;
	Layers[instance->Layer].instances.erase(std::find(Layers[instance->Layer].instances.begin(), Layers[instance->Layer].instances.end(), instance));
	Layers[instance->Layer].instances.insert(Layers[instance->Layer].instances.begin() + minIndex, instance);
}

int Frame::GetMouseX()
{
	return Application::Instance().GetBackend()->input->GetMouseX() + scrollX;
}

int Frame::GetMouseY()
{
	return Application::Instance().GetBackend()->input->GetMouseY() + scrollY;
}

void Frame::ApplyGlobalObjectData(std::vector<ObjectGlobalData*> savedData)
{
	std::map<unsigned int, std::vector<ObjectGlobalData*>> dataByHandle;
	for (auto* data : savedData)
	{
		dataByHandle[data->objectInfoHandle].push_back(data);
	}
	
	std::map<unsigned int, std::vector<ObjectInstance*>> instancesByHandle;
	for (auto& [handle, instance] : ObjectInstances)
	{
		if (instance->global)
		{
			ObjectGlobalData* testData = instance->CreateGlobalData();
			if (testData != nullptr)
			{
				delete testData;
				instancesByHandle[instance->ObjectInfoHandle].push_back(instance);
			}
		}
	}
	
	for (auto& [objInfoHandle, instances] : instancesByHandle)
	{
		std::sort(instances.begin(), instances.end(), 
			[](ObjectInstance* a, ObjectInstance* b) { return a->Handle < b->Handle; });
	}
	
	for (auto& [objInfoHandle, instances] : instancesByHandle)
	{
		if (dataByHandle.find(objInfoHandle) == dataByHandle.end())
		{
			continue;
		}
		
		auto& dataList = dataByHandle[objInfoHandle];
		int savedCount = dataList.size();
		int currentCount = instances.size();
		
		if (savedCount <= currentCount)
		{
			int startIndex = currentCount - savedCount;
			for (int i = 0; i < savedCount; i++)
			{
				instances[startIndex + i]->ApplyGlobalData(dataList[i]);
			}
		}
		else
		{
			int dataStartIndex = savedCount - currentCount;
			for (int i = 0; i < currentCount; i++)
			{
				instances[i]->ApplyGlobalData(dataList[dataStartIndex + i]);
			}
		}
	}
}

//Check if the object is colliding with any backdrop
bool Frame::IsCollidingWithBackground(ObjectInstance *instance)
{
	if (instance->Type != 2) return false; // Only Active objects
	
	// Check collision with all backdrop objects
	for (auto& [handle, backdropInstance] : ObjectInstances)
	{
		if (backdropInstance->Layer != instance->Layer) continue;

		// Check only against (Quick) backdrop objects
		if (backdropInstance->Type == 1)
		{
			Backdrop* backdrop = (Backdrop*)backdropInstance;
			
			// Check if this backdrop is an obstacle
			if (backdrop->ObstacleType > 0) {
				if (IsColliding(instance, backdrop)) return true;
			}
		}
		else if (backdropInstance->Type == 0)
		{
			QuickBackdrop* quickBackdrop = (QuickBackdrop*)backdropInstance;
			
			// Check if this backdrop is an obstacle
			if (quickBackdrop->ObstacleType > 0) {
				if (IsColliding(instance, quickBackdrop)) return true;
			}
		}
	}
	
	return false; // No collision with any backdrop
}

struct CollisionInstanceBounds {
	int minX, minY, maxX, maxY;
	int width, height;
	int centerX, centerY;
	int angle;
	unsigned int imageId;
	int hotspotX, hotspotY;
	int maskWidth, maskHeight;
	float scaleX, scaleY;
};

static std::unordered_map<Frame*, std::unordered_map<ObjectInstance*, CollisionInstanceBounds>> s_instanceBoundsCache;

void Frame::ClearBoundsCache() {
	auto it = s_instanceBoundsCache.find(this);
	if (it != s_instanceBoundsCache.end())
		it->second.clear();
}

static CollisionInstanceBounds GetInstanceBounds(Frame* frame, ObjectInstance* instance, int scrollX, int scrollY) {
	auto& cache = s_instanceBoundsCache[frame];
	auto cit = cache.find(instance);
	if (cit != cache.end())
		return cit->second;

	CollisionInstanceBounds bounds = {0};
	bounds.angle = instance->GetAngle();
	bounds.scaleX = 1.0f;
	bounds.scaleY = 1.0f;
	
	unsigned int imageId = 0;
	int drawX = 0, drawY = 0;
	int hotspotX = 0, hotspotY = 0;
	
	if (instance->Type == 0) { // Quick backdrop
		imageId = ((QuickBackdrop*)instance)->shape.Image;
		drawX = instance->X - (scrollX * frame->Layers[instance->Layer].XCoefficient);
		drawY = instance->Y - (scrollY * frame->Layers[instance->Layer].YCoefficient);
		bounds.width = ((QuickBackdrop*)instance)->Width;
		bounds.height = ((QuickBackdrop*)instance)->Height;
		bounds.maskWidth = bounds.width;
		bounds.maskHeight = bounds.height;
	} else if (instance->Type == 1) { // Backdrop
		imageId = ((Backdrop*)instance)->Image;
		drawX = instance->X - (scrollX * frame->Layers[instance->Layer].XCoefficient);
		drawY = instance->Y - (scrollY * frame->Layers[instance->Layer].YCoefficient);
		auto imageInfo = ImageBank::Instance().GetImage(imageId);
		if (imageInfo) {
			bounds.width = imageInfo->Width;
			bounds.height = imageInfo->Height;
			bounds.maskWidth = imageInfo->Width;
			bounds.maskHeight = imageInfo->Height;
		}
	} else { // Active object
		Active* active = (Active*)instance;
		imageId = active->animations.GetCurrentImageHandle();
		bounds.scaleX = active->xScale;
		bounds.scaleY = active->yScale;
		int scrollXOffset = 0, scrollYOffset = 0;
		if (active->FollowFrame) {
			scrollXOffset = scrollX * frame->Layers[instance->Layer].XCoefficient;
			scrollYOffset = scrollY * frame->Layers[instance->Layer].YCoefficient;
		}
		drawX = instance->X - scrollXOffset;
		drawY = instance->Y - scrollYOffset;
		auto imageInfo = ImageBank::Instance().GetImage(imageId);
		if (imageInfo) {
			bounds.maskWidth = imageInfo->Width;
			bounds.maskHeight = imageInfo->Height;
			hotspotX = (int)std::lround(imageInfo->HotspotX * bounds.scaleX);
			hotspotY = (int)std::lround(imageInfo->HotspotY * bounds.scaleY);
			bounds.width = std::max((int)std::lround(imageInfo->Width * bounds.scaleX), 1);
			bounds.height = std::max((int)std::lround(imageInfo->Height * bounds.scaleY), 1);
		}
	}
	
	bounds.centerX = drawX;
	bounds.centerY = drawY;
	bounds.imageId = imageId;
	bounds.hotspotX = hotspotX;
	bounds.hotspotY = hotspotY;
	
	int x1 = drawX - hotspotX;
	int y1 = drawY - hotspotY;
	int x2 = x1 + bounds.width;
	int y2 = y1;
	int x3 = x1;
	int y3 = y1 + bounds.height;
	int x4 = x2;
	int y4 = y3;
	
	if (bounds.angle != 0) {
		float rotationAngle = 360.0f - bounds.angle;
		float radians = rotationAngle * (PI / 180.0f);
		float cosA = cos(radians);
		float sinA = sin(radians);
		float dx1 = x1 - bounds.centerX, dy1 = y1 - bounds.centerY;
		float dx2 = x2 - bounds.centerX, dy2 = y2 - bounds.centerY;
		float dx3 = x3 - bounds.centerX, dy3 = y3 - bounds.centerY;
		float dx4 = x4 - bounds.centerX, dy4 = y4 - bounds.centerY;
		
		x1 = bounds.centerX + (int)(dx1 * cosA - dy1 * sinA);
		y1 = bounds.centerY + (int)(dx1 * sinA + dy1 * cosA);
		x2 = bounds.centerX + (int)(dx2 * cosA - dy2 * sinA);
		y2 = bounds.centerY + (int)(dx2 * sinA + dy2 * cosA);
		x3 = bounds.centerX + (int)(dx3 * cosA - dy3 * sinA);
		y3 = bounds.centerY + (int)(dx3 * sinA + dy3 * cosA);
		x4 = bounds.centerX + (int)(dx4 * cosA - dy4 * sinA);
		y4 = bounds.centerY + (int)(dx4 * sinA + dy4 * cosA);
	}

	bounds.minX = std::min({x1, x2, x3, x4});
	bounds.minY = std::min({y1, y2, y3, y4});
	bounds.maxX = std::max({x1, x2, x3, x4});
	bounds.maxY = std::max({y1, y2, y3, y4});

	cache[instance] = bounds;
	return bounds;
}

static bool IsPointInRotatedBox(int worldX, int worldY, const CollisionInstanceBounds& bounds) {
	float dx = worldX - bounds.centerX;
	float dy = worldY - bounds.centerY;
	if (bounds.angle != 0) {
		float rotationAngle = 360.0f - bounds.angle;
		float radians = -rotationAngle * (PI / 180.0f);
		float cosA = cos(radians);
		float sinA = sin(radians);
		float newX = dx * cosA - dy * sinA;
		float newY = dx * sinA + dy * cosA;
		dx = newX;
		dy = newY;
	}
	int localX = (int)(dx + bounds.hotspotX);
	int localY = (int)(dy + bounds.hotspotY);
	return localX >= 0 && localX < bounds.width && localY >= 0 && localY < bounds.height;
}

static bool IsPixelSolid(const std::vector<uint8_t>& maskData, int width, int height, int x, int y) {
	if (x < 0 || x >= width || y < 0 || y >= height) return false;
	
	int bytesPerRow = (width + 7) / 8;
	int byteIndex = y * bytesPerRow + (x / 8);
	int bitIndex = 7 - (x % 8);
	
	if (byteIndex >= (int)maskData.size()) return false;
	
	return (maskData[byteIndex] & (1 << bitIndex)) != 0;
}

static int ConvertScaledCoordToMaskCoord(int scaledCoord, float scale, int maskSize) {
	(void)maskSize;
	return (int)std::floor(scaledCoord / scale);
}

bool Frame::IsColliding(ObjectInstance *instance1, ObjectInstance *instance2)
{
	//Only check collision for relevant object types
	if ((instance1->Type != 0 && instance1->Type != 1 && instance1->Type != 2) ||
		(instance2->Type != 0 && instance2->Type != 1 && instance2->Type != 2))
		return false;

	// Check if the objects are on the same layer
	if (instance1->Layer != instance2->Layer) return false;

	CollisionInstanceBounds bounds1 = GetInstanceBounds(this, instance1, scrollX, scrollY);
	CollisionInstanceBounds bounds2 = GetInstanceBounds(this, instance2, scrollX, scrollY);
	
	if (bounds1.maxX < bounds2.minX || bounds1.minX > bounds2.maxX ||
		bounds1.maxY < bounds2.minY || bounds1.minY > bounds2.maxY)
		return false;

	unsigned int imageId1 = 0, imageId2 = 0;
	
	if (instance1->Type == 0) {
		imageId1 = ((QuickBackdrop*)instance1)->shape.Image;
	} else if (instance1->Type == 1) {
		imageId1 = ((Backdrop*)instance1)->Image;
	} else {
		imageId1 = ((Active*)instance1)->animations.GetCurrentImageHandle();
	}
	
	if (instance2->Type == 0) {
		imageId2 = ((QuickBackdrop*)instance2)->shape.Image;
	} else if (instance2->Type == 1) {
		imageId2 = ((Backdrop*)instance2)->Image;
	} else {
		imageId2 = ((Active*)instance2)->animations.GetCurrentImageHandle();
	}
	
	Backend* backend = Application::Instance().GetBackend().get();
	const std::vector<uint8_t>* maskData1 = backend->platform->GetCollisionMaskData(imageId1);
	const std::vector<uint8_t>* maskData2 = backend->platform->GetCollisionMaskData(imageId2);

	bool useMask1 = maskData1 && !maskData1->empty() && (instance1->Type == 1 || (instance1->Type == 2 && ((Active*)instance1)->FineDetection));
	bool useMask2 = maskData2 && !maskData2->empty() && (instance2->Type == 1 || (instance2->Type == 2 && ((Active*)instance2)->FineDetection));
	
	auto imageInfo1 = ImageBank::Instance().GetImage(imageId1);
	auto imageInfo2 = ImageBank::Instance().GetImage(imageId2);
	if (!imageInfo1 || !imageInfo2) {
		int overlapMinX = std::max(bounds1.minX, bounds2.minX);
		int overlapMinY = std::max(bounds1.minY, bounds2.minY);
		int overlapMaxX = std::min(bounds1.maxX, bounds2.maxX);
		int overlapMaxY = std::min(bounds1.maxY, bounds2.maxY);
		for (int py = overlapMinY; py <= overlapMaxY; py++)
			for (int px = overlapMinX; px <= overlapMaxX; px++)
				if (IsPointInRotatedBox(px, py, bounds1) && IsPointInRotatedBox(px, py, bounds2))
					return true;
		return false;
	}
	
	int width1 = bounds1.maskWidth;
	int height1 = bounds1.maskHeight;
	int width2 = bounds2.maskWidth;
	int height2 = bounds2.maskHeight;
	if (instance1->Type == 0) {
		width1 = ((QuickBackdrop*)instance1)->Width;
		height1 = ((QuickBackdrop*)instance1)->Height;
	}
	if (instance2->Type == 0) {
		width2 = ((QuickBackdrop*)instance2)->Width;
		height2 = ((QuickBackdrop*)instance2)->Height;
	}
	
	int hotspotX1 = bounds1.hotspotX, hotspotY1 = bounds1.hotspotY;
	int hotspotX2 = bounds2.hotspotX, hotspotY2 = bounds2.hotspotY;
	
	float cos1 = 1.0f, sin1 = 0.0f;
	float cos2 = 1.0f, sin2 = 0.0f;
	if (bounds1.angle != 0) {
		float radians = -(360.0f - bounds1.angle) * (PI / 180.0f);
		cos1 = cos(radians);
		sin1 = sin(radians);
	}
	if (bounds2.angle != 0) {
		float radians = -(360.0f - bounds2.angle) * (PI / 180.0f);
		cos2 = cos(radians);
		sin2 = sin(radians);
	}
	
	int overlapMinX = std::max(bounds1.minX, bounds2.minX);
	int overlapMinY = std::max(bounds1.minY, bounds2.minY);
	int overlapMaxX = std::min(bounds1.maxX, bounds2.maxX);
	int overlapMaxY = std::min(bounds1.maxY, bounds2.maxY);
	
	for (int py = overlapMinY; py <= overlapMaxY; py++) {
		for (int px = overlapMinX; px <= overlapMaxX; px++) {
			float dx1 = px - bounds1.centerX, dy1 = py - bounds1.centerY;
			if (bounds1.angle != 0) {
				float nx = dx1 * cos1 - dy1 * sin1, ny = dx1 * sin1 + dy1 * cos1;
				dx1 = nx; dy1 = ny;
			}
			int lx1 = (int)(dx1 + hotspotX1), ly1 = (int)(dy1 + hotspotY1);
			int sampleX1 = ConvertScaledCoordToMaskCoord(lx1, bounds1.scaleX, width1);
			int sampleY1 = ConvertScaledCoordToMaskCoord(ly1, bounds1.scaleY, height1);
			bool solid1 = useMask1 ? IsPixelSolid(*maskData1, width1, height1, sampleX1, sampleY1)
				: (lx1 >= 0 && lx1 < bounds1.width && ly1 >= 0 && ly1 < bounds1.height);

			float dx2 = px - bounds2.centerX, dy2 = py - bounds2.centerY;
			if (bounds2.angle != 0) {
				float nx = dx2 * cos2 - dy2 * sin2, ny = dx2 * sin2 + dy2 * cos2;
				dx2 = nx; dy2 = ny;
			}
			int lx2 = (int)(dx2 + hotspotX2), ly2 = (int)(dy2 + hotspotY2);
			int sampleX2 = ConvertScaledCoordToMaskCoord(lx2, bounds2.scaleX, width2);
			int sampleY2 = ConvertScaledCoordToMaskCoord(ly2, bounds2.scaleY, height2);
			bool solid2 = useMask2 ? IsPixelSolid(*maskData2, width2, height2, sampleX2, sampleY2)
				: (lx2 >= 0 && lx2 < bounds2.width && ly2 >= 0 && ly2 < bounds2.height);

			if (solid1 && solid2)
				return true;
		}
	}
	return false;
}


bool Frame::IsColliding(ObjectInstance *instance, int x, int y)
{
	if (instance->Type != 0 && instance->Type != 1 && instance->Type != 2) return false;

	x -= scrollX;
	y -= scrollY;

	CollisionInstanceBounds bounds = GetInstanceBounds(this, instance, scrollX, scrollY);
	if (x < bounds.minX || x > bounds.maxX || y < bounds.minY || y > bounds.maxY)
		return false;

	unsigned int imageId = 0;
	bool fineDetection = false;
	
	if (instance->Type == 0) {
		imageId = ((QuickBackdrop*)instance)->shape.Image;
	} else if (instance->Type == 1) {
		imageId = ((Backdrop*)instance)->Image;
		fineDetection = true;
	} else {
		imageId = ((Active*)instance)->animations.GetCurrentImageHandle();
		fineDetection = ((Active*)instance)->FineDetection;
	}
	if (!fineDetection)
		return IsPointInRotatedBox(x, y, bounds);
	
	const std::vector<uint8_t>* maskData = Application::Instance().GetBackend().get()->platform->GetCollisionMaskData(imageId);
	if (!maskData || maskData->empty())
		return IsPointInRotatedBox(x, y, bounds);

	auto imageInfo = ImageBank::Instance().GetImage(imageId);
	if (!imageInfo) return IsPointInRotatedBox(x, y, bounds);

	int width = bounds.maskWidth;
	int height = bounds.maskHeight;
	if (instance->Type == 0) {
		width = ((QuickBackdrop*)instance)->Width;
		height = ((QuickBackdrop*)instance)->Height;
	}

	float dx = x - bounds.centerX;
	float dy = y - bounds.centerY;
	if (bounds.angle != 0) {
		float radians = -(360.0f - bounds.angle) * (PI / 180.0f);
		float cosA = cos(radians);
		float sinA = sin(radians);
		float newX = dx * cosA - dy * sinA;
		float newY = dx * sinA + dy * cosA;
		dx = newX;
		dy = newY;
	}
	int localX = (int)(dx + bounds.hotspotX);
	int localY = (int)(dy + bounds.hotspotY);
	int sampleX = ConvertScaledCoordToMaskCoord(localX, bounds.scaleX, width);
	int sampleY = ConvertScaledCoordToMaskCoord(localY, bounds.scaleY, height);
	return IsPixelSolid(*maskData, width, height, sampleX, sampleY);
}