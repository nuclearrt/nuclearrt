#pragma once

#include <unordered_map>
#include <string>
#include <memory>

struct ImageInfo {
    unsigned int Handle;

    int Width;
    int Height;
    short HotspotX;
    short HotspotY;
    short ActionPointX;
    short ActionPointY;
    int TransparentColor;

    ImageInfo(unsigned int handle, int width, int height, short hotspotX, short hotspotY, short actionPointX, short actionPointY, int transparentColor)
        : Handle(handle), Width(width), Height(height), HotspotX(hotspotX), HotspotY(hotspotY), ActionPointX(actionPointX), ActionPointY(actionPointY), TransparentColor(transparentColor) {}
};

class ImageBank {
public:
    static ImageBank& Instance() {
        static ImageBank instance;
        return instance;
    }
    
    std::shared_ptr<ImageInfo> GetImage(unsigned int handle) const {
        auto it = Images.find(handle);
        if (it != Images.end()) {
            return it->second;
        }
        return nullptr;
    }
    
private:
    ImageBank();
    
    std::unordered_map<unsigned int, std::shared_ptr<ImageInfo>> Images;
}; 