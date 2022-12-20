#include <Arduino.h>
#include <time.h>
#include <sntp.h>

#pragma once

//#define CONFIG_LWIP_SNTP_UPDATE_DELAY SecondsBetweenUpdate

extern int settimeofday(const struct timeval* tv, const struct timezone* tz);

class NtpHelper {
    public:
    
    static bool isNTPEnabled() {
        return sntp_enabled();
    }

    static void enableNTP(bool enable) {
        if (enable != sntp_enabled()) {
            if (enable) {
                sntp_init();
                Serial.println("sntp_init()");
            }
            else {
                sntp_stop();
                Serial.println("sntp_stop()");
            }
        }
    }

    static void initializeNTP(uint8_t timezone, uint8_t daylight, // TZ_Europe_Moscow (TZ.h)
        const char* timeServer1, const char* timeServer2, const char* timeServer3, bool enable = true) {
        configTime(timezone * 3600, daylight * 3600, timeServer1, timeServer2, timeServer3);
        enableNTP(enable);
    } 
};

// bool set_datetime(time_t time) {
//     struct timeval tv = { time, 0 };
//     return settimeofday(&tv, nullptr) == 0;
// }

// // Convert YYYYMMDDTHHMMSSZ string to time, returns -1 on fail
// time_t parse_datetime(const char* input) {
//     if (strlen(input) == 16 && input[8] == 'T' && input[15] == 'Z') {
//         struct tm tmm = {0};
//         sscanf(input, "%4d%2d%2dT%2d%2d%2dZ",
//             &tmm.tm_year, &tmm.tm_mon, &tmm.tm_mday,
//             &tmm.tm_hour, &tmm.tm_min, &tmm.tm_sec);
//         tmm.tm_year -= 1900;
//         tmm.tm_mon -= 1;
//         return mktime(&tmm);
//     }
//     return -1;
// }

// const char* timetostring(time_t time) {
//     static char buff[17];
//     struct tm * tmm = localtime(&time);
//     strftime(buff, 17, "%Y%m%dT%H%M%SZ", tmm);
//     return buff;
// }

#define NOT_A_TIME -1

class Time {
    time_t _ms;

    public:
    Time(const time_t &milliseconds = 0) {
        _ms = milliseconds;
    }

    Time(Time &time) {
        _ms = time._ms;
    }

    Time (int year, int month, int day, int hour, int minute, int second) {
        struct tm t = {0};
        t.tm_year = year - 1900; t.tm_mon = month - 1; t.tm_mday = day;
        t.tm_hour = hour; t.tm_min = minute; t.tm_sec = second;
        _ms = mktime(&t);
    }

    // Create time from ISO string "YYYYMMDDTHHMMSSZ"
    Time (const char* input) {
        if (strlen(input) >= 16 && input[8] == 'T' && input[15] == 'Z') {
            struct tm tmm = {0};
            sscanf(input, "%4d%2d%2dT%2d%2d%2dZ",
                &tmm.tm_year, &tmm.tm_mon, &tmm.tm_mday,
                &tmm.tm_hour, &tmm.tm_min, &tmm.tm_sec);
            tmm.tm_year -= 1900;
            tmm.tm_mon -= 1;
            _ms = mktime(&tmm);
        }
        else _ms = NOT_A_TIME;
    }

    const time_t& milliseconds() const {
        return _ms;
    }

    tm* toDetails() {
        return localtime(&_ms);
    }

    // Return time ISO string "YYYYMMDDTHHMMSSZ"
    String toString() const {
        struct tm *t = localtime(&_ms);
        char v[17];
        strftime(v, sizeof(v), "%Y%m%dT%H%M%SZ", t);
        return String(v);
    }

    bool setSystemTime() {
        struct timeval tv = { _ms, 0 };
        return settimeofday(&tv, NULL) == 0;
    }

    static Time now() {
        return Time(time(NULL));
    }
};