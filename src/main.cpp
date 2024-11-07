#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <queue>
#include <vector>
#include <numeric>
#include <string.h>
#include <LiquidCrystal_I2C.h>
#include "E12ResSeriesUtil.hpp"
#include "LEDControl.hpp"

#define __sineWave0To1(phase) 0.5 * sin((FREQ * currMillis * (2 * PI / 1000)) + phase) + 0.5

// Modes state enum
enum ProgMode : uint8_t
{
    RGBLEDTestMode = 3, // Prints millis() to LCD, sine cycles phases of LEDs. Second line shows bool toggled by VALUE
    RawResMeasure = 1,  // Shows raw meaured resistance. VALUE toggles LEDs that simply show resistance "strength"
    E12ResMeasure = 0,  // Shows measured res rounded to nearest E12, with decade. VALUE toggles LEDs that show color bands.
    E12ResSearch = 2    // Uses VALUE key to cycle through E12 values
};

// Constants
const uint8_t PIN_SDA = 27, PIN_SCL = 26, PIN_RESMEASURE = 39, PIN_BUTTON2 = 35, PIN_BUTTON1 = 34,
              PIN_R1 = 18, PIN_B1 = 19, PIN_G1 = 21, // LED 1
    PIN_R2 = 32, PIN_B2 = 33, PIN_G2 = 25,           // LED 2
    PIN_R3 = 4, PIN_B3 = 16, PIN_G3 = 17;            // LED 3
const uint32_t KEYPRESS_COOLDOWN_MS = 100, LCD_RFRSH_DLY_MS = 200, ADC_SAMPLE_COUNT = 10, RESMEASURE_RES1 = 2200;
const std::string LED_DIGIT1 = "digit1", LED_DIGIT2 = "digit2", LED_EXP = "exp";
const std::map<ProgMode, uint32_t> MODE_LOOP_DELAY = {{RGBLEDTestMode, 1 / 120.0}};

// Fields
LiquidCrystal_I2C *_lcdScreen = new LiquidCrystal_I2C{0x27, 16, 2};
util::LEDControl *_ledController = new util::LEDControl{};
uint64_t _tOfLastKey1 = 0, _tOfLastKey2 = 0, _tOfLastLCDRfrsh = 0, TEMPKPR = 0;
std::queue<uint8_t> *_keypressQueue = new std::queue<uint8_t>{};
std::vector<uint32_t> *_adcRecentSamples = new std::vector<uint32_t>{};
ProgMode _currProgramMode = (ProgMode)0;
uint8_t _valueKeyState = 0;
bool _firstFrameOfCurrMode = true;

void UpdateADCSamples()
{
    _adcRecentSamples->push_back(analogReadMilliVolts(PIN_RESMEASURE));
    if (_adcRecentSamples->size() > ADC_SAMPLE_COUNT)
    {
        _adcRecentSamples->erase(_adcRecentSamples->begin());
    }
}

double GetADCAverage()
{
    if (_adcRecentSamples->empty())
    {
        return 0;
    }
    return std::accumulate(_adcRecentSamples->begin(), _adcRecentSamples->end(), 0) / (double)_adcRecentSamples->size();
}

double MeasureResistance()
{
    // See https://www.desmos.com/calculator/h8w3dxdxvp for range
    double res2V = GetADCAverage() / 1000;
    double current = (3.3 - res2V) / RESMEASURE_RES1;
    double res2 = res2V / current;
    return res2 > 1000000 ? -1 : res2;
}

// Handles keypress interrupts
void IRAM_ATTR ISRButton1()
{
    if (millis() - _tOfLastKey1 < KEYPRESS_COOLDOWN_MS)
        return;

    // Add keypress to queue
    _tOfLastKey1 = millis();
    _keypressQueue->push(1);
}

void IRAM_ATTR ISRButton2()
{
    if (millis() - _tOfLastKey2 < KEYPRESS_COOLDOWN_MS)
        return;

    // Add keypress to queue
    _tOfLastKey2 = millis();
    _keypressQueue->push(2);
}

void setup()
{
    _adcRecentSamples->reserve(ADC_SAMPLE_COUNT);

    // Pins setup
    Wire.begin(PIN_SDA, PIN_SCL);
    pinMode(PIN_RESMEASURE, INPUT);
    pinMode(PIN_BUTTON1, INPUT);
    pinMode(PIN_BUTTON2, INPUT);
    attachInterrupt(PIN_BUTTON1, ISRButton1, HIGH);
    attachInterrupt(PIN_BUTTON2, ISRButton2, HIGH);

    pinMode(PIN_R1, OUTPUT);
    pinMode(PIN_B1, OUTPUT);
    pinMode(PIN_G1, OUTPUT);
    pinMode(PIN_R2, OUTPUT);
    pinMode(PIN_B2, OUTPUT);
    pinMode(PIN_G2, OUTPUT);
    pinMode(PIN_R3, OUTPUT);
    pinMode(PIN_B3, OUTPUT);
    pinMode(PIN_G3, OUTPUT);

    // Init LCD
    _lcdScreen->init();
    _lcdScreen->backlight();
    _lcdScreen->noBlink();
    _lcdScreen->noCursor();

    // Init LEDs
    _ledController->AddLED(LED_DIGIT1, PIN_R1, PIN_G1, PIN_B1);
    _ledController->AddLED(LED_DIGIT2, PIN_R2, PIN_G2, PIN_B2);
    _ledController->AddLED(LED_EXP, PIN_R3, PIN_G3, PIN_B3);

    // Bootup Sequence
    _lcdScreen->print("ZResistorToolMk2");

    _lcdScreen->setCursor(0, 1);
    _lcdScreen->print("------R--R--R---");
    _ledController->WriteLED(LED_DIGIT1, 1, 0, 0);
    _ledController->WriteLED(LED_DIGIT2, 1, 0, 0);
    _ledController->WriteLED(LED_EXP, 1, 0, 0);
    delay(500);
    _lcdScreen->setCursor(0, 1);
    _lcdScreen->print("------G--G--G---");
    _ledController->WriteLED(LED_DIGIT1, 0, 1, 0);
    _ledController->WriteLED(LED_DIGIT2, 0, 1, 0);
    _ledController->WriteLED(LED_EXP, 0, 1, 0);
    delay(500);
    _lcdScreen->setCursor(0, 1);
    _lcdScreen->print("------B--B--B---");
    _ledController->WriteLED(LED_DIGIT1, 0, 0, 1);
    _ledController->WriteLED(LED_DIGIT2, 0, 0, 1);
    _ledController->WriteLED(LED_EXP, 0, 0, 1);
    delay(500);
}

void loop()
{
    uint64_t currMillis = millis();
    // Update ADC readings, cache measured resistance
    UpdateADCSamples();
    double currMesRes = MeasureResistance();
    uint8_t currResVal = 0, currResExp = 0xFF;
    util::E12ResSeriesUtil::FindNearestE12Value(currMesRes, currResVal, currResExp);

    // Handle keypresses
    while (!_keypressQueue->empty())
    {
        uint8_t key = _keypressQueue->front();
        // Key 1 is MODE
        if (key == 1)
        {
            if (_currProgramMode == (ProgMode)3)
            {
                _currProgramMode = (ProgMode)0;
            }
            else
            {
                _currProgramMode = (ProgMode)(_currProgramMode + 1U);
            }
            _valueKeyState = 0;
            _firstFrameOfCurrMode = true;
        }
        // Key 2 is VALUE
        else if (key == 2)
        {
            switch (_currProgramMode)
            {
            case E12ResSearch:
                // Cycle through all E12 values, then increment exponent by 1
                // Hahaha, this will be very annoying to use!
                _valueKeyState++;
                if (_valueKeyState >= 12 * 4)
                {
                    _valueKeyState = 0;
                }

            default:
                _valueKeyState = (_valueKeyState == 0) ? 1 : 0;
                break;
            }
        }

        TEMPKPR++;
        _keypressQueue->pop();
    }

    // Do standard loop actions for current mode, control LEDs here
    switch (_currProgramMode)
    {
    case RGBLEDTestMode:
        if (_valueKeyState == 0)
        {
            // Cycle RGB colors depending on function
            const double FREQ = 1, PHI1 = 0, PHI2 = 2 * PI / 3, PHI3 = 4 * PI / 3;
            _ledController->WriteLED(LED_DIGIT1, __sineWave0To1(PHI1), __sineWave0To1(PHI2), __sineWave0To1(PHI3));
            _ledController->WriteLED(LED_DIGIT2, __sineWave0To1(PHI3), __sineWave0To1(PHI1), __sineWave0To1(PHI2));
            _ledController->WriteLED(LED_EXP, __sineWave0To1(PHI2), __sineWave0To1(PHI3), __sineWave0To1(PHI1));
        }
        else
        {
            _ledController->TurnOffAllLEDs();
        }
        break;

    case E12ResMeasure:
        if (_valueKeyState == 0)
        {
            // LEDs are colors of bands
            std::tuple<util::ResColor, util::ResColor, util::ResColor> bandColors = util::E12ResColors::GetColors(currResVal, currResExp);
            _ledController->WriteLED(LED_DIGIT1, std::get<0>(bandColors));
            _ledController->WriteLED(LED_DIGIT2, std::get<1>(bandColors));
            _ledController->WriteLED(LED_EXP, std::get<2>(bandColors));
        }
        else
        {
            _ledController->TurnOffAllLEDs();
        }
        break;

    case E12ResSearch:
    {
        // Get values
        uint8_t val = 0, exp = 0;
        util::E12ResSeriesUtil::GetValueFromSearchIndex(_valueKeyState, val, exp);
        // Write LEDS
        std::tuple<util::ResColor, util::ResColor, util::ResColor> bandColors = util::E12ResColors::GetColors(val, exp);
        _ledController->WriteLED(LED_DIGIT1, std::get<0>(bandColors));
        _ledController->WriteLED(LED_DIGIT2, std::get<1>(bandColors));
        _ledController->WriteLED(LED_EXP, std::get<2>(bandColors));
    }
    break;
    }

    // Check LCD refresh due
    if (currMillis - _tOfLastLCDRfrsh > LCD_RFRSH_DLY_MS)
    {
        _tOfLastLCDRfrsh = currMillis;
        if (_firstFrameOfCurrMode)
        {
            _lcdScreen->clear();
        }

        switch (_currProgramMode)
        {
        case RGBLEDTestMode:
            if (_firstFrameOfCurrMode)
            {
                _lcdScreen->print("RGBLEDTest:");
            }
            _lcdScreen->setCursor(0, 1);
            _lcdScreen->print(currMillis);
            break;

        case RawResMeasure:
            if (_firstFrameOfCurrMode)
            {
                // Static text
                _lcdScreen->print("Raw Reading:");
            }
            // Dynamic data
            _lcdScreen->setCursor(0, 1);
            _lcdScreen->print("               ");
            _lcdScreen->setCursor(0, 1);
            _lcdScreen->print("R:");
            _lcdScreen->print(currMesRes);
            break;

        case E12ResMeasure:
            if (_firstFrameOfCurrMode)
            {
                _lcdScreen->print("E12 Reading:");
                _lcdScreen->setCursor(0, 1);
                _lcdScreen->print("R:");
            }
            // Display
            _lcdScreen->setCursor(2, 1);
            _lcdScreen->print("              ");
            _lcdScreen->setCursor(2, 1);
            _lcdScreen->print(util::E12ResSeriesUtil::ResValToString(currResVal, currResExp).c_str());
            // {
            //     std::tuple<util::ResColor, util::ResColor, util::ResColor> bandColors = util::E12ResColors::GetColors(currResVal, currResExp);
            //     _lcdScreen->print(" f:");
            //     _lcdScreen->print(std::get<0>(bandColors).GetR());
            //     _lcdScreen->print(std::get<0>(bandColors).GetG());
            //     _lcdScreen->print(std::get<0>(bandColors).GetB());
            // }
            break;

        case E12ResSearch:
            if (_firstFrameOfCurrMode)
            {
                _lcdScreen->print("E12 Search:");
                _lcdScreen->setCursor(0, 1);
                _lcdScreen->print("R:");
            }
            {
                uint8_t val = 0, exp = 0;
                util::E12ResSeriesUtil::GetValueFromSearchIndex(_valueKeyState, val, exp);
                _lcdScreen->setCursor(2, 1);
                _lcdScreen->print("              ");
                _lcdScreen->setCursor(2, 1);
                _lcdScreen->print(util::E12ResSeriesUtil::ResValToString(val, exp).c_str());
            }
            break;
        }

        // Top-right shows current mode
        _lcdScreen->setCursor(14, 0);
        _lcdScreen->write(0xE4); // μ
        _lcdScreen->print(_currProgramMode);
        _firstFrameOfCurrMode = false;
    }

    // Delay loop based on current program mode
    auto delayIter = MODE_LOOP_DELAY.find(_currProgramMode);
    delay(delayIter == MODE_LOOP_DELAY.end() ? 100U : delayIter->second);
}