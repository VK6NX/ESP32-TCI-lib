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
    parser.setProtocolHandler([this](const String& name, const String& version) {
        onProtocol(name, version);
    });
    parser.setDeviceHandler([this](const String& device) {
        onDevice(device);
    });
    parser.setTrxCountHandler([this](uint32_t count) {
        onTrxCount(count);
    });
    parser.setChannelsCountHandler([this](uint32_t count) {
        onChannelsCount(count);
    });
    parser.setVfoLimitsHandler([this](double low, double high) {
        onVfoLimits(low, high);
    });
    parser.setModulationsListHandler([this](const std::vector<String>& modulations) {
        onModulationsList(modulations);
    });
}

void TCIClient::begin() {
    startConnection();
}

void TCIClient::connect() {
    startConnection();
}

void TCIClient::disconnect() {
    connectionActive = false;
    if (status != Status::Disconnected) {
        wsClient.disconnect();
        changeStatus(Status::Disconnected);
    }
}

void TCIClient::loop() {
    if (pendingUpdate) {
        applyPendingUpdate();
        return;
    }

    // Process WebSocket events
    if (connectionActive) {
        wsClient.loop();
    }

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

void TCIClient::sendStart() {
    sendCommand("START;");
}

void TCIClient::sendStop() {
    sendCommand("STOP;");
}

void TCIClient::sendVfoRead(uint32_t receiver, uint32_t channel) {
    char command[32];
    snprintf(command, sizeof(command), "VFO:%u,%u;", (unsigned)receiver, (unsigned)channel);
    sendCommand(command);
}

void TCIClient::sendVfoSet(uint32_t receiver, uint32_t channel, double frequency) {
    char command[64];
    snprintf(command, sizeof(command), "VFO:%u,%u,%.0f;", (unsigned)receiver, (unsigned)channel, frequency);
    sendCommand(command);
}

void TCIClient::sendTuneDrive(uint32_t transceiver, uint32_t power) {
    char command[48];
    snprintf(command, sizeof(command), "TUNE_DRIVE:%u,%u;", (unsigned)transceiver, (unsigned)power);
    sendCommand(command);
}

void TCIClient::sendCwMacros(uint32_t transceiver, const char* text) {
    String command = "CW_MACROS:";
    command += String(transceiver);
    command += ",";

    // Reserved characters in the text are replaced as the TCI specification requires
    for (const char* p = text; *p != '\0'; ++p) {
        switch (*p) {
            case ':':
                command += '^';
                break;
            case ',':
                command += '~';
                break;
            case ';':
                command += '*';
                break;
            default:
                command += *p;
                break;
        }
    }

    command += ";";
    sendCommand(command.c_str());
}

void TCIClient::setAddress(const String& address) {
    tciAddress = address;
    if (!parseAddress(tciAddress, tciHost, tciPort)) {
    }
}

void TCIClient::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            if (connectionActive) {
                changeStatus(Status::Connecting);  // Auto-reconnect, so move to Connecting
            } else {
                changeStatus(Status::Disconnected);
            }
            break;

        case WStype_CONNECTED:
            // Status stays Connecting until READY
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
    changeStatus(Status::Connected);
}


void TCIClient::onVfoChanged(uint32_t arg1, uint32_t arg2, double arg3) {
    if (vfoChangedCallback) {
        vfoChangedCallback(arg1, arg2, arg3);
    }
}

void TCIClient::onProtocol(const String& name, const String& version) {
    if (protocolCallback) {
        protocolCallback(name, version);
    }
}

void TCIClient::onDevice(const String& device) {
    if (deviceCallback) {
        deviceCallback(device);
    }
}

void TCIClient::onTrxCount(uint32_t count) {
    if (trxCountCallback) {
        trxCountCallback(count);
    }
}

void TCIClient::onChannelsCount(uint32_t count) {
    if (channelsCountCallback) {
        channelsCountCallback(count);
    }
}

void TCIClient::onVfoLimits(double low, double high) {
    if (vfoLimitsCallback) {
        vfoLimitsCallback(low, high);
    }
}

void TCIClient::onModulationsList(const std::vector<String>& modulations) {
    if (modulationsListCallback) {
        modulationsListCallback(modulations);
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

void TCIClient::setProtocolCallback(ProtocolCallback callback) {
    protocolCallback = callback;
}

void TCIClient::setDeviceCallback(DeviceCallback callback) {
    deviceCallback = callback;
}

void TCIClient::setTrxCountCallback(TrxCountCallback callback) {
    trxCountCallback = callback;
}

void TCIClient::setChannelsCountCallback(ChannelsCountCallback callback) {
    channelsCountCallback = callback;
}

void TCIClient::setVfoLimitsCallback(VfoLimitsCallback callback) {
    vfoLimitsCallback = callback;
}

void TCIClient::setModulationsListCallback(ModulationsListCallback callback) {
    modulationsListCallback = callback;
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
    wsClient.enableHeartbeat(15000, 3000, 2);
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
