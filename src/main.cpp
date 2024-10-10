#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>
#include <LiquidCrystal_I2C.h>

const uint8_t PIN_SDA = 27, PIN_SCL = 26;
LiquidCrystal_I2C *_lcdScreen = new LiquidCrystal_I2C{0x27, 16, 2};

void setup()
{
    // Startup sequence
    Wire.begin(PIN_SDA, PIN_SCL);
    pinMode(2, OUTPUT);

    // Init devices
    _lcdScreen->init();
    _lcdScreen->backlight();
    _lcdScreen->blink_off();
    _lcdScreen->noCursor();
}

void loop()
{
    digitalWrite(2, HIGH);
    delay(200);
    digitalWrite(2, LOW);
    delay(200);
}