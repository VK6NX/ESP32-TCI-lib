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
 * @file tciparser.h
 */

#ifndef TCI_PARSER_H
#define TCI_PARSER_H

#include <Arduino.h>
#include <functional>

class TciMessageParser {
public:
    using VfoHandler = std::function<void(uint32_t, uint32_t, double)>;
    using ReadyHandler = std::function<void()>;

    void setVfoHandler(VfoHandler handler) { vfoHandler = handler; }
    void setReadyHandler(ReadyHandler handler) { readyHandler = handler; }

    void process(const char* message, size_t length);

private:
    VfoHandler vfoHandler;
    ReadyHandler readyHandler;

    static bool equalsIgnoreCase(const char* a, size_t aLen, const char* b);
    static void trimToken(const char*& start, size_t& len);
    static bool parseUint32(const char* start, size_t len, uint32_t& out);
    static bool parseDouble(const char* start, size_t len, double& out);
    static bool extractArgs(const char* start, const char* end, const char* (&argStarts)[3], size_t (&argLens)[3], size_t& argCount);
};

#endif // TCI_PARSER_H
