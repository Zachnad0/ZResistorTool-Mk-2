#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <queue>
#include <LiquidCrystal_I2C.h>

// Modes state enum
enum ProgMode : uint8_t
{
    RGBLEDTestMode, // Prints millis() to LCD, sine cycles phases of LEDs. Second line shows most recently pressed key
    RawResMeasure,  // Shows raw meaured resistance. VALUE toggles LEDs that simply show resistance "strength"
    E12ResMeasure,  // Shows measured res rounded to nearest E12, with decade. VALUE toggles LEDs that show color bands.
    E12ResSearch    // Uses VALUE key to cycle through E12 values
};

// Constants
const uint8_t PIN_SDA = 27, PIN_SCL = 26, PIN_RESMEASURE = 36, PIN_BUTTON1 = 35, PIN_BUTTON2 = 34,
              PIN_R1 = 32, PIN_B1 = 33, PIN_G1 = 25, // LED 1
    PIN_R2 = 18, PIN_B2 = 19, PIN_G2 = 21,           // LED 2
    PIN_R3 = 4, PIN_B3 = 16, PIN_G3 = 17;            // LED 3
const uint32_t KEYPRESS_COOLDOWN_MS = 100, LCD_RFRSH_DLY_MS = 200;

// Fields
LiquidCrystal_I2C *_lcdScreen = new LiquidCrystal_I2C{0x27, 16, 2};
uint64_t _tOfLastKey1 = 0, _tOfLastKey2 = 0, _tOfLastLCDRfrsh = 0;
std::queue<uint8_t> *_keypressQueue = new std::queue<uint8_t>{};
ProgMode _currProgramMode = RGBLEDTestMode;
uint8_t _valueKeyState = 0;

double MeasureResistance()
{
    double res2V = 3.3 * analogRead(PIN_RESMEASURE) / 4095.0;
    double current = (3.3 - res2V) / 1000.0;
    return res2V / current;
}

// Handles keypress interrupts
void IRAM_ATTR ISRButton1()
{
    if (millis() - _tOfLastKey1 < KEYPRESS_COOLDOWN_MS || digitalRead(PIN_BUTTON1) == LOW)
        return;

    // Add keypress to queue
    _tOfLastKey1 = millis();
    _keypressQueue->push(1);
}

void IRAM_ATTR ISRButton2()
{
    if (millis() - _tOfLastKey2 < KEYPRESS_COOLDOWN_MS || digitalRead(PIN_BUTTON2) == LOW)
        return;

    // Add keypress to queue
    _tOfLastKey2 = millis();
    _keypressQueue->push(2);
}

void setup()
{
    // Pins setup
    Wire.begin(PIN_SDA, PIN_SCL);
    pinMode(PIN_RESMEASURE, INPUT);
    pinMode(PIN_BUTTON1, INPUT);
    pinMode(PIN_BUTTON2, INPUT);
    attachInterrupt(PIN_BUTTON1, ISRButton1, ONHIGH);
    attachInterrupt(PIN_BUTTON2, ISRButton2, ONHIGH);

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
                _currProgramMode = (ProgMode)(_currProgramMode + 1U);
            }
            _valueKeyState=0;
        }
        // Key 2 is VALUE
        else if (key == 2)
        {
            switch (_currProgramMode)
            {
            default: // TODO =======================================================================
                break;
            }
        }
        _keypressQueue->pop();
    }

    // Refresh LCD
    if (currTime - _tOfLastLCDRfrsh > LCD_RFRSH_DLY_MS)
    {
        _tOfLastLCDRfrsh = currTime;
        _lcdScreen->clear();
        _lcdScreen->print(MeasureResistance());
    }
}