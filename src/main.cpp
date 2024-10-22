#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <queue>
#include <vector>
#include <numeric>
#include <string.h>
#include <LiquidCrystal_I2C.h>
#include "E12ResSeriesUtil.hpp"

// Modes state enum
enum ProgMode : uint8_t
{
    RGBLEDTestMode, // Prints millis() to LCD, sine cycles phases of LEDs. Second line shows bool toggled by VALUE
    RawResMeasure,  // Shows raw meaured resistance. VALUE toggles LEDs that simply show resistance "strength"
    E12ResMeasure,  // Shows measured res rounded to nearest E12, with decade. VALUE toggles LEDs that show color bands.
    E12ResSearch    // Uses VALUE key to cycle through E12 values
};

// Constants
const uint8_t PIN_SDA = 27, PIN_SCL = 26, PIN_RESMEASURE = 39, PIN_BUTTON1 = 35, PIN_BUTTON2 = 34,
              PIN_R1 = 32, PIN_B1 = 33, PIN_G1 = 25, // LED 1
    PIN_R2 = 18, PIN_B2 = 19, PIN_G2 = 21,           // LED 2
    PIN_R3 = 4, PIN_B3 = 16, PIN_G3 = 17;            // LED 3
const uint32_t KEYPRESS_COOLDOWN_MS = 100, LCD_RFRSH_DLY_MS = 200, ADC_SAMPLE_COUNT = 10, RESMEASURE_RES1 = 2200;

// Fields
LiquidCrystal_I2C *_lcdScreen = new LiquidCrystal_I2C{0x27, 16, 2};
uint64_t _tOfLastKey1 = 0, _tOfLastKey2 = 0, _tOfLastLCDRfrsh = 0, TEMPKPR = 0;
std::queue<uint8_t> *_keypressQueue = new std::queue<uint8_t>{};
std::vector<uint32_t> *_adcRecentSamples = new std::vector<uint32_t>{};
ProgMode _currProgramMode = RGBLEDTestMode;
uint8_t _valueKeyState = 0;

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
}

void loop()
{
    uint64_t currTime = millis();
    // Update ADC readings
    UpdateADCSamples();

    // Handle keypresses
    while (!_keypressQueue->empty())
    {
        uint8_t key = _keypressQueue->front();
        // Key 1 is MODE
        if (key == 1)
        {
            if (_currProgramMode == E12ResSearch)
            {
                _currProgramMode = RGBLEDTestMode;
            }
            else
            {
                // _currProgramMode = (ProgMode)(_currProgramMode + 1U);
            }
            // _valueKeyState = 0;
        }
        // Key 2 is VALUE
        else if (key == 2)
        {
            switch (_currProgramMode)
            {
            case RGBLEDTestMode:
                _valueKeyState = (_valueKeyState == 0) ? 1 : 0;
                break;
            }
        }

        TEMPKPR++;
        _keypressQueue->pop();
    }

    // Check LCD refresh due
    if (currTime - _tOfLastLCDRfrsh > LCD_RFRSH_DLY_MS)
    {
        _tOfLastLCDRfrsh = currTime;
        _lcdScreen->clear();
        switch (_currProgramMode)
        {
        case RGBLEDTestMode:
            _lcdScreen->print(millis());
            _lcdScreen->print(" M:");
            double mr = MeasureResistance();
            _lcdScreen->print(mr);
            _lcdScreen->setCursor(0, 1);
            uint8_t rVal = 0, rExp = 0;
            util::E12ResSeriesUtil::FindNearestE12Value(mr, rVal, rExp);
            _lcdScreen->print("V:");
            _lcdScreen->print(rVal);
            _lcdScreen->print(" E:");
            _lcdScreen->print(rExp);
            // _lcdScreen->print(MeasureResistance());
            // _lcdScreen->write(0xF4);
            break;
        }
    }

    delay(100);
}