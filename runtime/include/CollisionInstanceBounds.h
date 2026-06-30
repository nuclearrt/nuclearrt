#pragma once

struct CollisionInstanceBounds {
    short minX, minY, maxX, maxY;
    short width, height;
    short centerX, centerY;
    unsigned short angle;
    unsigned short imageId;
    short hotspotX, hotspotY;
    short scrollX, scrollY;
    short maskWidth, maskHeight;
    float scaleX, scaleY;
};