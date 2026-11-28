#pragma once

#include <cmath>
#include <cstdio>
#include <memory>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

class CValue
{
public:
    static constexpr unsigned char TYPE_INT = 0;
    static constexpr unsigned char TYPE_DOUBLE = 1;
    static constexpr unsigned char TYPE_STRING = 2;

    unsigned char type = TYPE_INT;

    CValue() 
    {
        data.i = 0;
    }

    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    CValue(T value)
    {
        data.i = static_cast<int>(value);
    }

    CValue(double value) : type(TYPE_DOUBLE)
    {
        data.d = value;
    }

    CValue(const char* value) : type(TYPE_STRING)
    {
        new (&data.s) std::string(value ? value : "");
    }

    CValue(const std::string& value) : type(TYPE_STRING)
    {
        new (&data.s) std::string(value);
    }

    CValue(std::string&& value) : type(TYPE_STRING)
    {
        new (&data.s) std::string(std::move(value));
    }

    CValue(const CValue& other) : type(other.type)
    {
        if (type == TYPE_STRING)
            new (&data.s) std::string(other.data.s);
        else if (type == TYPE_DOUBLE)
            data.d = other.data.d;
        else
            data.i = other.data.i;
    }

    CValue(CValue&& other) noexcept : type(other.type)
    {
        if (type == TYPE_STRING)
            new (&data.s) std::string(std::move(other.data.s));
        else if (type == TYPE_DOUBLE)
            data.d = other.data.d;
        else
            data.i = other.data.i;
    }

    ~CValue()
    {
        DestroyString();
    }

    CValue& operator=(const CValue& other)
    {
        if (this == &other)
            return *this;

        if (type == TYPE_STRING && other.type == TYPE_STRING)
        {
            data.s = other.data.s;
            return *this;
        }

        DestroyString();
        type = other.type;
        if (type == TYPE_STRING)
            new (&data.s) std::string(other.data.s);
        else if (type == TYPE_DOUBLE)
            data.d = other.data.d;
        else
            data.i = other.data.i;
        return *this;
    }

    CValue& operator=(CValue&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (type == TYPE_STRING && other.type == TYPE_STRING)
        {
            data.s = std::move(other.data.s);
            return *this;
        }

        DestroyString();
        type = other.type;
        if (type == TYPE_STRING)
            new (&data.s) std::string(std::move(other.data.s));
        else if (type == TYPE_DOUBLE)
            data.d = other.data.d;
        else
            data.i = other.data.i;
        return *this;
    }

    char GetType() const { return type; }

    static std::string DoubleToString(double value)
    {
        char buf[32];
        if (value == (double)(int)value)
            std::snprintf(buf, sizeof(buf), "%d", (int)value);
        else
            std::snprintf(buf, sizeof(buf), "%g", value);
        return buf;
    }

    int GetIntValue() const
    {
        if (type == TYPE_INT) return data.i;
        if (type == TYPE_DOUBLE) return (int)data.d;
        return 0;
    }

    double GetDoubleValue() const
    {
        if (type == TYPE_INT) return (double)data.i;
        if (type == TYPE_DOUBLE) return data.d;
        return 0.0;
    }

    const std::string& GetStringValue() const
    {
        if (type == TYPE_STRING)
            return data.s;
        return EmptyString();
    }

    int* GetIntValuePtr()
    {
        if (type != TYPE_INT)
            SetIntValue(GetIntValue());
        return &data.i;
    }

    double* GetDoubleValuePtr()
    {
        if (type != TYPE_DOUBLE)
            SetDoubleValue(GetDoubleValue());
        return &data.d;
    }

    void SetIntValue(int value)
    {
        DestroyString();
        type = TYPE_INT;
        data.i = value;
    }

    void SetDoubleValue(double value)
    {
        DestroyString();
        type = TYPE_DOUBLE;
        data.d = value;
    }

    void SetStringValue(const std::string& value)
    {
        if (type == TYPE_STRING)
        {
            data.s = value;
            return;
        }
        type = TYPE_STRING;
        new (&data.s) std::string(value);
    }

    void SetStringValue(std::string&& value)
    {
        if (type == TYPE_STRING)
        {
            data.s = std::move(value);
            return;
        }
        type = TYPE_STRING;
        new (&data.s) std::string(std::move(value));
    }

    CValue& operator+(const CValue& other)
    {
        switch (type)
        {
            case TYPE_INT:
                if (other.type == TYPE_INT)
                {
                    data.i += other.data.i;
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    ConvertIntToDouble();
                    data.d += other.data.d;
                }
                else if (other.type == TYPE_STRING)
                {
                    ConvertToString();
                    data.s += other.data.s;
                }
                break;
            case TYPE_DOUBLE:
                if (other.type == TYPE_INT)
                {
                    data.d += (double)other.data.i;
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    data.d += other.data.d;
                }
                else if (other.type == TYPE_STRING)
                {
                    ConvertToString();
                    data.s += other.data.s;
                }
                break;
            case TYPE_STRING:
                if (other.type == TYPE_STRING)
                {
                    data.s += other.data.s;
                }
                else if (other.type == TYPE_INT)
                {
                    data.s += std::to_string(other.data.i);
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    data.s += DoubleToString(other.data.d);
                }
                break;
        }
        return *this;
    }

    CValue& operator+(int other)
    {
        switch (type)
        {
            case TYPE_INT:
                data.i += other;
                break;
            case TYPE_DOUBLE:
                data.d += (double)other;
                break;
            case TYPE_STRING:
                data.s += std::to_string(other);
                break;
        }
        return *this;
    }

    CValue& operator+(double other)
    {
        switch (type)
        {
            case TYPE_INT:
                ConvertIntToDouble();
                data.d += other;
                break;
            case TYPE_DOUBLE:
                data.d += other;
                break;
            case TYPE_STRING:
                data.s += DoubleToString(other);
                break;
        }
        return *this;
    }

    CValue& operator+(const std::string& other)
    {
        ConvertToString();
        data.s += other;
        return *this;
    }

    CValue& operator-(const CValue& other)
    {
        switch (type)
        {
            case TYPE_INT:
                if (other.type == TYPE_INT)
                {
                    data.i -= other.data.i;
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    ConvertIntToDouble();
                    data.d -= other.data.d;
                }
                break;
            case TYPE_DOUBLE:
                if (other.type == TYPE_INT)
                {
                    data.d -= (double)other.data.i;
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    data.d -= other.data.d;
                }
                break;
        }
        return *this;
    }

    CValue& operator-(int other)
    {
        switch (type)
        {
            case TYPE_INT:
                data.i -= other;
                break;
            case TYPE_DOUBLE:
                data.d -= (double)other;
                break;
        }
        return *this;
    }

    CValue& operator-(double other)
    {
        switch (type)
        {
            case TYPE_INT:
                ConvertIntToDouble();
                data.d -= other;
                break;
            case TYPE_DOUBLE:
                data.d -= other;
                break;
        }
        return *this;
    }

    CValue& operator-()
    {
        switch (type)
        {
            case TYPE_INT:
                data.i = -data.i;
                break;
            case TYPE_DOUBLE:
                data.d = -data.d;
                break;
        }
        return *this;
    }

    CValue& operator*(const CValue& other)
    {
        switch (type)
        {
            case TYPE_INT:
                if (other.type == TYPE_INT)
                {
                    data.i *= other.data.i;
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    ConvertIntToDouble();
                    data.d *= other.data.d;
                }
                break;
            case TYPE_DOUBLE:
                if (other.type == TYPE_INT)
                {
                    data.d *= (double)other.data.i;
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    data.d *= other.data.d;
                }
                break;
        }
        return *this;
    }

    CValue& operator*(int other)
    {
        switch (type)
        {
            case TYPE_INT:
                data.i *= other;
                break;
            case TYPE_DOUBLE:
                data.d *= (double)other;
                break;
        }
        return *this;
    }

    CValue& operator*(double other)
    {
        switch (type)
        {
            case TYPE_INT:
                ConvertIntToDouble();
                data.d *= other;
                break;
            case TYPE_DOUBLE:
                data.d *= other;
                break;
        }
        return *this;
    }

    CValue& operator/(const CValue& other)
    {
        switch (type)
        {
            case TYPE_INT:
                if (other.type == TYPE_INT)
                {
                    if (other.data.i != 0)
                        data.i /= other.data.i;
                    else
                        data.i = 0;
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    ConvertIntToDouble();
                    if (other.data.d != 0.0)
                        data.d /= other.data.d;
                    else
                        data.d = 0.0;
                }
                break;
            case TYPE_DOUBLE:
                if (other.type == TYPE_INT)
                {
                    if (other.data.i != 0)
                        data.d /= (double)other.data.i;
                    else
                        data.d = 0.0;
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    if (other.data.d != 0.0)
                        data.d /= other.data.d;
                    else
                        data.d = 0.0;
                }
                break;
        }
        return *this;
    }

    CValue& operator%(const CValue& other)
    {
        switch (type)
        {
            case TYPE_INT:
                if (other.type == TYPE_INT)
                {
                    if (other.data.i != 0)
                        data.i %= other.data.i;
                    else
                        data.i = 0;
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    ConvertIntToDouble();
                    if (other.data.d != 0.0)
                        data.d = std::fmod(data.d, other.data.d);
                    else
                        data.d = 0.0;
                }
                break;
            case TYPE_DOUBLE:
                if (other.type == TYPE_INT)
                {
                    if (other.data.i != 0)
                        data.d = std::fmod(data.d, (double)other.data.i);
                    else
                        data.d = 0.0;
                }
                else if (other.type == TYPE_DOUBLE)
                {
                    if (other.data.d != 0.0)
                        data.d = std::fmod(data.d, other.data.d);
                    else
                        data.d = 0.0;
                }
                break;
        }
        return *this;
    }

    CValue& operator%(int other)
    {
        switch (type)
        {
            case TYPE_INT:
                if (other != 0)
                    data.i %= other;
                else
                    data.i = 0;
                break;
            case TYPE_DOUBLE:
                if (other != 0)
                    data.d = std::fmod(data.d, (double)other);
                else
                    data.d = 0.0;
                break;
        }
        return *this;
    }

    CValue& operator%(double other)
    {
        switch (type)
        {
            case TYPE_INT:
                ConvertIntToDouble();
                if (other != 0.0)
                    data.d = std::fmod(data.d, other);
                else
                    data.d = 0.0;
                break;
            case TYPE_DOUBLE:
                if (other != 0.0)
                    data.d = std::fmod(data.d, other);
                else
                    data.d = 0.0;
                break;
        }
        return *this;
    }

    bool operator==(const CValue& other) const
    {
        switch (type)
        {
            case TYPE_INT:
                if (other.type == TYPE_INT)
                    return data.i == other.data.i;
                if (other.type == TYPE_DOUBLE)
                    return (double)data.i == other.data.d;
                break;
            case TYPE_DOUBLE:
                if (other.type == TYPE_INT)
                    return data.d == (double)other.data.i;
                if (other.type == TYPE_DOUBLE)
                    return data.d == other.data.d;
                break;
            case TYPE_STRING:
                if (other.type == TYPE_STRING)
                    return data.s == other.data.s;
                break;
        }
        return false;
    }

    bool operator==(int other) const
    {
        if (type == TYPE_INT) return data.i == other;
        if (type == TYPE_DOUBLE) return data.d == (double)other;
        return false;
    }

    bool operator==(double other) const
    {
        if (type == TYPE_INT) return (double)data.i == other;
        if (type == TYPE_DOUBLE) return data.d == other;
        return false;
    }

    bool operator>(const CValue& other) const
    {
        switch (type)
        {
            case TYPE_INT:
                if (other.type == TYPE_INT)
                    return data.i > other.data.i;
                if (other.type == TYPE_DOUBLE)
                    return (double)data.i > other.data.d;
                break;
            case TYPE_DOUBLE:
                if (other.type == TYPE_INT)
                    return data.d > (double)other.data.i;
                if (other.type == TYPE_DOUBLE)
                    return data.d > other.data.d;
                break;
            case TYPE_STRING:
                if (other.type == TYPE_STRING)
                    return data.s > other.data.s;
                break;
        }
        return false;
    }

    bool operator>(int other) const
    {
        if (type == TYPE_INT) return data.i > other;
        if (type == TYPE_DOUBLE) return data.d > (double)other;
        return false;
    }

    bool operator>(double other) const
    {
        if (type == TYPE_INT) return (double)data.i > other;
        if (type == TYPE_DOUBLE) return data.d > other;
        return false;
    }

    bool operator>=(const CValue& other) const
    {
        switch (type)
        {
            case TYPE_INT:
                if (other.type == TYPE_INT)
                    return data.i >= other.data.i;
                if (other.type == TYPE_DOUBLE)
                    return (double)data.i >= other.data.d;
                break;
            case TYPE_DOUBLE:
                if (other.type == TYPE_INT)
                    return data.d >= (double)other.data.i;
                if (other.type == TYPE_DOUBLE)
                    return data.d >= other.data.d;
                break;
            case TYPE_STRING:
                if (other.type == TYPE_STRING)
                    return data.s >= other.data.s;
                break;
        }
        return false;
    }

    bool operator>=(int other) const
    {
        if (type == TYPE_INT) return data.i >= other;
        if (type == TYPE_DOUBLE) return data.d >= (double)other;
        return false;
    }

    bool operator>=(double other) const
    {
        if (type == TYPE_INT) return (double)data.i >= other;
        if (type == TYPE_DOUBLE) return data.d >= other;
        return false;
    }

    bool operator<(const CValue& other) const
    {
        switch (type)
        {
            case TYPE_INT:
                if (other.type == TYPE_INT)
                    return data.i < other.data.i;
                if (other.type == TYPE_DOUBLE)
                    return (double)data.i < other.data.d;
                break;
            case TYPE_DOUBLE:
                if (other.type == TYPE_INT)
                    return data.d < (double)other.data.i;
                if (other.type == TYPE_DOUBLE)
                    return data.d < other.data.d;
                break;
            case TYPE_STRING:
                if (other.type == TYPE_STRING)
                    return data.s < other.data.s;
                break;
        }
        return false;
    }

    bool operator<(int other) const
    {
        if (type == TYPE_INT) return data.i < other;
        if (type == TYPE_DOUBLE) return data.d < (double)other;
        return false;
    }

    bool operator<(double other) const
    {
        if (type == TYPE_INT) return (double)data.i < other;
        if (type == TYPE_DOUBLE) return data.d < other;
        return false;
    }

    bool operator<=(const CValue& other) const
    {
        switch (type)
        {
            case TYPE_INT:
                if (other.type == TYPE_INT)
                    return data.i <= other.data.i;
                if (other.type == TYPE_DOUBLE)
                    return (double)data.i <= other.data.d;
                break;
            case TYPE_DOUBLE:
                if (other.type == TYPE_INT)
                    return data.d <= (double)other.data.i;
                if (other.type == TYPE_DOUBLE)
                    return data.d <= other.data.d;
                break;
            case TYPE_STRING:
                if (other.type == TYPE_STRING)
                    return data.s <= other.data.s;
                break;
        }
        return false;
    }

    bool operator<=(int other) const
    {
        if (type == TYPE_INT) return data.i <= other;
        if (type == TYPE_DOUBLE) return data.d <= (double)other;
        return false;
    }

    bool operator<=(double other) const
    {
        if (type == TYPE_INT) return (double)data.i <= other;
        if (type == TYPE_DOUBLE) return data.d <= other;
        return false;
    }

    bool operator!=(const CValue& other) const
    {
        return !(*this == other);
    }

    bool operator!=(int other) const
    {
        return !(*this == other);
    }

    bool operator!=(double other) const
    {
        return !(*this == other);
    }

private:
    union Data
    {
        int i;
        double d;
        std::string s;

        Data() {}
        ~Data() {}
    } data;

    static const std::string& EmptyString()
    {
        static const std::string empty;
        return empty;
    }

    void DestroyString()
    {
        if (type == TYPE_STRING)
            std::destroy_at(&data.s);
    }

    void ConvertIntToDouble()
    {
        double value = (double)data.i;
        type = TYPE_DOUBLE;
        data.d = value;
    }

    void ConvertToString()
    {
        if (type == TYPE_STRING)
            return;

        std::string value;
        if (type == TYPE_INT)
            value = std::to_string(data.i);
        else
            value = DoubleToString(data.d);

        type = TYPE_STRING;
        new (&data.s) std::string(std::move(value));
    }
};
