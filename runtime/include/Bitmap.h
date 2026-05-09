#pragma once

#include <vector>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <cstdlib>
#include <cmath>

class Bitmap {
public:
    Bitmap() : width(0), height(0) {}

    Bitmap(int width, int height)
    {
        Init(width, height);
    }

    void Resize(int width, int height) {
        Init(width, height);
    }

    void Clear(int color) {
        if (color == 0 && !data.empty()) {
            std::memset(data.data(), 0, data.size() * sizeof(unsigned int));
            return;
        }
        const unsigned int u = static_cast<unsigned int>(color);
        std::fill(data.begin(), data.end(), u);
    }

    void SetPixel(int x, int y, int color) {
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        data[y * width + x] = color;
    }

    void DrawLine(int x1, int y1, int x2, int y2, int color) {
        int dx = x2 - x1;
        int dy = y2 - y1;
        int steps = std::max(abs(dx), abs(dy));
        if (steps == 0) {
            SetPixel(x1, y1, color);
            return;
        }
        float xIncrement = dx / (float)steps;
        float yIncrement = dy / (float)steps;
        float x = (float)x1;
        float y = (float)y1;
        for (int i = 0; i <= steps; i++) {
            SetPixel((int)x, (int)y, color);
            x += xIncrement;
            y += yIncrement;
        }
    }

    void DrawRectangle(int x, int y, int width, int height, int color) {
        for (int i = x; i < x + width; i++) {
            for (int j = y; j < y + height; j++) {
                SetPixel(i, j, color);
            }
        }
    }

    void DrawRectangleLines(int x, int y, int width, int height, int color) {
        DrawLine(x, y, x + width - 1, y, color);
        DrawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
        DrawLine(x + width - 1, y + height - 1, x, y + height - 1, color);
        DrawLine(x, y + height - 1, x, y, color);
    }

    void DrawEllipseLines(int x, int y, int width, int height, int color) {
        if (width <= 0 || height <= 0) {
            return;
        }
        if (width == 1) {
            DrawLine(x, y, x, y + height - 1, color);
            return;
        }
        if (height == 1) {
            DrawLine(x, y, x + width - 1, y, color);
            return;
        }

        const double cx = x + (width - 1) * 0.5;
        const double cy = y + (height - 1) * 0.5;
        const double rx = (width - 1) * 0.5;
        const double ry = (height - 1) * 0.5;

        if (rx <= 0.0 || ry <= 0.0) {
            DrawRectangleLines(x, y, width, height, color);
            return;
        }

        const int steps = std::max(48, (width + height) * 4);
        int lx = 0;
        int ly = 0;
        bool hasLast = false;
        for (int i = 0; i <= steps; i++) {
            double t = (i * 2.0 * 3.14159265358979323846) / steps;
            int px = static_cast<int>(std::lround(cx + rx * std::cos(t)));
            int py = static_cast<int>(std::lround(cy + ry * std::sin(t)));
            if (hasLast) {
                DrawLine(lx, ly, px, py, color);
            }
            lx = px;
            ly = py;
            hasLast = true;
        }
    }

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    uint32_t* GetData() { return data.data(); }
private:
    std::vector<uint32_t> data;
    int width;
    int height;

    void Init(int width, int height) {
        this->width = width;
        this->height = height;
        const size_t count = static_cast<size_t>(width) * static_cast<size_t>(height);
        data.assign(count, 0u);
    }
};