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
 * @file tciparser.h
 */

#ifndef TCI_PARSER_H
#define TCI_PARSER_H

#include <Arduino.h>
#include <functional>
#include <vector>

class TciMessageParser {
public:
    using VfoHandler = std::function<void(uint32_t, uint32_t, double)>;
    using ReadyHandler = std::function<void()>;
    using ProtocolHandler = std::function<void(const String&, const String&)>;
    using DeviceHandler = std::function<void(const String&)>;
    using TrxCountHandler = std::function<void(uint32_t)>;
    using ChannelsCountHandler = std::function<void(uint32_t)>;
    using VfoLimitsHandler = std::function<void(double, double)>;
    using ModulationsListHandler = std::function<void(const std::vector<String>&)>;

    void setVfoHandler(VfoHandler handler) { vfoHandler = handler; }
    void setReadyHandler(ReadyHandler handler) { readyHandler = handler; }
    void setProtocolHandler(ProtocolHandler handler) { protocolHandler = handler; }
    void setDeviceHandler(DeviceHandler handler) { deviceHandler = handler; }
    void setTrxCountHandler(TrxCountHandler handler) { trxCountHandler = handler; }
    void setChannelsCountHandler(ChannelsCountHandler handler) { channelsCountHandler = handler; }
    void setVfoLimitsHandler(VfoLimitsHandler handler) { vfoLimitsHandler = handler; }
    void setModulationsListHandler(ModulationsListHandler handler) { modulationsListHandler = handler; }

    void process(const char* message, size_t length);

private:
    VfoHandler vfoHandler;
    ReadyHandler readyHandler;
    ProtocolHandler protocolHandler;
    DeviceHandler deviceHandler;
    TrxCountHandler trxCountHandler;
    ChannelsCountHandler channelsCountHandler;
    VfoLimitsHandler vfoLimitsHandler;
    ModulationsListHandler modulationsListHandler;

    static bool equalsIgnoreCase(const char* a, size_t aLen, const char* b);
    static void trimToken(const char*& start, size_t& len);
    static bool parseUint32(const char* start, size_t len, uint32_t& out);
    static bool parseDouble(const char* start, size_t len, double& out);
    static bool extractArgs(const char* start, const char* end, const char* (&argStarts)[3], size_t (&argLens)[3], size_t& argCount);
    static bool extractList(const char* start, const char* end, std::vector<String>& out);
};

#endif // TCI_PARSER_H
