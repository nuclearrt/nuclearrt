#include "LayerObjectExtension.h"

#include <algorithm>

#include "Active.h"
#include "Application.h"
#include "Frame.h"

void LayerObjectExtension::Initialize()
{
    currentLayer = Layer;
}

void LayerObjectExtension::YSort(bool ascending)
{
    auto& layer = Application::Instance().GetCurrentFrame().get()->Layers[currentLayer];
    std::sort(layer.instances.begin(), layer.instances.end(), [ascending](ObjectInstance* a, ObjectInstance* b) { return ascending ? a->GetY() < b->GetY() : a->GetY() > b->GetY(); });
}

void LayerObjectExtension::XSort(bool ascending)
{
    auto& layer = Application::Instance().GetCurrentFrame().get()->Layers[currentLayer];
    std::sort(layer.instances.begin(), layer.instances.end(), [ascending](ObjectInstance* a, ObjectInstance* b) { return ascending ? a->GetX() < b->GetX() : a->GetX() > b->GetX(); });
}

void LayerObjectExtension::AltValueSort(bool ascending, int altIndex, int defaultValue)
{
    auto& layer = Application::Instance().GetCurrentFrame().get()->Layers[currentLayer];
    std::sort(layer.instances.begin(), layer.instances.end(), [ascending, altIndex, defaultValue](ObjectInstance* a, ObjectInstance* b) {
        int aValue = defaultValue;
        int bValue = defaultValue;

        if (Active* activeA = dynamic_cast<Active*>(a)) {
            aValue = activeA->Values.GetValue(altIndex);
        }
        if (Active* activeB = dynamic_cast<Active*>(b)) {
            bValue = activeB->Values.GetValue(altIndex);
        }

        return ascending ? aValue < bValue : aValue > bValue;
    });
}