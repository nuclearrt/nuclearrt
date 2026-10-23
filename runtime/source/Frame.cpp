#include "Frame.h"

#include <map>
#include <algorithm>
#include <math.h>
#include <unordered_map>
#include <charconv>
#include <cassert>

#include "Application.h"
#include "Active.h"
#include "Counter.h"
#include "Extension.h"
#include "FontBank.h"
#include "ImageBank.h"
#include "ObjectGlobalDataCounter.h"
#include "GraphicsBackend.h"
#include "InputBackend.h"
#include "PlatformBackend.h"
#if defined(_MSC_VER)
#define NR_SUPPRESS_WARNING(n) __pragma(warning(suppress: n))
#else
#define NR_SUPPRESS_WARNING(n)
#endif

constexpr float PI = 3.14159265358979323846f;

void Frame::Initialize()
{
}

void Frame::PostInitialize()
{
}

void Frame::Update()
{
	updateCount++;
	
	float deltaTime = Application::Instance().GetBackend()->platform->GetTimeDelta();
	GameTimer.Update(deltaTime);
	scrollDirty = false;

	DispatchTrueEvents();

	std::vector<unsigned int> handles;
	handles.reserve(ObjectInstances.size());
	for (auto& [handle, instance] : ObjectInstances)
	{
		handles.push_back(handle);
	}

	for (unsigned int handle : handles)
	{
		auto found = ObjectInstances.find(handle);
		if (found == ObjectInstances.end()) continue;
		ObjectInstance* instance = found->second;

		//Animation update
		if (instance->Type == 2) // Common object with possible animation
		{
			((Active*)instance)->movements.Update(deltaTime);

			int movementSpeed = 0;
			int movementMinimumSpeed = 0;
			int movementMaximumSpeed = 0;
			auto* movement = ((Active*)instance)->movements.GetCurrentMovement();
			if (movement != nullptr) {
				movementSpeed = movement->GetRealSpeed().GetIntValue();
				movementMinimumSpeed = movement->GetMinimumSpeed();
				movementMaximumSpeed = movement->GetMaximumSpeed();
			}
			((Active*)instance)->animations.SetAnimationSpeed(movementSpeed, movementMinimumSpeed, movementMaximumSpeed);
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

	//TODO: should timer update be here?
	GameTimer.ProcessNamedEvents([this](const std::string& eventName) {
		OnTimerEvent(eventName);
	});
}

void Frame::DispatchTrueEvents()
{
	if (updateCount == 1)
	{
		GenerateEvent(-3, -1);
	}

	auto input = Application::Instance().GetInput();
	if (input->IsAnyKeyPressed())
	{
		GenerateEvent(-6, -9); // Upon pressing any key
	}

	if (input->IsAnyMouseButtonPressed())
	{
		GenerateEvent(-6, -5); // Upon clicking
		GenerateEvent(-6, -6); // Upon clicking in zone
		GenerateEvent(-6, -7); // Upon clicking on object
	}

	for (int player = 0; player < 4; player++)
	{
		if (input->IsControlsPressed(player, 0xFF))
		{
			GenerateEvent(-7, -4);
			break;
		}
	}
}

void Frame::Draw()
{
	Application::Instance().GetBackend()->graphics->Clear(BackgroundColor);

	for (unsigned int i = 0; i < Layers.size(); i++)
	{
		Layer& layer = Layers[i];
		if (!layer.Visible) continue;
		
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

void Frame::SetScroll(const CValue& x, const CValue& y, int layer)
{
	int windowWidth = Application::Instance().GetAppData()->GetWindowWidth();
	int windowHeight = Application::Instance().GetAppData()->GetWindowHeight();

	float requestedX = static_cast<float>(x.GetIntValue());
	float requestedY = static_cast<float>(y.GetIntValue());

	requestedX -= windowWidth / 2.0f;
	requestedY -= windowHeight / 2.0f;

	if (layer != -1 && layer < static_cast<int>(Layers.size()))
	{
		if (Layers[layer].XCoefficient > 1.0f)
		{
			float dxf = (requestedX - scrollX);
			dxf /= Layers[layer].XCoefficient;
			requestedX = scrollX + dxf;
		}

		if (Layers[layer].YCoefficient > 1.0f)
		{
			float dyf = (requestedY - scrollY);
			dyf /= Layers[layer].YCoefficient;
			requestedY = scrollY + dyf;
		}
	}

	int intX = static_cast<int>(requestedX);
	int intY = static_cast<int>(requestedY);

	intX = std::max(0, intX);
	intY = std::max(0, intY);
	intX = std::min(std::max(0, Width - windowWidth), intX);
	intY = std::min(std::max(0, Height - windowHeight), intY);

	if (intX == scrollX && intY == scrollY) return;
	UpdateScrolling(intX, intY);
	scrollDirty = true;
}

void Frame::SetScrollX(const CValue& x)
{
	int windowWidth = Application::Instance().GetAppData()->GetWindowWidth();
	int intX = x.GetIntValue();
	intX -= windowWidth / 2;
	intX = std::max(0, intX);
	intX = std::min(std::max(0, Width - windowWidth), intX);
	if (intX == scrollX) return;
	UpdateScrolling(intX, scrollY);
	scrollDirty = true;
}

void Frame::SetScrollY(const CValue& y)
{
	int windowHeight = Application::Instance().GetAppData()->GetWindowHeight();
	int intY = y.GetIntValue();
	intY -= windowHeight / 2;
	intY = std::max(0, intY);
	intY = std::min(std::max(0, Height - windowHeight), intY);
	if (intY == scrollY) return;
	UpdateScrolling(scrollX, intY);
	scrollDirty = true;
}

void Frame::UpdateScrolling(int newX, int newY)
{
	int oldX = scrollX;
	int oldY = scrollY;
	int deltaX = newX - oldX;
	int deltaY = newY - oldY;

	scrollX = newX;
	scrollY = newY;

	if (deltaX == 0 && deltaY == 0) return;

	for (auto& [handle, instance] : ObjectInstances)
	{
		if (instance->Type == 0 || instance->Type == 1) continue;

		if (!instance->FollowFrame)
		{
			if (deltaX != 0)
				instance->SetX(CValue(instance->GetX().GetIntValue() + deltaX));
			if (deltaY != 0)
				instance->SetY(CValue(instance->GetY().GetIntValue() + deltaY));
			continue;
		}

		if (instance->Layer >= Layers.size()) continue;
		const Layer& layer = Layers[instance->Layer];

		int oldLayerDx = oldX;
		int oldLayerDy = oldY;
		int newLayerDx = newX;
		int newLayerDy = newY;

		oldLayerDx = static_cast<int>(layer.XCoefficient * oldLayerDx);
		newLayerDx = static_cast<int>(layer.XCoefficient * newLayerDx);
		oldLayerDy = static_cast<int>(layer.YCoefficient * oldLayerDy);
		newLayerDy = static_cast<int>(layer.YCoefficient * newLayerDy);
		int curX = instance->GetX().GetIntValue();
		int curY = instance->GetY().GetIntValue();

		int nX = (curX + oldLayerDx) - newLayerDx + deltaX;
		int nY = (curY + oldLayerDy) - newLayerDy + deltaY;

		if (nX != curX || nY != curY)
			instance->SetPosition(CValue(nX), CValue(nY));
	}
}

CValue Frame::GetXLeftEdge()
{
	return CValue(scrollX);
}

CValue Frame::GetXRightEdge()
{
	return CValue(scrollX + Application::Instance().GetAppData()->GetWindowWidth());
}

CValue Frame::GetYTopEdge()
{
	return CValue(scrollY);
}

CValue Frame::GetYBottomEdge()
{
	return CValue(scrollY + Application::Instance().GetAppData()->GetWindowHeight());
}


void Frame::DrawLayer(Layer& layer)
{
	const int viewWidth = Application::Instance().GetAppData()->GetWindowWidth();
	const int viewHeight = Application::Instance().GetAppData()->GetWindowHeight();
	GraphicsBackend* graphics = Application::Instance().GetBackend()->graphics;

	for (auto& instance : layer.instances)
	{
		//cull objects outside of view
		if (instance->Type <= 3)
		{
			CollisionInstanceBounds bounds = GetInstanceBounds(instance);
			int offsetX = scrollX;
			int offsetY = scrollY;
			if (instance->Type <= 1)
			{
				offsetX = static_cast<int>(offsetX * layer.XCoefficient);
				offsetY = static_cast<int>(offsetY * layer.YCoefficient);
			}

			if (instance->Type == 2 && ((Active*)instance)->AutomaticRotation)
			{
				int dx = std::max<int>(bounds.hotspotX, bounds.width - bounds.hotspotX);
				int dy = std::max<int>(bounds.hotspotY, bounds.height - bounds.hotspotY);
				int radius = (int)std::ceil(std::sqrt((double)dx * dx + (double)dy * dy));
				int cx = bounds.centerX - offsetX;
				int cy = bounds.centerY - offsetY;

				if (cx + radius < 0 || cx - radius > viewWidth ||
					cy + radius < 0 || cy - radius > viewHeight)
					continue;
			}
			else if (bounds.maxX - offsetX < 0 || bounds.minX - offsetX > viewWidth ||
				bounds.maxY - offsetY < 0 || bounds.minY - offsetY > viewHeight)
				continue;
		}

		if (instance->Type == 1)
		{
			//cull objects outside of view
			if (IsOutsideView(instance, static_cast<int>(scrollX * layer.XCoefficient), static_cast<int>(scrollY * layer.YCoefficient), viewWidth, viewHeight))
				continue;

			unsigned int imageId = ((Backdrop*)instance)->Image;

			graphics->DrawTexture(
				imageId, instance->GetX().GetIntValue() - (scrollX * layer.XCoefficient), instance->GetY().GetIntValue() - (scrollY * layer.YCoefficient),
				0, 0, 0, 1.0f, 1.0f, instance->RGBCoefficient, instance->Effect, instance->GetBlendCoefficient().GetIntValue(), instance->effectInstance);
		}
		else if (instance->Type == 2)
		{
			if (!((Active*)instance)->Visible) continue;
			if (((Active*)instance)->GetXScale() <= 0.0f || ((Active*)instance)->GetYScale() <= 0.0f) continue;

			//cull objects outside of view
			if (((Active*)instance)->AutomaticRotation
				? IsRotatedOutsideView(instance, scrollX, scrollY, viewWidth, viewHeight)
				: IsOutsideView(instance, scrollX, scrollY, viewWidth, viewHeight))
				continue;

			auto& imageBank = ImageBank::Instance();
			auto& animations = ((Active*)instance)->animations;
			unsigned int imageId = animations.GetCurrentImageHandle();

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

				graphics->DrawTexture(
					imageId, instance->GetX().GetIntValue() - scrollX, instance->GetY().GetIntValue() - scrollY,
					imageInfo->HotspotX, imageInfo->HotspotY, 
					angle, ((Active*)instance)->GetXScale().GetDoubleValue(), ((Active*)instance)->GetYScale().GetDoubleValue(), instance->RGBCoefficient, instance->Effect, instance->GetBlendCoefficient().GetIntValue(), instance->effectInstance);
			}
		}
		else if (instance->Type == 3) // Text
		{
			if (!((StringObject*)instance)->Visible) continue;
			//cull objects outside of view
			if (IsOutsideView(instance, scrollX, scrollY, viewWidth, viewHeight))
				continue;

			const std::string& text = ((StringObject*)instance)->GetTextRef();
			graphics->DrawText(
				FontBank::Instance().GetFont(((StringObject*)instance)->GetFont()),
				instance->GetX().GetIntValue() - scrollX,
				instance->GetY().GetIntValue() - scrollY,
				((StringObject*)instance)->Width,
				((StringObject*)instance)->Height,
				((StringObject*)instance)->GetHorizontalAlignment(),
				((StringObject*)instance)->GetVerticalAlignment(),
				((StringObject*)instance)->GetColor(),
				text,
				instance->Handle,
				instance->RGBCoefficient,
				instance->Effect,
				instance->GetBlendCoefficient().GetIntValue(),
				instance->effectInstance
			);
		}
		else if (instance->Type == 5 || instance->Type == 6 || instance->Type == 7) // Score, Lives, Counter
		{
			CounterBase* counter = (CounterBase*)instance;
			if (!counter->Visible) continue;
			

			//TODO: Add support for other display types
			if (counter->DisplayType == 1) // Numbers
			{				
				DrawCounterNumbers(counter, counter->GetValue().GetIntValue(), instance->GetX().GetIntValue() - scrollX, instance->GetY().GetIntValue() - scrollY);
			}
			else if (counter->DisplayType == 2 || counter->DisplayType == 3) // Bar
			{
				graphics->DrawCounterBar(instance->GetX().GetIntValue() - scrollX, instance->GetY().GetIntValue() - scrollY, (Counter*)counter);
			}
			else if (counter->DisplayType == 4) // Animation
			{
				if (instance->Type == 6) // Lives
				{
					int count = Application::Instance().GetAppData()->GetPlayerLives()[counter->Player];
					int imageWidth = ImageBank::Instance().GetImage(counter->Frames[0])->Width;
					int imageHeight = ImageBank::Instance().GetImage(counter->Frames[0])->Height;
					int x = instance->GetX().GetIntValue() - scrollX;
					int y = instance->GetY().GetIntValue() - scrollY;
					for (int i = 0; i < count; i++)
					{
						if (counter->Width > 0 && (x - (instance->GetX().GetIntValue() - scrollX)) >= counter->Width)
						{
							x = instance->GetX().GetIntValue() - scrollX; 
							y += imageHeight;
						}

						graphics->DrawTexture(counter->Frames[0], x, y, 0, 0, 0, 1.0f, 1.0f, instance->RGBCoefficient, instance->Effect, instance->GetBlendCoefficient().GetIntValue(), instance->effectInstance);
						x += imageWidth;
					}
				}
				else
				{
					int imageID = 0;
					int frameIndex = 0;
					int imageCount = counter->Frames.size();
					
					if (imageCount > 1)
					{
						int value = ((Counter*)instance)->GetValue().GetIntValue();
						int minValue = ((Counter*)instance)->MinValue.GetIntValue();
						int maxValue = ((Counter*)instance)->MaxValue.GetIntValue();

						if (value >= maxValue)
						{
							frameIndex = imageCount - 1;
						}
						else
						{
							frameIndex = (value - minValue) * (imageCount - 1) / (maxValue - minValue);
							frameIndex = std::clamp(frameIndex, 0, imageCount - 2);
						}
					}
					
					imageID = counter->Frames[frameIndex];
					graphics->DrawTexture(imageID, instance->GetX().GetIntValue() - scrollX, instance->GetY().GetIntValue() - scrollY, 0, 0, 0, 1.0f, 1.0f, instance->RGBCoefficient, instance->Effect, instance->GetBlendCoefficient().GetIntValue(), instance->effectInstance);
				}
			}
		}
		else if (instance->Type == 0) // Quick backdrop
		{
			int scrollXOffset = scrollX * layer.XCoefficient;
			int scrollYOffset = scrollY * layer.YCoefficient;
			//cull objects outside of view
			if (IsOutsideView(instance, scrollXOffset, scrollYOffset, viewWidth, viewHeight))
				continue;

			graphics->DrawQuickBackdrop(instance->GetX().GetIntValue() - scrollXOffset, instance->GetY().GetIntValue() - scrollYOffset, ((QuickBackdrop*)instance)->Width, ((QuickBackdrop*)instance)->Height, &((QuickBackdrop*)instance)->shape);
		}
		else if (instance->Type >= 32) // Extension
		{
			if (!((Extension*)instance)->Visible) continue;
			((Extension*)instance)->Draw(scrollX, scrollY);
		}
	}
}


void Frame::DrawCounterNumbers(CounterBase* counter, int value, int x, int y)
{
	// value is a 32-bit int: at most 10 digits (2147483647), plus one char for '-'
	constexpr int kMaxCounterDigits = 10;
	constexpr int kMaxCounterChars = kMaxCounterDigits + 1;
	assert(counter->IntDigitCount <= kMaxCounterDigits); // assert incase it was not intended
	char buf[kMaxCounterChars];
	auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), value);

	int stringSize = ptr - buf;
	// std::to_chars
	int8_t intDigitCount = counter->IntDigitCount;
	bool negative = value < 0;

	//Fixed Size
	if (intDigitCount > 0)
	{
		int insertPos = negative ? 1 : 0;
		// stringSize includes the '-', intDigitCount does not
		int digitCount = stringSize - insertPos;

		if (intDigitCount > digitCount) //Add leading zeros
		{
			int zeroCount = intDigitCount - digitCount;
			// shift the existing digits to the right to make space for leading zeros
			std::memmove(buf + insertPos + zeroCount, buf + insertPos, digitCount);
			// fill leading zeros right after the sign
			std::memset(buf + insertPos, '0', zeroCount);
		}
		else //Remove extra digits
		{
			std::memmove(buf + insertPos, buf + insertPos + (digitCount - intDigitCount), intDigitCount);
		}

		stringSize = insertPos + intDigitCount;
	}

	//Counter char map
	const ImageBank& imageBank = ImageBank::Instance();
	const ImageInfo* images[kMaxCounterChars];

	int totalWidth = 0;
	int MaxHeight = 0;

	for (int i = 0; i < stringSize; i++)
	{
		NR_SUPPRESS_WARNING(6385)
		int imageIndex = CounterImageIndex(buf[i]);
		const ImageInfo* imageInfo = imageBank.GetImage(counter->Frames[imageIndex]);
		images[i] = imageInfo;
		//Get height of the tallest character displayed
		MaxHeight = std::max((unsigned short)MaxHeight, imageInfo->Height);
		//get the total width of the string to be displayed
		totalWidth += imageInfo->Width;
	}

	int currentX = x - totalWidth;
	int currentY = y - MaxHeight;
	int blend = counter->GetBlendCoefficient().GetIntValue();
	GraphicsBackend* graphics = Application::Instance().GetBackend()->graphics;

	//Draw
	for (int i = 0; i < stringSize; i++)
	{
		auto imageInfo = images[i];
		if (imageInfo)
		{
			int imageIndex = CounterImageIndex(buf[i]);
			graphics->DrawTexture(
				counter->Frames[imageIndex], currentX, currentY,
				0, 0, 0, 1.0f, 1.0f, counter->RGBCoefficient, counter->Effect, blend, counter->effectInstance);
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
	createdInstance->SetPosition(x, y);
	createdInstance->Layer = layer;
	createdInstance->InstanceValue = instanceValue;
	createdInstance->ObjectInfoHandle = objectInfoHandle;
	createdInstance->SetAngle(angle);
	createdInstance->FixedValue = (createdInstance->Handle << 16) | ((createdInstance->Handle-1) & 0xFFFF);

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
		createdInstance->SetX(createdInstance->GetX() + parentInstance->GetX());
		createdInstance->SetY(createdInstance->GetY() + parentInstance->GetY());
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
	// Check collision with all backdrop objects
	for (auto& backdropInstance : Layers[instance->Layer].instances)
	{
		if (backdropInstance->Type != 0 && backdropInstance->Type != 1) continue; // only check against backdrops
		unsigned int obstacleType = (backdropInstance->Type == 0 ? ((QuickBackdrop*)backdropInstance)->ObstacleType : ((Backdrop*)backdropInstance)->ObstacleType);

		if (obstacleType > 0) {
			if (IsColliding(instance, backdropInstance)) return true;
		}

	}
	
	return false; // No collision with any backdrop
}

CollisionInstanceBounds Frame::GetInstanceBounds(ObjectInstance* instance) {
	if (instance->collisionBoundsDirty) {
		CollisionInstanceBounds bounds{};
		bounds.angle = instance->GetAngle();
		bounds.scaleX = 1.0f;
		bounds.scaleY = 1.0f;

		unsigned int imageId = 0;
		int drawX = instance->GetX().GetIntValue();
		int drawY = instance->GetY().GetIntValue();
		int hotspotX = 0, hotspotY = 0;

		if (instance->Type == 0) { // Quick backdrop
			imageId = ((QuickBackdrop*)instance)->shape.Image;
			bounds.width = ((QuickBackdrop*)instance)->Width;
			bounds.height = ((QuickBackdrop*)instance)->Height;
			bounds.maskWidth = bounds.width;
			bounds.maskHeight = bounds.height;
		} else if (instance->Type == 1) { // Backdrop
			imageId = ((Backdrop*)instance)->Image;
			auto imageInfo = ImageBank::Instance().GetImage(imageId);
			if (imageInfo) {
				bounds.width = imageInfo->Width;
				bounds.height = imageInfo->Height;
				bounds.maskWidth = imageInfo->Width;
				bounds.maskHeight = imageInfo->Height;
			}
		} else if (instance->Type == 3) { // String
			StringObject* stringObj = (StringObject*)instance;
			bounds.width = stringObj->Width;
			bounds.height = stringObj->Height;
			bounds.maskWidth = bounds.width;
			bounds.maskHeight = bounds.height;
		} else if (instance->Type == 5 || instance->Type == 6 || instance->Type == 7) { // Counters
			CounterBase* counter = (CounterBase*)instance;
			bounds.width = counter->GetWidth();
			bounds.height = counter->GetHeight();
			bounds.maskWidth = bounds.width;
			bounds.maskHeight = bounds.height;

			if (counter->DisplayType == 1) {
				drawX -= bounds.width;
				drawY -= bounds.height;
			}
		} else { // Active object
			Active* active = (Active*)instance;
			imageId = active->animations.GetCurrentImageHandle();
			bounds.scaleX = active->GetXScale().GetDoubleValue();
			bounds.scaleY = active->GetYScale().GetDoubleValue();
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

		instance->collisionBoundsDirty = false;
		instance->collisionBounds = bounds;
	}

	CollisionInstanceBounds result = instance->collisionBounds;
	result.scrollX = 0;
	result.scrollY = 0;
	if (instance->Layer < Layers.size())
	{
		if (instance->Type == 0 || instance->Type == 1) // Quick backdrop or backdrop
		{
			result.scrollX = static_cast<short>(scrollX * Layers[instance->Layer].XCoefficient);
			result.scrollY = static_cast<short>(scrollY * Layers[instance->Layer].YCoefficient);
		}
		else
		{
			result.scrollX = static_cast<short>(scrollX);
			result.scrollY = static_cast<short>(scrollY);
		}
	}
	return result;
}

bool Frame::IsPointInRotatedBox(int worldX, int worldY, const CollisionInstanceBounds& bounds) {
	float dx = worldX - bounds.centerX + bounds.scrollX;
	float dy = worldY - bounds.centerY + bounds.scrollY;
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

bool Frame::IsPixelSolid(const std::vector<uint8_t>& maskData, int width, int height, int x, int y) {
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
	// Check if the objects are on the same layer
	if (instance1->Layer != instance2->Layer) return false;

	CollisionInstanceBounds bounds1 = GetInstanceBounds(instance1);
	CollisionInstanceBounds bounds2 = GetInstanceBounds(instance2);
	
	if (bounds1.maxX - bounds1.scrollX < bounds2.minX - bounds2.scrollX || bounds1.minX - bounds1.scrollX > bounds2.maxX - bounds2.scrollX ||
		bounds1.maxY - bounds1.scrollY < bounds2.minY - bounds2.scrollY || bounds1.minY - bounds1.scrollY > bounds2.maxY - bounds2.scrollY)
		return false;

	unsigned int imageId1 = 0, imageId2 = 0;
	
	if (instance1->Type == 0) {
		imageId1 = ((QuickBackdrop*)instance1)->shape.Image;
	} else if (instance1->Type == 1) {
		imageId1 = ((Backdrop*)instance1)->Image;
	} else if (instance1->Type == 3 || instance1->Type == 5 || instance1->Type == 6 || instance1->Type == 7) {
		imageId1 = -1;
	} else if (instance2->Type == 2) {
		imageId1 = ((Active*)instance1)->animations.GetCurrentImageHandle();
	}

	if (instance2->Type == 0) {
		imageId2 = ((QuickBackdrop*)instance2)->shape.Image;
	} else if (instance2->Type == 1) {
		imageId2 = ((Backdrop*)instance2)->Image;
	} else if (instance2->Type == 3 || instance2->Type == 5 || instance2->Type == 6 || instance2->Type == 7) {
		imageId2 = -1;
	} else if (instance2->Type == 2) {
		imageId2 = ((Active*)instance2)->animations.GetCurrentImageHandle();
	}
	
	Backend* backend = Application::Instance().GetBackend().get();
	const std::vector<uint8_t> *maskData1 = (unsigned int)imageId1 != -1 ? backend->platform->GetCollisionMaskData(imageId1) : nullptr;
	const std::vector<uint8_t> *maskData2 = (unsigned int)imageId2 != -1 ? backend->platform->GetCollisionMaskData(imageId2) : nullptr;

	bool useMask1 = maskData1 && !maskData1->empty() && (instance1->Type == 1 || (instance1->Type == 2 && ((Active*)instance1)->FineDetection));
	bool useMask2 = maskData2 && !maskData2->empty() && (instance2->Type == 1 || (instance2->Type == 2 && ((Active*)instance2)->FineDetection));
	
	auto imageInfo1 = ImageBank::Instance().GetImage(imageId1);
	auto imageInfo2 = ImageBank::Instance().GetImage(imageId2);
	if (!imageInfo1 || !imageInfo2) {
		int overlapMinX = std::max(bounds1.minX - bounds1.scrollX, bounds2.minX - bounds2.scrollX);
		int overlapMinY = std::max(bounds1.minY - bounds1.scrollY, bounds2.minY - bounds2.scrollY);
		int overlapMaxX = std::min(bounds1.maxX - bounds1.scrollX, bounds2.maxX - bounds2.scrollX);
		int overlapMaxY = std::min(bounds1.maxY - bounds1.scrollY, bounds2.maxY - bounds2.scrollY);
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
	
	int overlapMinX = std::max(bounds1.minX - bounds1.scrollX, bounds2.minX - bounds2.scrollX);
	int overlapMinY = std::max(bounds1.minY - bounds1.scrollY, bounds2.minY - bounds2.scrollY);
	int overlapMaxX = std::min(bounds1.maxX - bounds1.scrollX, bounds2.maxX - bounds2.scrollX);
	int overlapMaxY = std::min(bounds1.maxY - bounds1.scrollY, bounds2.maxY - bounds2.scrollY);
	
	for (int py = overlapMinY; py <= overlapMaxY; py++) {
		for (int px = overlapMinX; px <= overlapMaxX; px++) {
			float dx1 = px - bounds1.centerX + bounds1.scrollX, dy1 = py - bounds1.centerY + bounds1.scrollY;
			if (bounds1.angle != 0) {
				float nx = dx1 * cos1 - dy1 * sin1, ny = dx1 * sin1 + dy1 * cos1;
				dx1 = nx; dy1 = ny;
			}
			int lx1 = (int)(dx1 + hotspotX1), ly1 = (int)(dy1 + hotspotY1);
			int sampleX1 = ConvertScaledCoordToMaskCoord(lx1, bounds1.scaleX, width1);
			int sampleY1 = ConvertScaledCoordToMaskCoord(ly1, bounds1.scaleY, height1);
			bool solid1 = useMask1 ? IsPixelSolid(*maskData1, width1, height1, sampleX1, sampleY1)
				: (lx1 >= 0 && lx1 < bounds1.width && ly1 >= 0 && ly1 < bounds1.height);

			float dx2 = px - bounds2.centerX + bounds2.scrollX, dy2 = py - bounds2.centerY + bounds2.scrollY;
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
	x -= scrollX;
	y -= scrollY;

	CollisionInstanceBounds bounds = GetInstanceBounds(instance);
	if (x < bounds.minX - bounds.scrollX || x > bounds.maxX - bounds.scrollX || y < bounds.minY - bounds.scrollY || y > bounds.maxY - bounds.scrollY)
		return false;

	unsigned int imageId = 0;
	bool fineDetection = false;
	
	if (instance->Type == 0) {
		imageId = ((QuickBackdrop*)instance)->shape.Image;
	} else if (instance->Type == 1) {
		imageId = ((Backdrop*)instance)->Image;
		fineDetection = true;
	}
	if (instance->Type == 3 || instance->Type == 5 || instance->Type == 6 || instance->Type == 7) {
		imageId = -1;
		fineDetection = false;
	}
	else {
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

	float dx = x - bounds.centerX + bounds.scrollX;
	float dy = y - bounds.centerY + bounds.scrollY;
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
