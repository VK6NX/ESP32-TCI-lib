// SPDX-License-Identifier: GPL-3.0-or-later
//
// ESP32 adaptation of TCI (Transceiver Control Interface).
// Developed on the basis of TCI, owned by Expert Electronics.
// TCI protocol specification: Copyright (c) 2021 Expert Electronics, MIT License
//   https://github.com/ExpertSDR3/TCI  (see licenses/TCI-LICENSE.txt)
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

    // VFO:arg1,arg2,arg3; -> receiver, channel, frequency (Hz)
    tciClient.setVfoChangedCallback([](uint32_t receiver, uint32_t channel, double frequency) {
        Serial.printf("VFO: receiver %u, channel %u, frequency %.0f Hz\n",
                      (unsigned)receiver, (unsigned)channel, frequency);
    });

    tciClient.connect();
}

void loop() {
    tciClient.loop();
}
