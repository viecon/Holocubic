---
name: copilot-instructions
description: Project instructions for Holocubic ESP32 GIF player, including architecture, coding conventions, and common pitfalls.
---

# Holocubic — Copilot Project Instructions

## Project Overview

ESP32 透明小電視，透過 SD 卡播放 GIF 動畫，支援網頁管理介面與傾斜切換。

## Hardware

- **MCU**: ESP32 NodeMCU-32S (dual-core 240MHz, 320KB RAM, 4MB Flash)
- **Display**: ST7735 TFT 128×160, 只使用中間 128×128 區域 (Y offset = 16)
- **IMU**: MPU6050 (I2C, 傾斜偵測切換 GIF)
- **Storage**: SD card (SPI, 20MHz)
- **顯示架構**: 雙 GFXcanvas16 緩衝 (128×128×2 bytes each = 64KB total)

## Architecture

### Dual-Core Pipeline
- **Core 0**: 背景任務 `frameLoaderTask` — 從 SD 解碼 BMP 到 back buffer
- **Core 1**: Arduino `loop()` — 渲染 TFT、處理傾斜、Web server

### Module Structure (`lib/`)
每個模組都是獨立的 class + 全域 `extern` 實例：

| Module | Class | Instance | Purpose |
|--------|-------|----------|---------|
| `Display/` | `Display` | `display` | TFT 渲染、BMP 解碼、overlay |
| `MPU/` | `MPU` | `mpu` | 加速度計傾斜偵測 |
| `GifManager/` | `GifManager` | `gifManager` | SD 卡 GIF CRUD、排序 |
| `WebServer/` | `HoloWebServer` | `webServer` | REST API、嵌入式網頁 |

### Web Server Pattern
- ESPAsyncWebServer with regex routes
- HTML 以 `PROGMEM` 存放、用 chunked streaming `beginResponse()` 回傳
- Upload handlers 使用 `char _uploadPath[64]` + `snprintf`（避免 String heap 分配）
- JSON 用 ArduinoJson v7 (`JsonDocument`)

### SD Card File Structure
```
/wifi.json              — WiFi credentials
/gifs/
  order.json            — GIF playback order
  <name>/
    config.json          — {frameCount, width, height, defaultDelay}
    0.bmp ... N.bmp      — BMP frames (RGB565 16-bit or BGR 24-bit)
    original.gif         — Original GIF for web preview
```

## Coding Conventions

### Memory Management (Critical)
- **禁止**在 hot path 使用 `String` 拼接 — 用 `char buf[N]` + `snprintf`
- 路徑緩衝固定 64 bytes (`char path[64]`)
- 大型 HTML 不用 `request->send(200, "text/html", LARGE_PROGMEM)` — 用 chunked streaming
- 上傳期間設 `_isUploading = true` 暫停 SD 讀取

### Code Style
- 所有常數定義在 `include/config.h`
- Serial log 格式: `Serial.printf("[ModuleName] message\n", ...)`
- Header guard: `#ifndef MODULE_H` / `#define MODULE_H` / `#endif`
- Class 成員變數以 `_` 開頭: `_tft`, `_canvas`, `_pathBuf`
- 保持註解精簡，避免重複說明顯而易見的程式碼

### PlatformIO
- Platform: `espressif32`
- Framework: `arduino`
- Partition: `huge_app.csv` (單一大 app partition)
- Build flags: `-DASYNCWEBSERVER_REGEX -I include`
- 所有 lib 放在 `lib/` 下，每個有自己的 `.h` + `.cpp`

### Performance Targets
- SPI TFT: 40MHz
- SPI SD: 20MHz
- I2C: 400kHz
- Frame pipeline: Core 0 pre-load → Core 1 swap + render (零等待)

## Common Pitfalls
1. `request->send()` 配大型 PROGMEM 會 heap exhaustion → 用 `beginResponse()` callback
2. SD 和 TFT 共用 SPI bus → 上傳時必須暫停 frame loader
3. ESP32 `SD.open()` 的 `entry.name()` 回傳完整路徑，不只檔名
4. BMP 可能是 top-down (h<0) 或 bottom-up (h>0)，需處理兩種
5. Free heap < 30KB 容易導致 async web server crash
