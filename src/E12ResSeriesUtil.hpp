#pragma once
#include <stdint.h>
#include <array>
#include <map>
#include <math.h>
#include <string>

#define __E12RESSERIESUTIL_H__

namespace util
{
    struct ResColor
    {
    private:
        uint8_t _r, _g, _b;

    public:
        ResColor(double rVal, double gVal, double bVal)
        {
            _r = (uint8_t)std::round(rVal * 255);
            _g = (uint8_t)std::round(gVal * 255);
            _b = (uint8_t)std::round(bVal * 255);
        }
        inline double GetR() { return _r / 255.0; }
        inline double GetG() { return _g / 255.0; }
        inline double GetB() { return _b / 255.0; }
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
            std::string result;
            switch (exp)
            {
            case 0: // 33
                result = value;
                break;
            case 1: // 330
                result = value * 10;
                break;
            case 2: // 3.3k
                result = value / 10;
                result += 'k';
                break;
            case 3: // 33k
                result = value;
                result += 'k';
                break;
            case 4: // 330k
                result = value * 10;
                result += 'k';
                break;
            }

            return result;
        }
    };
    const std::array<uint8_t, 12> E12ResSeriesUtil::RES_VALS = {10, 12, 15, 18, 22, 27, 33, 39, 47, 56, 68, 82}; // C++ so silly like that

    class E12ResColors
    {
    private:
        const static std::map<uint8_t, std::pair<ResColor, ResColor>> RES_DIGIT_COLORS;
        const static std::map<uint8_t, ResColor> RES_EXP_COLORS;

    public:
        static std::tuple<ResColor, ResColor, ResColor> GetColors(uint8_t val, uint8_t exp)
        {
            if (RES_DIGIT_COLORS.count(val) == 0 || RES_EXP_COLORS.count(exp) == 0)
            {
                return {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};
            }

            const std::pair<ResColor, ResColor> &digitColors = RES_DIGIT_COLORS.at(val);
            const ResColor &expColor = RES_EXP_COLORS.at(exp);
            return {digitColors.first, digitColors.second, expColor};
        }
    };
    // TODO CONTINUE HERE =================================================================================
    // DEFINE digit colors and exp colors
}