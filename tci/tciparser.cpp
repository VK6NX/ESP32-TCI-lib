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
 * @file tciparser.cpp
 */

#include "tciparser.h"
#include <stdlib.h>
#include <string.h>

void TciMessageParser::process(const char* message, size_t length) {
    if (message == nullptr || length == 0) {
        return;
    }

    const char* current = message;
    const char* end = message + length;

    while (current < end) {
        const char* segmentEnd = current;
        while (segmentEnd < end && *segmentEnd != ';') {
            ++segmentEnd;
        }

        if (segmentEnd > current && segmentEnd < end) {
            const char* colon = current;
            while (colon < segmentEnd && *colon != ':') {
                ++colon;
            }

            if (colon < segmentEnd) {
                const char* cmdStart = current;
                size_t cmdLen = static_cast<size_t>(colon - current);
                trimToken(cmdStart, cmdLen);

                const char* argsStart = colon + 1;
                const char* argsEnd = segmentEnd;

                const char* argStarts[3] = {nullptr, nullptr, nullptr};
                size_t argLens[3] = {0, 0, 0};
                size_t argCount = 0;

                if (extractArgs(argsStart, argsEnd, argStarts, argLens, argCount)) {
                    uint32_t a1 = 0;
                    uint32_t a2 = 0;
                    double a3d = 0.0;

                    // Customization example: VFOCHANGED is not part of the TCI specification.
                    // It is accepted here as a custom alias of VFO. Add your own commands the same way.
                    if (equalsIgnoreCase(cmdStart, cmdLen, "VFO") || equalsIgnoreCase(cmdStart, cmdLen, "VFOCHANGED")) {
                        if (argCount >= 3 && parseUint32(argStarts[0], argLens[0], a1)
                            && parseUint32(argStarts[1], argLens[1], a2)
                            && parseDouble(argStarts[2], argLens[2], a3d)
                            && memchr(argStarts[2], ',', argsEnd - argStarts[2]) == nullptr) {
                            if (vfoHandler) {
                                vfoHandler(a1, a2, a3d);
                            }
                        }
                    }
                    else if (equalsIgnoreCase(cmdStart, cmdLen, "PROTOCOL")) {
                        if (argCount == 2) {
                            if (protocolHandler) {
                                protocolHandler(String(argStarts[0], argLens[0]), String(argStarts[1], argLens[1]));
                            }
                        }
                    }
                    else if (equalsIgnoreCase(cmdStart, cmdLen, "DEVICE")) {
                        if (argCount == 1) {
                            if (deviceHandler) {
                                deviceHandler(String(argStarts[0], argLens[0]));
                            }
                        }
                    }
                    else if (equalsIgnoreCase(cmdStart, cmdLen, "TRX_COUNT")) {
                        if (argCount == 1 && parseUint32(argStarts[0], argLens[0], a1)) {
                            if (trxCountHandler) {
                                trxCountHandler(a1);
                            }
                        }
                    }
                    // CHANNEL_COUNT is the name of this command before TCI 2.1
                    else if (equalsIgnoreCase(cmdStart, cmdLen, "CHANNELS_COUNT") || equalsIgnoreCase(cmdStart, cmdLen, "CHANNEL_COUNT")) {
                        if (argCount == 1 && parseUint32(argStarts[0], argLens[0], a1)) {
                            if (channelsCountHandler) {
                                channelsCountHandler(a1);
                            }
                        }
                    }
                    else if (equalsIgnoreCase(cmdStart, cmdLen, "VFO_LIMITS")) {
                        double low = 0.0;
                        double high = 0.0;
                        if (argCount == 2 && parseDouble(argStarts[0], argLens[0], low)
                            && parseDouble(argStarts[1], argLens[1], high)) {
                            if (vfoLimitsHandler) {
                                vfoLimitsHandler(low, high);
                            }
                        }
                    }
                    else if (equalsIgnoreCase(cmdStart, cmdLen, "MODULATIONS_LIST")) {
                        std::vector<String> modulations;
                        if (extractList(argsStart, argsEnd, modulations)) {
                            if (modulationsListHandler) {
                                modulationsListHandler(modulations);
                            }
                        }
                    }
                }
            }
            else {
                const char* cmdStart = current;
                size_t cmdLen = static_cast<size_t>(segmentEnd - current);
                trimToken(cmdStart, cmdLen);

                if (equalsIgnoreCase(cmdStart, cmdLen, "READY")) {
                    if (readyHandler) {
                        readyHandler();
                    }
                }
            }
        }

        if (segmentEnd == end) {
            break;
        }
        current = segmentEnd + 1;
    }
}

bool TciMessageParser::equalsIgnoreCase(const char* a, size_t aLen, const char* b) {
    if (a == nullptr || b == nullptr) {
        return false;
    }

    size_t bLen = strlen(b);
    if (aLen != bLen) {
        return false;
    }

    for (size_t i = 0; i < aLen; ++i) {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'a' && ca <= 'z') {
            ca = static_cast<char>(ca - 'a' + 'A');
        }
        if (cb >= 'a' && cb <= 'z') {
            cb = static_cast<char>(cb - 'a' + 'A');
        }
        if (ca != cb) {
            return false;
        }
    }

    return true;
}

void TciMessageParser::trimToken(const char*& start, size_t& len) {
    while (len > 0 && (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')) {
        ++start;
        --len;
    }

    while (len > 0) {
        char c = start[len - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            --len;
        } else {
            break;
        }
    }
}

bool TciMessageParser::parseUint32(const char* start, size_t len, uint32_t& out) {
    if (start == nullptr || len == 0 || len >= 16) {
        return false;
    }

    char buffer[16];
    memcpy(buffer, start, len);
    buffer[len] = '\0';

    if (buffer[0] == '-') return false;

    char* endPtr = nullptr;
    unsigned long value = strtoul(buffer, &endPtr, 10);
    if (endPtr == buffer || *endPtr != '\0') {
        return false;
    }

    out = static_cast<uint32_t>(value);
    return true;
}

bool TciMessageParser::parseDouble(const char* start, size_t len, double& out) {
    if (start == nullptr || len == 0 || len >= 32) {
        return false;
    }

    char buffer[32];
    memcpy(buffer, start, len);
    buffer[len] = '\0';

    char* endPtr = nullptr;
    double value = strtod(buffer, &endPtr);
    if (endPtr == buffer || *endPtr != '\0') {
        return false;
    }

    out = value;
    return true;
}

bool TciMessageParser::extractArgs(const char* start, const char* end, const char* (&argStarts)[3], size_t (&argLens)[3], size_t& argCount) {
    argCount = 0;
    if (start == nullptr || end == nullptr || start >= end) {
        return false;
    }

    const char* current = start;

    while (current <= end && argCount < 3) {
        const char* tokenEnd = current;
        while (tokenEnd < end && *tokenEnd != ',') {
            ++tokenEnd;
        }

        // Arguments are not trimmed: a space can be a value, for example CW_MON: ;
        const char* tokenStart = current;
        size_t tokenLen = static_cast<size_t>(tokenEnd - current);

        if (tokenLen == 0) {
            return false;
        }

        argStarts[argCount] = tokenStart;
        argLens[argCount] = tokenLen;
        ++argCount;

        if (tokenEnd >= end) {
            break;
        }

        current = tokenEnd + 1;
    }

    return argCount > 0;
}

bool TciMessageParser::extractList(const char* start, const char* end, std::vector<String>& out) {
    out.clear();
    if (start == nullptr || end == nullptr || start >= end) {
        return false;
    }

    const char* current = start;

    while (current <= end) {
        const char* tokenEnd = current;
        while (tokenEnd < end && *tokenEnd != ',') {
            ++tokenEnd;
        }

        const char* tokenStart = current;
        size_t tokenLen = static_cast<size_t>(tokenEnd - current);

        if (tokenLen == 0) {
            return false;
        }

        out.push_back(String(tokenStart, tokenLen));

        if (tokenEnd >= end) {
            break;
        }

        current = tokenEnd + 1;
    }

    return !out.empty();
}
