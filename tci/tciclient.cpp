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
 * @file tciclient.cpp
 */

#include "tciclient.h"

// Static instance pointer for callback
TCIClient* TCIClient::instance = nullptr;

TCIClient::TCIClient(const char* host, uint16_t port)
    : tciAddress(buildAddress(host, port)), tciHost(host), tciPort(port), status(Status::Disconnected),
            lastStatusPrint(0), statusChangeCallback(nullptr), configured(false), connectionActive(false),
            pendingUpdate(false), pendingHost(""), pendingPort(0) {
    instance = this;

    parser.setVfoHandler([this](uint32_t arg1, uint32_t arg2, double arg3) {
        onVfoChanged(arg1, arg2, arg3);
    });
    parser.setReadyHandler([this]() {
        onReady();
    });
}

void TCIClient::begin() {
    startConnection();
}

void TCIClient::connect() {
    startConnection();
}

void TCIClient::disconnect() {
    if (status != Status::Disconnected) {
        wsClient.disconnect();
        connectionActive = false;
        changeStatus(Status::Disconnected);
    }
}

void TCIClient::loop() {
    if (pendingUpdate) {
        applyPendingUpdate();
        return;
    }

    // Process WebSocket events
    wsClient.loop();

    // Periodic connection status (every 10 seconds if not connected)
    if (connectionActive && status != Status::Connected && (millis() - lastStatusPrint >= 10000)) {
        lastStatusPrint = millis();
    }
}

bool TCIClient::isConnected() {
    return status == Status::Connected;
}

void TCIClient::sendCommand(const char* command) {
    if (status == Status::Connected) {
        wsClient.sendTXT(command);
    } else {
    }
}

void TCIClient::setAddress(const String& address) {
    tciAddress = address;
    if (!parseAddress(tciAddress, tciHost, tciPort)) {
    }
}

void TCIClient::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            changeStatus(Status::Connecting);  // Auto-reconnect, so move to Connecting
            break;

        case WStype_CONNECTED:
            changeStatus(Status::Connected);
            break;

        case WStype_TEXT:
            // Received text message from TCI server
            // Serial.printf("=== TCI Client: RX (%u bytes)\n", length);
            parseMessage(reinterpret_cast<const char*>(payload), length);
            break;

        case WStype_BIN:
            break;

        case WStype_ERROR:
            break;

        case WStype_PING:
            // Pings are handled automatically
            break;

        case WStype_PONG:
            // Pongs are handled automatically
            break;

        default:
            break;
    }
}

void TCIClient::parseMessage(const char* message, size_t length) {
    parser.process(message, length);
}

void TCIClient::onReady() {
    // When server is ready, you can send initialization commands
}

void TCIClient::onDevice(const char* deviceName) {
}

void TCIClient::onProtocol(const char* protocol) {
}

void TCIClient::onVfoChanged(uint32_t arg1, uint32_t arg2, double arg3) {
    if (vfoChangedCallback) {
        vfoChangedCallback(arg1, arg2, arg3);
    }
}

void TCIClient::updateConnection(const char* newHost, uint16_t newPort) {
    pendingHost = String(newHost);
    pendingPort = newPort;
    pendingUpdate = true;
}

String TCIClient::getConnectionStatus() const {
    switch (status) {
        case Status::Connected:
            return "Connected";
        case Status::Connecting:
            return "Connecting";
        case Status::Disconnected:
            return "Disconnected";
        default:
            return "Unknown";
    }
}

void TCIClient::setStatusChangeCallback(StatusChangeCallback callback) {
    statusChangeCallback = callback;
}

void TCIClient::setVfoChangedCallback(VfoChangedCallback callback) {
    vfoChangedCallback = callback;
}

void TCIClient::changeStatus(Status newStatus) {
    if (status != newStatus) {
        Status oldStatus = status;
        status = newStatus;

        // Notify callback if registered
        if (statusChangeCallback) {
            statusChangeCallback(newStatus);
        }
    }
}

void TCIClient::configureWebSocket() {
    if (configured) {
        return;
    }

    // Set event handler using lambda to capture 'this'
    wsClient.onEvent([](WStype_t type, uint8_t* payload, size_t length) {
        if (TCIClient::instance != nullptr) {
            TCIClient::instance->webSocketEvent(type, payload, length);
        }
    });

    // Enable automatic reconnection every 5 seconds
    wsClient.setReconnectInterval(5000);
    configured = true;
}

void TCIClient::startConnection() {
    configureWebSocket();

    if (!parseAddress(tciAddress, tciHost, tciPort)) {
        changeStatus(Status::Disconnected);
        return;
    }

    changeStatus(Status::Connecting);

    wsClient.begin(tciHost.c_str(), tciPort, "/");
    connectionActive = true;
    lastStatusPrint = 0;
}

void TCIClient::applyPendingUpdate() {
    pendingUpdate = false;

    connectionActive = false;
    if (status != Status::Disconnected) {
        wsClient.disconnect();
        changeStatus(Status::Disconnected);
    }

    setAddress(buildAddress(pendingHost.c_str(), pendingPort));

    lastStatusPrint = 0;
    startConnection();
}

String TCIClient::buildAddress(const char* host, uint16_t port) {
    String address = "ws://";
    address += host;
    address += ":";
    address += String(port);
    return address;
}

bool TCIClient::parseAddress(const String& address, String& hostOut, uint16_t& portOut) {
    String addr = address;
    addr.trim();

    if (addr.startsWith("ws://")) {
        addr = addr.substring(5);
    }

    int colonIndex = addr.lastIndexOf(':');
    if (colonIndex <= 0 || colonIndex == addr.length() - 1) {
        return false;
    }

    String host = addr.substring(0, colonIndex);
    String portStr = addr.substring(colonIndex + 1);
    int port = portStr.toInt();

    if (host.length() == 0 || port <= 0 || port > 65535) {
        return false;
    }

    hostOut = host;
    portOut = static_cast<uint16_t>(port);
    return true;
}
