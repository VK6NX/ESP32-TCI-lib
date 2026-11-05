// SPDX-License-Identifier: GPL-3.0-or-later
//
// ESP32 adaptation of TCI (Transceiver Control Interface).
// Developed on the basis of TCI, owned by Expert Electronics.
// TCI protocol specification: Copyright (c) 2021 Expert Electronics, MIT License
//   https://github.com/ExpertSDR3/TCI  (see https://github.com/ExpertSDR3/TCI/blob/main/LICENSE)
//
// ESP32 adaptation: Copyright (c) 2026 VK6NX, GPL-3.0-or-later
//   (see licenses/GPL-3.0.txt)

/**
 * @file main.cpp
 * @brief Minimal ESP32 example of the TCI client library
 */

#include <Arduino.h>
#include <WiFi.h>
#include "tciclient.h"

// WiFi credentials
static const char* WIFI_SSID     = "your-ssid";
static const char* WIFI_PASSWORD = "your-password";

// TCI server address
static const char* TCI_HOST = "192.168.1.100";
static const uint16_t TCI_PORT = 40001;

TCIClient tciClient(TCI_HOST, TCI_PORT);

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());

    // Connection status changes
    tciClient.setStatusChangeCallback([](TCIClient::Status newStatus) {
        const char* s = (newStatus == TCIClient::Status::Connected)  ? "Connected"
                      : (newStatus == TCIClient::Status::Connecting) ? "Connecting"
                      :                                                "Disconnected";
        Serial.printf("TCI status: %s\n", s);
    });

    // PROTOCOL:arg1,arg2; -> program name, protocol version
    tciClient.setProtocolCallback([](const String& name, const String& version) {
        Serial.printf("PROTOCOL: program %s, version %s\n", name.c_str(), version.c_str());
    });

    // DEVICE:arg1; -> device name
    tciClient.setDeviceCallback([](const String& device) {
        Serial.printf("DEVICE: %s\n", device.c_str());
    });

    // TRX_COUNT:arg1; -> number of receivers/transceivers
    tciClient.setTrxCountCallback([](uint32_t count) {
        Serial.printf("TRX_COUNT: %u\n", (unsigned)count);
    });

    // CHANNELS_COUNT:arg1; -> number of receive channels
    tciClient.setChannelsCountCallback([](uint32_t count) {
        Serial.printf("CHANNELS_COUNT: %u\n", (unsigned)count);
    });

    // VFO_LIMITS:arg1,arg2; -> lower and upper frequency limits (Hz)
    tciClient.setVfoLimitsCallback([](double low, double high) {
        Serial.printf("VFO_LIMITS: %.0f - %.0f Hz\n", low, high);
    });

    // MODULATIONS_LIST:arg1,...,argN; -> supported modulations
    tciClient.setModulationsListCallback([](const std::vector<String>& modulations) {
        Serial.print("MODULATIONS_LIST:");
        for (size_t i = 0; i < modulations.size(); ++i) {
            Serial.printf("%s %s", (i == 0) ? "" : ",", modulations[i].c_str());
        }
        Serial.println();
    });

    // VFO:arg1,arg2,arg3; -> receiver, channel, frequency (Hz)
    tciClient.setVfoChangedCallback([](uint32_t receiver, uint32_t channel, double frequency) {
        Serial.printf("VFO: receiver %u, channel %u, frequency %.0f Hz\n",
                      (unsigned)receiver, (unsigned)channel, frequency);
    });

    tciClient.connect();

    Serial.println("Keys: s START, x STOP, r VFO read, v VFO set, t TUNE_DRIVE, c CW_MACROS");
}

void loop() {
    tciClient.loop();

    // Standard commands, sent by keys from the serial monitor when Connected
    if (Serial.available() > 0) {
        char key = Serial.read();
        if (tciClient.isConnected()) {
            switch (key) {
                case 's':
                    Serial.println("TX: START;");
                    tciClient.sendStart();
                    break;
                case 'x':
                    Serial.println("TX: STOP;");
                    tciClient.sendStop();
                    break;
                case 'r':
                    Serial.println("TX: VFO:0,0;");
                    tciClient.sendVfoRead(0, 0);
                    break;
                case 'v':
                    Serial.println("TX: VFO:0,0,7100000;");
                    tciClient.sendVfoSet(0, 0, 7100000);
                    break;
                case 't':
                    Serial.println("TX: TUNE_DRIVE:0,30;");
                    tciClient.sendTuneDrive(0, 30);
                    break;
                case 'c':
                    // Transmits on air
                    Serial.println("TX: CW_MACROS:0,TEST;");
                    tciClient.sendCwMacros(0, "TEST");
                    break;
                default:
                    break;
            }
        }
    }
}
