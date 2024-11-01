#pragma once
#include <map>

#define __LEDCONTROL_H__

namespace util
{
    struct LEDConfig
    {
        std::string _name;
        uint8_t _redChnl, _greenChnl, _blueChnl;
        LEDConfig(std::string &name, uint8_t redChnl, uint8_t greenChnl, uint8_t blueChnl)
        {
            _name = name;
            _redChnl = redChnl;
            _greenChnl = greenChnl;
            _blueChnl = blueChnl;
        }
    };

    class LEDControl
    {
    private:
        const uint32_t PWM_FREQ = 244;
        std::map<std::string, LEDConfig> *_leds;

    public:
        LEDControl()
        {
            _leds = new std::map<std::string, LEDConfig>();
        }

        ~LEDControl() { delete _leds; }

        void AddLED(std::string name, uint8_t redPin, uint8_t greenPin, uint8_t bluePin)
        {
            uint8_t n = _leds->size() * 3;
            _leds->emplace(name, LEDConfig{name, n, (uint8_t)(n + 1), (uint8_t)(n + 2)});

            // LEDC PWM setup
            for (uint8_t i = n; i <= n + 2; i++)
            {
                ledcSetup(i, PWM_FREQ, 8);
            }
            ledcAttachPin(redPin, n);
            ledcAttachPin(greenPin, n + 1);
            ledcAttachPin(bluePin, n + 2);
        }

        void WriteLED(std::string name, double redPc, double greenPc, double bluePc)
        {
            // Get LED
            LEDConfig *led = &_leds->at(name);
            if (led == nullptr)
                return;

            // Write to LED
            uint8_t r = (uint8_t)std::round(redPc * 255);
            uint8_t g = (uint8_t)std::round(greenPc * 255);
            uint8_t b = (uint8_t)std::round(bluePc * 255);
            ledcWrite(led->_redChnl, r);
            ledcWrite(led->_greenChnl, g);
            ledcWrite(led->_blueChnl, b);
        }

        void TurnOffAllLEDs()
        {
            // Iterate through and write 0 to all channels
            for (uint16_t i = 0; i < _leds->size() * 3; i++)
            {
                ledcWrite(i, 0);
            }
        }
    };
}