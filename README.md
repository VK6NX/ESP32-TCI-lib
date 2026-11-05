<!--
SPDX-License-Identifier: GPL-3.0-or-later

ESP32 adaptation of TCI (Transceiver Control Interface).
Developed on the basis of TCI, owned by Expert Electronics.
TCI protocol specification: Copyright (c) 2021 Expert Electronics, MIT License
  https://github.com/ExpertSDR3/TCI  (see https://github.com/ExpertSDR3/TCI/blob/main/LICENSE)

ESP32 adaptation: Copyright (c) 2026 VK6NX, GPL-3.0-or-later
  (see licenses/GPL-3.0.txt)
-->

<p align="center">
  <b>English</b> | <a href="README.ru.md">Русский</a>
</p>

<h3 align="center">ESP32 TCI library</h3>

<div align="center">

  [![GitHub Issues](https://img.shields.io/github/issues/VK6NX/ESP32-TCI-lib.svg)](https://github.com/VK6NX/ESP32-TCI-lib/issues)
  [![GitHub Pull Requests](https://img.shields.io/github/issues-pr/VK6NX/ESP32-TCI-lib.svg)](https://github.com/VK6NX/ESP32-TCI-lib/pulls)
  [![License](https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg)](tci/licenses/GPL-3.0.txt)

</div>

---

<p align="center"> TCI v2.x (Transceiver Control Interface) WebSocket client for ESP32 (Arduino framework, PlatformIO).
    <br> 
</p>

## 📝 Table of Contents
- [About](#about)
- [Getting Started](#getting_started)
- [Usage](#usage)
- [Status and Support](#support)
- [Built Using](#built_using)
- [License](#license)
- [Authors](#authors)
- [Acknowledgments](#acknowledgement)

## 🧐 About <a name = "about"></a>
TCI (Transceiver Control Interface) is a network protocol by Expert Electronics for control, data transfer and synchronization between a transceiver and client software or devices. TCI runs over WebSocket: the transceiver software (for example ExpertSDR3) is the server, all other programs and devices are clients.

This project is an ESP32 adaptation developed on the basis of TCI, owned by Expert Electronics. It provides a TCI client for ESP32: it connects to a TCI server, reconnects automatically and passes parsed commands to user callbacks.

**This is not a library with the full list of TCI commands.** It supports exactly the commands needed by the application, see [Usage](#usage).

The commands in the library are working and fully functional implementation examples. The exact list of commands needed for each project is determined by the developer from the vendor specification and added to their own copy of the library according to their goals and tasks. Adding only the needed commands keeps the ESP32 free of commands that the project does not use. Accordingly, when using this library, it is recommended to review the list of commands it already contains and comment out or remove those that are not intended to be used.

| Path | Content |
|------|---------|
| [tci/](tci/) | Library: `TCIClient` and `TciMessageParser` |
| [esp32_TCI_example/](esp32_TCI_example/) | Minimal PlatformIO example |
| [ExpertSDR3/TCI](https://github.com/ExpertSDR3/TCI) | TCI protocol specification by Expert Electronics |

## 🏁 Getting Started <a name = "getting_started"></a>
These instructions will get the example running on an ESP32 board.

### Prerequisites
- [PlatformIO](https://platformio.org/) (CLI or IDE extension)
- ESP32 board (the example targets `esp32dev`)
- TCI server reachable over the network, for example ExpertSDR3

### Installing
Clone the repository:

```
git clone https://github.com/VK6NX/ESP32-TCI-lib.git
cd ESP32-TCI-lib/esp32_TCI_example
```

Set the WiFi credentials and the TCI server address in [esp32_TCI_example/src/main.cpp](esp32_TCI_example/src/main.cpp):

```cpp
static const char* WIFI_SSID     = "your-ssid";
static const char* WIFI_PASSWORD = "your-password";

static const char* TCI_HOST = "192.168.16.200";
static const uint16_t TCI_PORT = 50001;
```

Build, upload and open the serial monitor:

```
pio run -t upload
pio device monitor
```

The example prints the connection status and every VFO change (values depend on your network and on the server state):

```
WiFi connected, IP: 192.168.1.50
TCI status: Connecting
TCI status: Connected
VFO: receiver 0, channel 1, frequency 7100000 Hz
```

## 🎈 Usage <a name="usage"></a>
Add the library to your PlatformIO project: copy the [tci/](tci/) folder into the `lib/` folder of the project, or point `lib_extra_dirs` to the folder that contains `tci/`, as the example does. Add the WebSockets dependency to `platformio.ini`:

```ini
lib_deps =
	links2004/WebSockets@2.7.3
```

Minimal sketch:

```cpp
#include <WiFi.h>
#include "tciclient.h"

TCIClient tciClient("192.168.16.200", 50001);

void setup() {
    // Connect to WiFi first
    tciClient.setVfoChangedCallback([](uint32_t receiver, uint32_t channel, double frequency) {
        // VFO:receiver,channel,frequency;
    });
    tciClient.connect();
}

void loop() {
    tciClient.loop();
}
```

`TCIClient` API ([tci/tciclient.h](tci/tciclient.h)). The list below shows the commands contained in the library as examples: the `send...` methods send TCI commands, the `set...Callback` methods (except `setStatusChangeCallback`) are called on incoming commands. The other methods control the connection.

| Method | Description |
|--------|-------------|
| `TCIClient(host, port)` | Creates the client, the address is `ws://host:port` |
| `begin()`, `connect()` | Start the connection |
| `disconnect()` | Close the connection |
| `loop()` | Process WebSocket events, call it from `loop()` |
| `isConnected()`, `getStatus()` | Connection state: `Disconnected`, `Connecting`, `Connected` |
| `getConnectionStatus()` | Connection state as a string |
| `sendCommand(command)` | Send a TCI command, for example `"VFO:0,0,7100000;"`. Ignored when not connected |
| `sendStart()`, `sendStop()` | Standard command: send `START;` / `STOP;`, start / stop the device |
| `sendVfoRead(receiver, channel)` | Standard command: send `VFO:arg1,arg2;` (read), the reply comes to the VFO callback |
| `sendVfoSet(receiver, channel, frequency)` | Standard command: send `VFO:arg1,arg2,arg3;` (set), frequency in Hz |
| `sendTuneDrive(transceiver, power)` | Standard command: send `TUNE_DRIVE:arg1,arg2;`, power from 0 to 100 |
| `sendCwMacros(transceiver, text)` | Standard command: send `CW_MACROS:arg1,arg2;`. In the text `:`, `,`, `;` are replaced with `^`, `~`, `*` as the specification requires |
| `sendCwMacrosStop()` | Standard command: send `CW_MACROS_STOP;`, stop the CW transmission immediately |
| `updateConnection(host, port)` | Change the server, the reconnection is done on the next `loop()` |
| `setAddress(address)` | Set the address in the `ws://host:port` form, used on the next connection |
| `getHost()`, `getPort()`, `getAddress()` | Current server address |
| `setStatusChangeCallback(callback)` | Called on every status change |
| `setProtocolCallback(callback)` | Called on `PROTOCOL:arg1,arg2;` with program name and protocol version |
| `setDeviceCallback(callback)` | Called on `DEVICE:arg1;` with device name |
| `setTrxCountCallback(callback)` | Called on `TRX_COUNT:arg1;` with the number of receivers (transceivers) |
| `setChannelsCountCallback(callback)` | Called on `CHANNELS_COUNT:arg1;` with the number of receive channels |
| `setVfoLimitsCallback(callback)` | Called on `VFO_LIMITS:arg1,arg2;` with lower and upper frequency limits in Hz |
| `setModulationsListCallback(callback)` | Called on `MODULATIONS_LIST:arg1,...,argN;` with the list of modulations |
| `setVfoChangedCallback(callback)` | Called on `VFO:arg1,arg2,arg3;` with receiver, channel and frequency in Hz. Customization example |

Behavior:
- When the connection is lost, the status changes to `Connecting` and the client reconnects automatically every 5 seconds.
- Initialization commands are limited to `PROTOCOL`, `DEVICE`, `TRX_COUNT`, `CHANNELS_COUNT`, `VFO_LIMITS` and `MODULATIONS_LIST`, the ones the example prints, and `READY`, which sets the status to `Connected`.
- Standard commands, examples of sending: `START`, `STOP`, `VFO` (read and set), `TUNE_DRIVE`, `CW_MACROS` and `CW_MACROS_STOP`. In the example they are sent by keys from the serial monitor when the status is `Connected`: `s` START, `x` STOP, `r` VFO read, `v` VFO set, `t` TUNE_DRIVE, `c` CW_MACROS, `q` CW_MACROS_STOP. The `c` key transmits on air, the `q` key stops the transmission.
- Handling of the incoming `VFO` is a customization example: it shows how to add a command needed by the application. `VFOCHANGED` is accepted as a custom alias of `VFO`, it is not a part of the TCI specification.
- Binary frames (IQ and audio streams) are ignored.
- To handle more commands, extend `TciMessageParser::process()` in [tci/tciparser.cpp](tci/tciparser.cpp) the same way as `VFO`. `extractArgs()` extracts up to three arguments, `extractList()` extracts a list of any length, as for `MODULATIONS_LIST`.
- One `TCIClient` instance per program: WebSocket events are routed through a static instance pointer.
- Server behavior (TCI specification 2.1, §4.7): a parameter changed by ExpertSDR3 or another client is locked for 200 ms, a set request within this window is dropped. Fast changes of one parameter are merged on a 30 ms tick, so the VFO callback may not receive all intermediate values.

The full list of TCI commands is in the specification: [TCI Protocol 2.1.pdf](https://github.com/ExpertSDR3/TCI/blob/main/TCI%20Protocol%202.1.pdf).

## 💬 Status and Support <a name = "support"></a>
The implemented commands follow the vendor TCI specification. The library is used by the author in the projects https://vk6nx.net/ER-PWR_control_en.html and https://vk6nx.net/ER-G450_control_en.html.

Questions about the library - Expert Electronics general support channel https://t.me/+80QDiqigD4hkZTI6

## ⛏️ Built Using <a name = "built_using"></a>
- [PlatformIO](https://platformio.org/) - Build system
- [Arduino core for ESP32](https://github.com/espressif/arduino-esp32) - Framework
- [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets) 2.7.3 - WebSocket client

## ⚖️ License <a name = "license"></a>
TCI (Transceiver Control Interface) is owned by Expert Electronics. The TCI protocol specification is Copyright (c) 2021 Expert Electronics and is distributed under the MIT License: https://github.com/ExpertSDR3/TCI, see [LICENSE](https://github.com/ExpertSDR3/TCI/blob/main/LICENSE).

This ESP32 adaptation is developed on the basis of TCI. ESP32 adaptation: Copyright (c) 2026 VK6NX, GPL-3.0-or-later, see [tci/licenses/GPL-3.0.txt](tci/licenses/GPL-3.0.txt).

## ✍️ Authors <a name = "authors"></a>
- [@VK6NX](https://github.com/VK6NX) - ESP32 adaptation

## 🎉 Acknowledgments <a name = "acknowledgement"></a>
- [Expert Electronics](https://github.com/ExpertSDR3/TCI) - TCI protocol and specification
