#pragma once

#include "Arduino.h"
#include <Udp.h>

#define SEVENZYYEARS 2208988800UL
#define NTP_PACKET_SIZE 48
#define NTP_DEFAULT_LOCAL_PORT 1337

class Ntp {
    private:
    UDP*          _udp;
    bool          _udpSetup       = false;

    const char*   _poolServerName = "pool.ntp.org";
    IPAddress     _poolServerIP;
    unsigned int  _port           = NTP_DEFAULT_LOCAL_PORT;
    long          _timeOffset     = 0;
    byte          _packetBuffer[NTP_PACKET_SIZE];

    unsigned long _timeOrigin = 0;      // Last known time, in s
    unsigned long _lastUpdate = 0;      // Time since last update, in ms

    public:
    Ntp(UDP& udp, long timeOffset) {
        _udp = &udp;
        _timeOffset = timeOffset;
    }

    bool update() {
        if (!_udpSetup) {
            _udp->begin(this->_port);
            _udpSetup = true;
        }

        while(_udp->parsePacket()) {
            _udp->flush();
        }

        memset(_packetBuffer, 0, NTP_PACKET_SIZE);
        _packetBuffer[0] = 0b11100011;
        _packetBuffer[1] = 0;
        _packetBuffer[2] = 6;
        _packetBuffer[3] = 0xEC;
        _packetBuffer[12]  = 49;
        _packetBuffer[13]  = 0x4E;
        _packetBuffer[14]  = 49;
        _packetBuffer[15]  = 52;

        _udp->beginPacket(_poolServerName, 123);
        _udp->write(_packetBuffer, NTP_PACKET_SIZE);
        _udp->endPacket();

        uint8_t timeout = 0;
        while (true) {
            if (timeout++ > 100) {
                return false;
            }
            delay(10);
            if (_udp->parsePacket() == NTP_PACKET_SIZE) {
                break;
            }
        }

        _lastUpdate = millis() - (10 * timeout);
        _udp->read(_packetBuffer, NTP_PACKET_SIZE);

        unsigned long hw = word(_packetBuffer[40], _packetBuffer[41]);
        unsigned long lw = word(_packetBuffer[42], _packetBuffer[43]);
        unsigned long secsSince1900 = hw << 16 | lw;

        _timeOrigin = secsSince1900 - SEVENZYYEARS;
        return true;
    }

    bool isSyncronized() const {
        return _lastUpdate > 0;
    }

    unsigned long getTime() const {
        return _timeOffset + _timeOrigin +  (millis() - _lastUpdate) / 1000;
    }
};