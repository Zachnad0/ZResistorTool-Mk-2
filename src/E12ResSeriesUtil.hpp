#pragma once
#include <stdint.h>
#include <array>
#include <map>
#include <math.h>
#include <string>

#define __E12RESSERIESUTIL_H__
#define __BLACK 0, 0, 0
#define __BROWN 126, 25, 0
#define __RED 255, 0, 0
#define __ORANGE 126, 20, 0
#define __YELLOW 255, 255, 0
#define __GREEN 0, 255, 0
#define __BLUE 0, 0, 255
#define __VIOLET 238, 130, 238
#define __GRAY 128, 128, 128
#define __WHITE 255, 255, 255

namespace util
{
    struct ResColor
    {
    private:
        uint8_t _r, _g, _b;

    public:
        ResColor(uint8_t r, uint8_t g, uint8_t b)
        {
            _r = r;
            _g = g;
            _b = b;
        }
        inline uint8_t GetR() { return _r; }
        inline uint8_t GetG() { return _g; }
        inline uint8_t GetB() { return _b; }

        inline double GetRD() { return _r / 255.0; }
        inline double GetGD() { return _g / 255.0; }
        inline double GetBD() { return _b / 255.0; }
    };

    class E12ResSeriesUtil
    {
    private:
        static const std::array<uint8_t, 12> RES_VALS;
        static const uint8_t EXP_MIN = 0, EXP_MAX = 4;

    public:
        static void FindNearestE12Value(double targetValue, uint8_t &outValue, uint8_t &outExp)
        {
            // Search through all values and find nearest, which is when the previous error is LESS than the current
            double prevErr = 999999999;
            for (uint8_t currExp = EXP_MIN; currExp <= EXP_MAX; currExp++)
            {
                for (uint8_t rvI = 0; rvI < RES_VALS.size(); rvI++)
                {
                    double currErr = std::pow((double)RES_VALS[rvI] * std::pow(10, currExp) - targetValue, 2);
                    if (prevErr <= currErr)
                    {
                        if (rvI == 0)
                        {
                            outValue = *RES_VALS.end();
                            outExp = currExp - 1;
                        }
                        else
                        {
                            outValue = RES_VALS[rvI - 1];
                            outExp = currExp;
                        }
                        return;
                    }
                    prevErr = currErr;
                }
            }

            outValue = 0;
            outExp = 0;
        }

        static std::string ResValToString(uint8_t value, uint8_t exp)
        {
            // E.g. 330Ω, 6.8kΩ
            std::string result = "ERR";
            switch (exp)
            {
            case 0: // 33
                result = std::to_string(value);
                break;
            case 1: // 330
                result = std::to_string(value * 10);
                break;
            case 2: // 3.3k
                result = std::to_string((uint8_t)std::floor(value / 10.0));
                result += '.';
                result += std::to_string(value % 10);
                result += 'k';
                break;
            case 3: // 33k
                result = std::to_string(value);
                result += 'k';
                break;
            case 4: // 330k
                result = std::to_string(value * 10);
                result += 'k';
                break;
            }

            return result;
        }

        static void GetValueFromSearchIndex(uint8_t index, uint8_t &val, uint8_t &exp)
        {
            // Essentially base-12 counting
            val = RES_VALS.at(index % 12);
            exp = (uint8_t)std::floor(index / 12.0);
        }
    };
    const std::array<uint8_t, 12> E12ResSeriesUtil::RES_VALS = {10, 12, 15, 18, 22, 27, 33, 39, 47, 56, 68, 82}; // C++ so silly like that

    class E12ResColors
    {
    private:
        const static std::map<uint8_t, ResColor> RES_DIGIT_COLORS;

    public:
        static std::tuple<ResColor, ResColor, ResColor> GetColors(uint8_t val, uint8_t exp)
        {
            uint8_t digit1 = (uint8_t)std::floor(val * 0.1), digit2 = val % 10;

            if (RES_DIGIT_COLORS.count(digit1) == 0 || RES_DIGIT_COLORS.count(digit2) == 0 || RES_DIGIT_COLORS.count(exp) == 0)
            {
                return {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};
            }

            const ResColor
                &digit1Color = RES_DIGIT_COLORS.at(digit1),
                &digit2Color = RES_DIGIT_COLORS.at(digit2),
                &expColor = RES_DIGIT_COLORS.at(exp);

            return {digit1Color, digit2Color, expColor};
        }
    };
    const std::map<uint8_t, ResColor> E12ResColors::RES_DIGIT_COLORS =
        {
            {0, {__BLACK}},
            {1, {__BROWN}},
            {2, {__RED}},
            {3, {__ORANGE}},
            {4, {__YELLOW}},
            {5, {__GREEN}},
            {6, {__BLUE}},
            {7, {__VIOLET}},
            {8, {__GRAY}},
            {9, {__WHITE}},
    };
}