#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h> 

#define LED_PIN   4
#define LED_COUNT 60 
#define LED_BRIGHTNESS 25

// #define COLOR_TICKS   0x080822
// #define COLOR_HOURS_N 0x000044
// #define COLOR_HOURS_D 0x3333AA
// #define COLOR_MINUTES 0xFF0000
// #define COLOR_SECONDS 0x001100 

#define IDC_TICKS     0x00
#define IDC_HOURS_N   0x01
#define IDC_HOURS_D   0x02
#define IDC_MINUTES   0x03
#define IDC_SECONDS   0x04

#define LEDS_PER_HOUR   (LED_COUNT / 12)
#define LEDS_PER_MINUTE (LED_COUNT / 60) 

class ClockDisplay {
    private:
    Adafruit_NeoPixel _strip;
    uint32_t _colors[5];

    public:
    ClockDisplay() : _strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800) {
       // resetColors();
    }

    // void resetColors() {
    //     _colors[IDC_TICKS]   = COLOR_TICKS;
    //     _colors[IDC_HOURS_N] = COLOR_HOURS_N;
    //     _colors[IDC_HOURS_D] = COLOR_HOURS_D;
    //     _colors[IDC_MINUTES] = COLOR_MINUTES;
    //     _colors[IDC_SECONDS] = COLOR_SECONDS; 
    // }

    void initialize(uint8_t brightness, uint32_t* colors) {
        setBrightnessAndColorScheme(brightness, colors);
        _strip.begin();
        _strip.setBrightness(LED_BRIGHTNESS); 
    }

    void clear() {
        _strip.clear();
    }

    uint8_t getBrightness() {
        return _strip.getBrightness();
    }
    
    void setBrightness(uint8_t brightness) {
        _strip.setBrightness(brightness);
    }

    bool setBrightnessAndColorScheme(String brightnessStr, String colorsStr) {
        int brightness = brightnessStr.toInt();
        if (brightness < 0x00 || brightness > 0xff) {
            return false;
        }

        const char * str = colorsStr.c_str();
        const int len = sizeof(_colors) / sizeof(uint32_t);
        uint32_t colors[len];

        if (colorsStr.length() != len * 6) {
            return false;
        }

        for(int i = 0; i < len; i++) {
            if (sscanf(str + i * 6, "%6x", &colors[i]) == 0) {
                return false;
            }
        }

        setBrightnessAndColorScheme(brightness, colors);
        return true;
    }


    void setBrightnessAndColorScheme(uint8_t brightness, uint32_t* colors) {
        _strip.setBrightness(brightness);
        const int len = sizeof(_colors) / sizeof(uint32_t);
        for (int i = 0; i < len; i++) {
            _colors[i] = colors[i];
        }
    }

    void copyBrightnessAndColorScheme(uint8_t* brightness, uint32_t* colors) {
        brightness[0] = _strip.getBrightness();
        const int len = sizeof(_colors) / sizeof(uint32_t);
        for (int i = 0; i < len; i++) {
            colors[i] = _colors[i];
        }
    }

    String getColorScheme() {
        int len = sizeof(_colors) / sizeof(uint32_t);
        char buf[len * 6 + 1];

        for(int n = 0; n < len; n++) {
            sprintf(buf + n * 6, "%06x", _colors[n]);
        }

        return String(buf);
    }

    void stripUpdate(unsigned long time) {
        uint8_t ss = time % 60;
        uint8_t hh = time / 3600;
        uint8_t mm = time / 60 - hh * 60; 

        for(uint8_t i = 0; i < LED_COUNT; i++) {
            if (_colors[IDC_SECONDS] && (ss == i / LEDS_PER_MINUTE)) {
                _strip.setPixelColor(i, _colors[IDC_SECONDS]);
            }
            else if (mm == i / LEDS_PER_MINUTE) {
                _strip.setPixelColor(i, _colors[IDC_MINUTES]);
            }
            else if (i % LEDS_PER_HOUR) {
                if ((hh % 12) == (i / LEDS_PER_HOUR)) {
                    _strip.setPixelColor(i, (hh < 6 || hh > 17)
                        ? _colors[IDC_HOURS_N]
                        : _colors[IDC_HOURS_D]);
                }
                else _strip.setPixelColor(i, 0x000000);
            }
            else {
                _strip.setPixelColor(i, _colors[IDC_TICKS]);
            }
        }
        _strip.show();
    }
};