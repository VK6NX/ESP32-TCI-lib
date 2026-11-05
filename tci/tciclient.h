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
 * @file tciclient.h
 * @brief TCI (Transceiver Control Interface) WebSocket client
 * 
 * TCI Protocol: https://github.com/ExpertSDR3/TCI
 * 
 */

#ifndef TCICLIENT_H
#define TCICLIENT_H

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <functional>
#include "tciparser.h"

/**
 * @brief TCI WebSocket client manager
 */
class TCIClient {
public:
    /**
     * @brief TCI connection status states
     */
    enum class Status {
        Disconnected,  // Not connected
        Connecting,    // Connection attempt in progress
        Connected      // Successfully connected and operational
    };
    
    /**
     * @brief Callback type for status change notifications
     */
    typedef std::function<void(Status newStatus)> StatusChangeCallback;

    /**
     * @brief Callback types for parsed TCI commands
     */
    using VfoChangedCallback = std::function<void(uint32_t, uint32_t, double)>;

    TCIClient(const char* host, uint16_t port);
    
    void begin();
    
    void connect();
    
    void disconnect();
    
    void loop();
    
    bool isConnected();
    
    Status getStatus() const { return status; }
    
    void sendCommand(const char* command);
    
    void setAddress(const String& address);

    void updateConnection(const char* newHost, uint16_t newPort);
    
    String getHost() const { return tciHost; }
    
    uint16_t getPort() const { return tciPort; }

    String getAddress() const { return tciAddress; }
    
    String getConnectionStatus() const;
    
    /**
     * @brief Set callback for status change notifications
     * @param callback Function to call when status changes
     */
    void setStatusChangeCallback(StatusChangeCallback callback);

    void setVfoChangedCallback(VfoChangedCallback callback);

private:
    WebSocketsClient wsClient;
    String tciAddress;
    String tciHost;
    uint16_t tciPort;
    Status status;
    unsigned long lastStatusPrint;
    StatusChangeCallback statusChangeCallback;
    VfoChangedCallback vfoChangedCallback;
    bool configured;
    bool connectionActive;
    bool pendingUpdate;
    String pendingHost;
    uint16_t pendingPort;
    TciMessageParser parser;
    
    /**
     * @brief Change status and notify callback if registered
     */
    void changeStatus(Status newStatus);

    void configureWebSocket();

    void startConnection();

    void applyPendingUpdate();

    static String buildAddress(const char* host, uint16_t port);

    static bool parseAddress(const String& address, String& hostOut, uint16_t& portOut);
    
    static void webSocketEventStatic(WStype_t type, uint8_t* payload, size_t length);
    
    void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    
    void parseMessage(const char* message, size_t length);
    
    void onReady();
    
    void onDevice(const char* deviceName);
    
    void onProtocol(const char* protocol);

    void onVfoChanged(uint32_t arg1, uint32_t arg2, double arg3);

    // Static instance pointer for callback access
    static TCIClient* instance;
};

#endif // TCICLIENT_H
