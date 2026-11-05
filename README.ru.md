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
  <a href="README.md">English</a> | <b>Русский</b>
</p>

<h3 align="center">ESP32-TCI-lib</h3>

<div align="center">

  [![GitHub Issues](https://img.shields.io/github/issues/VK6NX/ESP32-TCI-lib.svg)](https://github.com/VK6NX/ESP32-TCI-lib/issues)
  [![GitHub Pull Requests](https://img.shields.io/github/issues-pr/VK6NX/ESP32-TCI-lib.svg)](https://github.com/VK6NX/ESP32-TCI-lib/pulls)
  [![License](https://img.shields.io/badge/license-GPL--3.0--or--later-blue.svg)](tci/licenses/GPL-3.0.txt)

</div>

---

<p align="center"> WebSocket-клиент TCI v2.x (Transceiver Control Interface) для ESP32 (Arduino framework, PlatformIO).
    <br> 
</p>

## 📝 Содержание
- [О проекте](#about)
- [Начало работы](#getting_started)
- [Использование](#usage)
- [Статус и поддержка](#support)
- [Используемые средства](#built_using)
- [Лицензия](#license)
- [Авторы](#authors)
- [Благодарности](#acknowledgement)

## 🧐 О проекте <a name = "about"></a>
TCI (Transceiver Control Interface) — сетевой протокол Expert Electronics для управления, передачи данных и синхронизации между трансивером и клиентскими программами и устройствами. TCI работает поверх WebSocket: программа трансивера (например, ExpertSDR3) — сервер, все остальные программы и устройства — клиенты.

Данный проект — адаптация для ESP32, разработанная на основе TCI, владельцем которого является Expert Electronics. Проект реализует TCI-клиент для ESP32: подключается к TCI-серверу, автоматически переподключается и передаёт разобранные команды в пользовательские callback-функции.

**Это не библиотека с полным списком команд TCI.** Она поддерживает ровно те команды, которые нужны для приложения, см. [Использование](#usage). Остальные команды TCI не поддерживаются.

Реализованные команды — работающие и полностью функциональные примеры реализации. Точный список команд, нужных для каждого проекта, разработчик определяет сам по спецификации вендора: [ExpertSDR3/TCI](https://github.com/ExpertSDR3/TCI).

| Путь | Содержимое |
|------|------------|
| [tci/](tci/) | Библиотека: `TCIClient` и `TciMessageParser` |
| [esp32_TCI_example/](esp32_TCI_example/) | Минимальный пример для PlatformIO |
| [ExpertSDR3/TCI](https://github.com/ExpertSDR3/TCI) | Спецификация протокола TCI от Expert Electronics |

## 🏁 Начало работы <a name = "getting_started"></a>
Эти инструкции помогут запустить пример на плате ESP32.

### Предварительные требования
- [PlatformIO](https://platformio.org/) (CLI или расширение IDE)
- Плата ESP32 (пример собирается для `esp32dev`)
- TCI-сервер, доступный по сети, например ExpertSDR3

### Установка
Клонировать репозиторий:

```
git clone https://github.com/VK6NX/ESP32-TCI-lib.git
cd ESP32-TCI-lib/esp32_TCI_example
```

Указать параметры WiFi и адрес TCI-сервера в [esp32_TCI_example/src/main.cpp](esp32_TCI_example/src/main.cpp):

```cpp
static const char* WIFI_SSID     = "your-ssid";
static const char* WIFI_PASSWORD = "your-password";

static const char* TCI_HOST = "192.168.16.200";
static const uint16_t TCI_PORT = 50001;
```

Собрать, загрузить и открыть монитор последовательного порта:

```
pio run -t upload
pio device monitor
```

Пример выводит состояние подключения и каждое изменение VFO (значения зависят от вашей сети и состояния сервера):

```
WiFi connected, IP: 192.168.1.50
TCI status: Connecting
TCI status: Connected
VFO: receiver 0, channel 1, frequency 7100000 Hz
```

## 🎈 Использование <a name="usage"></a>
Подключение библиотеки к проекту PlatformIO: скопировать папку [tci/](tci/) в папку `lib/` проекта или указать в `lib_extra_dirs` папку, в которой лежит `tci/`, как это сделано в примере. Добавить зависимость WebSockets в `platformio.ini`:

```ini
lib_deps =
	links2004/WebSockets@2.7.3
```

Минимальный скетч:

```cpp
#include <WiFi.h>
#include "tciclient.h"

TCIClient tciClient("192.168.16.200", 50001);

void setup() {
    // Сначала подключиться к WiFi
    tciClient.setVfoChangedCallback([](uint32_t receiver, uint32_t channel, double frequency) {
        // VFO:receiver,channel,frequency;
    });
    tciClient.connect();
}

void loop() {
    tciClient.loop();
}
```

API `TCIClient` ([tci/tciclient.h](tci/tciclient.h)):

| Метод | Описание |
|-------|----------|
| `TCIClient(host, port)` | Создаёт клиент, адрес — `ws://host:port` |
| `begin()`, `connect()` | Начать подключение |
| `disconnect()` | Закрыть подключение |
| `loop()` | Обработка событий WebSocket, вызывать из `loop()` |
| `isConnected()`, `getStatus()` | Состояние подключения: `Disconnected`, `Connecting`, `Connected` |
| `getConnectionStatus()` | Состояние подключения строкой |
| `sendCommand(command)` | Отправить команду TCI, например `"VFO:0,0,7100000;"`. Без подключения игнорируется |
| `sendStart()`, `sendStop()` | Стандартная команда: отправить `START;` / `STOP;`, запуск / остановка устройства |
| `sendVfoRead(receiver, channel)` | Стандартная команда: отправить `VFO:arg1,arg2;` (чтение), ответ приходит в VFO callback |
| `sendVfoSet(receiver, channel, frequency)` | Стандартная команда: отправить `VFO:arg1,arg2,arg3;` (установка), частота в Гц |
| `sendTuneDrive(transceiver, power)` | Стандартная команда: отправить `TUNE_DRIVE:arg1,arg2;`, мощность от 0 до 100 |
| `sendCwMacros(transceiver, text)` | Стандартная команда: отправить `CW_MACROS:arg1,arg2;`. В тексте `:`, `,`, `;` заменяются на `^`, `~`, `*`, как требует спецификация |
| `updateConnection(host, port)` | Сменить сервер, переподключение выполняется в следующем `loop()` |
| `setAddress(address)` | Задать адрес в виде `ws://host:port`, применяется при следующем подключении |
| `getHost()`, `getPort()`, `getAddress()` | Текущий адрес сервера |
| `setStatusChangeCallback(callback)` | Вызывается при каждой смене состояния |
| `setProtocolCallback(callback)` | Вызывается на `PROTOCOL:arg1,arg2;` с названием программы и версией протокола |
| `setDeviceCallback(callback)` | Вызывается на `DEVICE:arg1;` с названием устройства |
| `setTrxCountCallback(callback)` | Вызывается на `TRX_COUNT:arg1;` с количеством приёмников (трансиверов) |
| `setChannelsCountCallback(callback)` | Вызывается на `CHANNELS_COUNT:arg1;` с количеством каналов приёма |
| `setVfoLimitsCallback(callback)` | Вызывается на `VFO_LIMITS:arg1,arg2;` с нижней и верхней границей частоты в Гц |
| `setModulationsListCallback(callback)` | Вызывается на `MODULATIONS_LIST:arg1,...,argN;` со списком модуляций |
| `setVfoChangedCallback(callback)` | Вызывается на `VFO:arg1,arg2,arg3;` с номером приёмника, каналом и частотой в Гц. Пример кастомизации |

Поведение:
- При потере связи состояние меняется на `Connecting`, клиент автоматически переподключается каждые 5 секунд.
- Команды инициализации ограничены командами `PROTOCOL`, `DEVICE`, `TRX_COUNT`, `CHANNELS_COUNT`, `VFO_LIMITS` и `MODULATIONS_LIST`, которые выводит пример, и `READY`, который переводит состояние в `Connected`. Остальные команды инициализации, например `IF_LIMITS` и `RECEIVE_ONLY`, игнорируются.
- Стандартные команды, примеры отправки: `START`, `STOP`, `VFO` (чтение и установка), `TUNE_DRIVE` и `CW_MACROS`. В примере они отправляются клавишами из монитора последовательного порта при состоянии `Connected`: `s` START, `x` STOP, `r` чтение VFO, `v` установка VFO, `t` TUNE_DRIVE, `c` CW_MACROS. Клавиша `c` включает передачу в эфир.
- Обработка входящей `VFO` — пример кастомизации: показывает, как добавить команду, нужную приложению. `VFOCHANGED` принимается как собственный псевдоним `VFO`, в спецификацию TCI он не входит.
- Остальные команды и бинарные кадры (IQ- и аудиопотоки) игнорируются.
- Для обработки других команд нужно расширить `TciMessageParser::process()` в [tci/tciparser.cpp](tci/tciparser.cpp) так же, как сделано для `VFO`. `extractArgs()` извлекает до трёх аргументов, `extractList()` — список любой длины, как для `MODULATIONS_LIST`.
- Один экземпляр `TCIClient` на программу: события WebSocket передаются через статический указатель на экземпляр.

Полный список команд TCI — в спецификации: [TCI Protocol_RU 2.1.pdf](https://github.com/ExpertSDR3/TCI/blob/main/TCI%20Protocol_RU%202.1.pdf).

## 💬 Статус и поддержка <a name = "support"></a>
Библиотека не является полной реализацией TCI: она поддерживает только команды, перечисленные в разделе [Использование](#usage). Реализованные команды соответствуют спецификации вендора. Библиотека используется автором в проектах https://vk6nx.net/ER-PWR_control_en.html и https://vk6nx.net/ER-G450_control_en.html.

Вопросы по библиотеке — канал общей поддержки Expert Electronics https://t.me/+80QDiqigD4hkZTI6

## ⛏️ Используемые средства <a name = "built_using"></a>
- [PlatformIO](https://platformio.org/) — Система сборки
- [Arduino core for ESP32](https://github.com/espressif/arduino-esp32) — Фреймворк
- [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets) 2.7.3 — WebSocket-клиент

## ⚖️ Лицензия <a name = "license"></a>
Владельцем TCI (Transceiver Control Interface) является Expert Electronics. Спецификация протокола TCI — Copyright (c) 2021 Expert Electronics, распространяется под лицензией MIT: https://github.com/ExpertSDR3/TCI, см. [LICENSE](https://github.com/ExpertSDR3/TCI/blob/main/LICENSE).

Данная адаптация для ESP32 разработана на основе TCI. Адаптация для ESP32: Copyright (c) 2026 VK6NX, GPL-3.0-or-later, см. [tci/licenses/GPL-3.0.txt](tci/licenses/GPL-3.0.txt).

## ✍️ Авторы <a name = "authors"></a>
- [@VK6NX](https://github.com/VK6NX) — Адаптация для ESP32

## 🎉 Благодарности <a name = "acknowledgement"></a>
- [Expert Electronics](https://github.com/ExpertSDR3/TCI) — Протокол и спецификация TCI
