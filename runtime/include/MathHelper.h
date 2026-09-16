#pragma once

#include "CValue.h"

#include <iostream>
#include <cmath>
#include <string>
#include <algorithm>
#include <type_traits>

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


    inline double AsDouble(double v) { return v; }
    inline double AsDouble(float v) { return static_cast<double>(v); }
    inline double AsDouble(int v) { return static_cast<double>(v); }
    inline double AsDouble(const CValue& v) { return v.GetDoubleValue(); }

    template<typename T>
    struct PowResult {
        T lhs;
        explicit PowResult(T val) : lhs(val) {}

        template<typename TR>
        auto operator/(TR rhs) const {
            double result = std::pow(AsDouble(lhs), AsDouble(rhs));
            if constexpr (std::is_same_v<std::decay_t<T>, CValue> || std::is_same_v<std::decay_t<TR>, CValue>)
                return CValue(result);
            else
                return result;
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
    
    inline CValue Sin(const CValue& v) { return CValue(Sin(v.GetDoubleValue())); }
    inline CValue Sqrt(const CValue& v) { return CValue(std::sqrt(v.GetDoubleValue())); }
    inline CValue Abs(const CValue& v) { return CValue(std::abs(v.GetDoubleValue())); }
    inline CValue Round(const CValue& v) { return CValue(std::round(v.GetDoubleValue())); }
    inline CValue Floor(const CValue& v) { return CValue(std::floor(v.GetDoubleValue())); }
    inline CValue Ceil(const CValue& v) { return CValue(std::ceil(v.GetDoubleValue())); }
    inline CValue Trunc(const CValue& v) { return CValue(std::trunc(v.GetDoubleValue())); }
    inline CValue Min(const CValue& a, const CValue& b) { return CValue(std::min(a.GetDoubleValue(), b.GetDoubleValue())); }
    inline CValue Max(const CValue& a, const CValue& b) { return CValue(std::max(a.GetDoubleValue(), b.GetDoubleValue())); }

    inline CValue ToValue(const CValue& v) {
        if (v.GetType() != CValue::TYPE_STRING)
            return v;

        const std::string& str = v.GetStringValue();
        if (str.empty())
            return CValue(0);
        if (str.find('.') != std::string::npos)
            return CValue(std::stod(str));
        return CValue(std::stoi(str));
    }

    // color helpers
    inline CValue GetRGB(const CValue& r, const CValue& g, const CValue& b) { return CValue(r.GetIntValue() << 16 | g.GetIntValue() << 8 | b.GetIntValue()); }
    inline int GetRGB(int r, int g, int b) { return (r << 16) | (g << 8) | b; }
    inline int GetRed(int rgb) { return (rgb >> 16) & 0xFF; }
    inline int GetGreen(int rgb) { return (rgb >> 8) & 0xFF; }
    inline int GetBlue(int rgb) { return rgb & 0xFF; }

    // math utility helpers
    inline double Distance(double x1, double y1, double x2, double y2) { return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2)); }
    inline CValue Distance(const CValue& x1, const CValue& y1, const CValue& x2, const CValue& y2) { return Distance(x1.GetDoubleValue(), y1.GetDoubleValue(), x2.GetDoubleValue(), y2.GetDoubleValue()); }
    
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
    inline CValue Range(const CValue& v, const CValue& minVal, const CValue& maxVal) { return CValue(Range(v.GetDoubleValue(), minVal.GetDoubleValue(), maxVal.GetDoubleValue())); }
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