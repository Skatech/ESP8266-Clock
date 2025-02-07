#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266NetBIOS.h>
#include <FS.h>
#include <LittleFS.h>
#include <time.h>

#include "configuration.h"
#include "mytime.h"
#include "display.h"

Configuration state;
ClockDisplay display;
wl_status_t wl_status = WL_IDLE_STATUS;
ESP8266WebServer server(80);

inline bool net_status_good(wl_status_t status) {
    return status == WL_CONNECTED || status == WL_DISCONNECTED;
}

bool net_initialize() {
    return WiFi.disconnect() && WiFi.mode(WIFI_STA) && WiFi.hostname(HOST_NAME) &&
        WiFi.config(IPAddress(state.stationIP), IPAddress(state.stationGateway),
            IPAddress(state.stationSubnet), IPAddress(state.stationDNS)) &&
        net_status_good(WiFi.begin(WIFI_SSID, WIFI_PASSWORD));
}

void initializeNTP() {
    NtpHelper::initializeNTP(state.timezone, state.daylight,
        state.timeServer1, state.timeServer2, state.timeServer3, state.ntpenabled);
}

void setup() {
    //pinMode(LED_BUILTIN, OUTPUT);
    //digitalWrite(LED_BUILTIN, 0x01);

    Serial.begin(UART_SPEED);
    Serial.println("\n\nESP8266 Hall Clock\n=============================\n");

    state.loadStoredConfigurationOrDefaults();

    // Serial.print("Time Server 1: ");
    // Serial.println(state.timeServer1);
    // Serial.print("Time Server 2: ");
    // Serial.println(state.timeServer2);
    // Serial.print("Time Server 3: ");
    // Serial.println(state.timeServer3);


    initializeNTP();

    display.initialize(state.displayBrightness, state.displayColors);

    Serial.print("Initializing network: ");
    Serial.println(net_initialize() ? "OK" : "FAILED");
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("IP Address:  ");
    Serial.println(IPAddress(state.stationIP));
    Serial.print("Gateway:     ");
    Serial.println(IPAddress(state.stationGateway));
    Serial.print("DNS Address: ");
    Serial.println(IPAddress(state.stationDNS));
    Serial.print("Subnet Mask: ");
    Serial.println(IPAddress(state.stationSubnet));
    Serial.print("SSID Name:   ");
    Serial.println(state.wifiSSID);
    
    Serial.print("NETBios:     ");
    Serial.println(NBNS.begin("hallclock") ? "OK (hallclock)" : "FAILED");
    
    Serial.print("Initializing filesystem: ");
    Serial.println(LittleFS.begin() ? "OK" : "FAILED");

    server.on("/time", HTTP_GET, []() {
        server.send(200, "text/html", Time::now().toString());
    });

    server.on("/status", HTTP_GET, []() {
        String json("{\"date\":\"[DATE]\", \"timezone\":[ZONE], \"daylight\":[DAYL], \"ntpenabled\":[NTPE], \"ntpserver1\":\"[NTP1]\", \"ntpserver2\":\"[NTP2]\", \"ntpserver3\":\"[NTP3]\", \"brightness\":[BRIG], \"colors\":\"[CLRS]\"}");
        json.replace("[DATE]", Time::now().toString());
        json.replace("[ZONE]", "3");
        json.replace("[DAYL]", "0");
        json.replace("[NTPE]", "true");
        json.replace("[NTP1]", "0.pool.ntp.org");
        json.replace("[NTP2]", "1.pool.ntp.org");
        json.replace("[NTP3]", "time.nist.gov");
        json.replace("[BRIG]", String(display.getBrightness()));
        json.replace("[CLRS]", display.getColorScheme());

        server.send(200, "application/json", json);
        Serial.println("Processed GET(/status)");
    });

    server.on("/set-date", HTTP_POST, []() {
        String date = server.arg("date");
        //time_t tt = parse_datetime(date.c_str());
        //bool succ = (tt >= 0) && set_datetime(tt);
        bool succ = Time(date.c_str()).setSystemTime();
        String msg = String(succ ? "Set new date: " : "Date set failed: ") + date;
        server.send(succ ? 200 : 400, "text/html", msg);
        Serial.println(msg);
    });

    server.on("/syncronize", HTTP_POST, []() {
        initializeNTP();
        server.send(200, "text/html", "SNTP restarted");
        Serial.println("SNTP restarted");
    });

    server.on("/set-display", HTTP_POST, []() {
        if (display.setBrightnessAndColorScheme(
            server.arg("brightness"), server.arg("colors"))) {

            display.copyBrightnessAndColorScheme(
                &state.displayBrightness, state.displayColors);
        
            const char* msg = "Display scheme updated";
            server.send(200, "text/html", msg);
            Serial.println(msg);
        }
        else {
            const char* msg = "Display scheme update failed";
            server.send(400, "text/html", msg);
            Serial.println(msg);
        }
    });

    server.on("/set-conn", HTTP_POST, []() {
        String ssid = server.arg("ssid");
        String pass = server.arg("pass");
        ssid.toCharArray(state.wifiSSID, 16);
        pass.toCharArray(state.wifiPassword, 16);
        server.send(true ? 200 : 400, "text/html", "OK");
        Serial.println("Connection data changed");
    });

    server.on("/write-config", HTTP_POST, []() {
        bool succ = state.saveToEEPROM();
        String msg = String("Configuration saved to EEPROM: ") + succ ? "OK" : "FAIL";
        server.send(succ ? 200 : 400, "text/html", msg);
        Serial.println(msg);
    });

    server.on("/write-config", HTTP_GET, []() {
        bool succ = state.saveToEEPROM();
        String msg = String("Configuration saved to EEPROM: ") + succ ? "OK" : "FAIL";
        server.send(succ ? 200 : 400, "text/html", msg);
        Serial.println(msg);
    });

    server.on("/sync", HTTP_GET, []() {
        initializeNTP();
        server.send(200, "text/html", "SNTP restarted");
    });

    server.on("/test", HTTP_GET, []() {
        server.send(200, "text/plain", "OK");
        display.test();
    });

    server.serveStatic("/", LittleFS, "/", "no-cache"/*"max-age=3600"*/); //86400
    server.begin();

    display.test();
}

void loop() {
    time_t lct = time(NULL);
    if (lct < 1000000000LL) {
        if (lct % 2) {
            display.draw(WiFi.isConnected() ? 0x000044 : 0x440000, 0x0B0800);
        }
        else display.draw(0x000000, 0x0B0800);
    }
    else display.poll();

    if (WiFi.status() != wl_status) {
        wl_status = WiFi.status();
        if (wl_status == WL_CONNECTED) {
            Serial.print("Station connected, IP address: ");
            Serial.println(WiFi.localIP());
        }
        else if (wl_status == WL_DISCONNECTED) {
            Serial.println("Station disconnected");
        }
        else {
            Serial.print("Station connection state changed: ");
            Serial.println(wl_status);
        }
    }

    if (wl_status == WL_CONNECTED) {
        server.handleClient();
    }

    delay(1);
}