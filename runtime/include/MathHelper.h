#pragma once
#include <iostream>
#include <cmath>
#include <string>
#include <algorithm>

namespace MathHelper {
    
    template<typename T>
    struct SafeDivResult {
        T lhs;
        explicit SafeDivResult(T val) : lhs(val) {}

        template<typename TR>
        auto operator/(TR rhs) const {
            // static_cast ensures we don't do integer division (matches Clickteam)
            return (rhs == 0) ? 0.0 : (static_cast<double>(lhs) / rhs);
        }
    };

    struct SafeDivision {
        template<typename T>
        SafeDivResult<T> operator/(T num) const {
            return SafeDivResult<T>(num);
        }
    };

    template<typename T>
    SafeDivResult<T> operator/(T lhs, const SafeDivision&) {
        return SafeDivResult<T>(lhs);
    }

    const SafeDivision& GetSafeDivision();


    template<typename T>
    struct PowResult {
        T lhs;
        explicit PowResult(T val) : lhs(val) {}

        template<typename TR>
        auto operator/(TR rhs) const {
            return std::pow(lhs, rhs);
        }
    };

    struct Power {
        template<typename T>
        PowResult<T> operator/(T num) const {
            return PowResult<T>(num);
        }
    };

    template<typename T>
    PowResult<T> operator/(T lhs, const Power&) {
        return PowResult<T>(lhs);
    }

    const Power& GetPower();

    // trig/math helpers (NOTE: ctf uses degrees, C++ uses radians)
    inline double ToRadians(double degrees) { return degrees * (3.14159265358979323846 / 180.0); }
    inline double ToDegrees(double radians) { return radians * (180.0 / 3.14159265358979323846); }

    inline double Sin(double degrees) { return std::sin(ToRadians(degrees)); }
    inline double Cos(double degrees) { return std::cos(ToRadians(degrees)); }
    inline double Tan(double degrees) { return std::tan(ToRadians(degrees)); }
    inline double ASin(double v) { return ToDegrees(std::asin(v)); }
    inline double ACos(double v) { return ToDegrees(std::acos(v)); }
    inline double ATan(double v) { return ToDegrees(std::atan(v)); }
    inline double ATan2(double y, double x) { return ToDegrees(std::atan2(y, x)); }

    // color helpers
    inline int GetRGB(int r, int g, int b) { return (r << 16) | (g << 8) | b; }
    inline int GetRed(int rgb) { return (rgb >> 16) & 0xFF; }
    inline int GetGreen(int rgb) { return (rgb >> 8) & 0xFF; }
    inline int GetBlue(int rgb) { return rgb & 0xFF; }

    // math utility helpers
    inline double Distance(double x1, double y1, double x2, double y2) { return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2)); }
    
    inline double VAngle(double x, double y) { 
        double angle = ToDegrees(std::atan2(-y, x));
        while (angle < 0) angle += 360.0;
        while (angle >= 360) angle -= 360.0;
        return angle;
    }

    // a guess
    inline double DistanceFromAngle(double x1, double y1, double x2, double y2) {
        return VAngle(x2 - x1, y2 - y1);
    }

    inline double Range(double v, double minVal, double maxVal) { return (v < minVal) ? minVal : (v > maxVal ? maxVal : v); }
    inline int Stoi(const std::string& str) {
        if (str.empty()) {
            return 0;
        }

        return std::stoi(str);
    }
    inline double Stod(const std::string& str) {
        if (str.empty()) {
            return 0.0;
        }

        return std::stod(str);
    }
}