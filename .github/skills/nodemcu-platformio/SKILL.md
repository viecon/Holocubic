---
name: nodemcu-platformio
description: Development guide for NodeMCU-32S with PlatformIO — board setup, pin reference, SPI/I2C, FreeRTOS dual-core, memory management, and common pitfalls.
---

# NodeMCU-32S + PlatformIO — Development Guide

## Board Overview

| Property | Value |
|----------|-------|
| MCU | ESP32 (Xtensa LX6 dual-core, 240 MHz) |
| RAM | 320 KB SRAM (+ 4 MB PSRAM on some modules) |
| Flash | 4 MB |
| USB-Serial | CH340C (built-in) |
| Board ID | `nodemcu-32s` |

---

## platformio.ini Baseline

```ini
[env:nodemcu-32s]
platform  = espressif32
board     = nodemcu-32s
framework = arduino
monitor_speed = 115200

; Custom partition (needed when app > ~1.2 MB)
board_build.partitions = huge_app.csv

build_flags =
    -DCORE_DEBUG_LEVEL=0   ; 0=off, 1=error, 2=warn, 3=info, 4=debug, 5=verbose
    -I include             ; expose include/ to all translation units

lib_deps =
    ; add libraries here — PlatformIO resolves + caches them automatically
```

### Useful build_flags
| Flag | Effect |
|------|--------|
| `-DCORE_DEBUG_LEVEL=4` | Enable ESP-IDF debug logs to Serial |
| `-DASYNCWEBSERVER_REGEX` | Enable regex routing in ESPAsyncWebServer |
| `-DBOARD_HAS_PSRAM` | Activate PSRAM (if module has it) |
| `-O2` / `-Os` | Optimize for speed / size |

---

## Pin Reference (NodeMCU-32S)

### SPI (VSPI — default)
| Signal | GPIO |
|--------|------|
| SCK    | 18   |
| MISO   | 19   |
| MOSI   | 23   |
| CS     | any  |

> Multiple SPI devices share SCK/MISO/MOSI; each needs its own CS pin.  
> Call `SPI.begin(SCK, MISO, MOSI)` once, then pass CS to each device.

### I2C (default)
| Signal | GPIO |
|--------|------|
| SDA    | 21   |
| SCL    | 22   |

> `Wire.begin()` uses these defaults. Override with `Wire.begin(SDA, SCL)`.

### UART
| Port   | TX  | RX  | Notes |
|--------|-----|-----|-------|
| UART0  | 1   | 3   | Used by Serial (USB-Serial bridge) |
| UART1  | 10  | 9   | Avoid — overlaps flash pins on most modules |
| UART2  | 17  | 16  | Safe for `Serial2` |

### PWM / Analog
- All digital GPIOs support `ledcWrite()` (PWM via LEDC peripheral)
- ADC1: GPIOs 32–39 (safe to use with WiFi)
- ADC2: GPIOs 0, 2, 4, 12–15, 25–27 — **disabled while WiFi is active**
- DAC: GPIO 25, 26

### Avoid / Reserved
| GPIO | Reason |
|------|--------|
| 6–11 | Flash SPI — **never use** |
| 0    | Boot mode — pull-up required at boot |
| 2    | Boot mode — must be low/floating at boot |
| 12   | Flash voltage strapping — keep low at boot |
| 34–39 | Input only (no pull-up/down) |

---

## SPI Setup (Multiple Devices)

```cpp
#include <SPI.h>

// In setup():
SPI.begin(18, 19, 23);          // SCK, MISO, MOSI
SPI.setFrequency(40000000);     // default bus frequency

// Per-device: pass custom SPISettings or use CS directly
// e.g. SD at 20 MHz, TFT at 40 MHz — handled by each library
```

**SD + TFT on same SPI bus**: both work as long as their CS pins are distinct and only one is active at a time. The libraries handle this automatically.

---

## I2C Setup

```cpp
#include <Wire.h>

Wire.begin(21, 22);             // SDA, SCL (default)
Wire.setClock(400000);          // 400 kHz Fast Mode

// Scan for devices
for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0)
        Serial.printf("Found I2C device at 0x%02X\n", addr);
}
```

---

## Dual-Core FreeRTOS

ESP32 has two cores. Arduino `loop()` runs on **Core 1**. Use Core 0 for background work.

```cpp
TaskHandle_t myTask = NULL;

void myTaskFunc(void *param) {
    for (;;) {
        // do background work
        vTaskDelay(pdMS_TO_TICKS(10));   // yield — never block indefinitely without yield
    }
}

// In setup():
xTaskCreatePinnedToCore(
    myTaskFunc,   // function
    "MyTask",     // name (for debugging)
    4096,         // stack size in bytes
    NULL,         // parameter
    1,            // priority (1 = low, configMAX_PRIORITIES-1 = high)
    &myTask,      // handle
    0             // core: 0 or 1
);
```

### Task Notification (lightweight signal between tasks)
```cpp
// Sender (e.g., Core 1):
xTaskNotifyGive(myTask);

// Receiver (Core 0 task):
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // block until notified
```

### Cross-Task Safety Rules
- **`volatile`** — mark any bool/int shared across tasks
- **Never** call `File.close()` from a different task than opened it (SD heap corruption)
- **Never** touch `AsyncWebServer` response objects outside their callback context
- Use `xSemaphoreCreateMutex()` when shared state is complex

---

## Memory Management

### Heap Budget
| Region | Size | Notes |
|--------|------|-------|
| Internal SRAM | ~320 KB | General heap |
| IRAM | ~128 KB | Code/ISR cache — not for data |
| PSRAM (optional) | 4–8 MB | Must enable with `-DBOARD_HAS_PSRAM` |

### Rules
- **No `String` in hot paths** — use `char buf[N]` + `snprintf`
- **No large stack allocations** — `char buf[4096]` on stack will stack-overflow; use heap or `static`
- Path buffers: 64 bytes is enough for SD paths (`/gifs/name/0.bmp`)
- Keep free heap > 30 KB — async web server becomes unstable below that
- Check heap: `Serial.printf("Free heap: %d\n", ESP.getFreeHeap())`
- PROGMEM for large string constants: `const char HTML[] PROGMEM = "...";`

### Partition Tables
| Table | App Size | Notes |
|-------|----------|-------|
| `default.csv` | ~1.2 MB | Default |
| `huge_app.csv` | ~3 MB | Use when firmware > 1.2 MB |
| `min_spiffs.csv` | ~1.9 MB | Larger app + small SPIFFS |

Set in `platformio.ini`: `board_build.partitions = huge_app.csv`

---

## WiFi

```cpp
#include <WiFi.h>

// Station mode
WiFi.begin("SSID", "password");
while (WiFi.status() != WL_CONNECTED) delay(500);
Serial.println(WiFi.localIP());

// Access Point fallback
WiFi.softAP("MyDevice", "password");
Serial.println(WiFi.softAPIP());  // default: 192.168.4.1
```

**Important**: ADC2 is unavailable while WiFi is active. Use ADC1 (GPIOs 32–39) instead.

---

## Serial Debugging

```cpp
Serial.begin(115200);           // in setup()
Serial.printf("[Tag] value=%d\n", val);   // preferred over println

// ESP-IDF log macros (require -DCORE_DEBUG_LEVEL >= N):
ESP_LOGE("Tag", "Error: %d", code);   // level 1
ESP_LOGW("Tag", "Warning");           // level 2
ESP_LOGI("Tag", "Info");              // level 3
ESP_LOGD("Tag", "Debug");             // level 4
```

PlatformIO monitor: `pio device monitor --baud 115200`  
Or use the PlatformIO IDE sidebar → Serial Monitor.

---

## OTA (Over-the-Air Updates)

```cpp
#include <ArduinoOTA.h>

// In setup() after WiFi connected:
ArduinoOTA.setHostname("my-device");
ArduinoOTA.begin();

// In loop():
ArduinoOTA.handle();
```

Upload via PlatformIO: add `upload_protocol = espota` and `upload_port = <IP>` to `platformio.ini`.

---

## Common Libraries (Arduino framework)

| Library | PlatformIO ID | Use |
|---------|--------------|-----|
| Adafruit GFX | `adafruit/Adafruit GFX Library` | Canvas, fonts, drawing primitives |
| Adafruit ST7735/7789 | `adafruit/Adafruit ST7735 and ST7789 Library` | TFT display driver |
| ArduinoJson 7 | `bblanchon/ArduinoJson@^7.0.0` | JSON config files on SD |
| ESPAsyncWebServer | `mathieucarbou/ESPAsyncWebServer@^3.1.0` | Non-blocking HTTP server |
| SD (built-in) | `SD` (ESP32 Arduino core) | SD card via SPI |
| Wire (built-in) | `Wire` | I2C |
| SPIFFS/LittleFS | built-in | Internal flash filesystem |

---

## SD Card

```cpp
#include <SD.h>
#include <SPI.h>

SPI.begin(18, 19, 23);
if (!SD.begin(SD_CS, SPI, 20000000)) {   // CS pin, bus, 20 MHz
    Serial.println("SD failed");
}

// Read file
File f = SD.open("/config.json");
if (f) {
    while (f.available()) Serial.write(f.read());
    f.close();
}
```

**Gotchas:**
- `entry.name()` returns the **full path**, not just the filename
- SD and TFT share SPI — never overlap their CS simultaneously
- Max reliable speed: 20 MHz (40 MHz is unstable on long wires)

---

## Common Pitfalls

1. **GPIO 6–11 are flash pins** — using them silently corrupts flash or crashes the chip
2. **ADC2 + WiFi conflict** — ADC2 reads return garbage while WiFi is active
3. **`String` fragmentation** — avoid repeated concatenation in long-running firmware; use `snprintf` into fixed buffers
4. **Stack overflow on tasks** — 4096 bytes is the minimum safe stack; increase to 8192 for tasks doing JSON parsing or String ops
5. **Strapping pins at boot** — GPIO 0 (must be high), GPIO 2 (must be low/float), GPIO 12 (keep low) — wrong levels prevent boot or flash
6. **`delay()` in FreeRTOS tasks** — use `vTaskDelay(pdMS_TO_TICKS(ms))` instead; `delay()` calls `vTaskDelay` internally but is less explicit
7. **Cross-task SD/file access** — always open, read/write, and close a `File` object within the same task; passing `File` objects across tasks causes heap corruption
8. **`const int` internal linkage** — C++ `const` at file scope is `static` by default; use `extern const int` when sharing across translation units
9. **Brownout detector** — power supply must provide ≥500 mA; USB hubs often cause random resets (brownout); disable with `esp_sleep_pd_config` if needed during development
10. **Upload timeout with ESPAsyncWebServer** — response lambda runs after upload completion; never call `request->send()` inside the upload body handler
