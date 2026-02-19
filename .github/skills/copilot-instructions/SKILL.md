---
name: copilot-instructions
description: Project instructions for Holocubic ESP32 GIF player, including architecture, coding conventions, and common pitfalls.
---

# Holocubic — Copilot Project Instructions

## Project Overview

ESP32 透明小電視，透過 SD 卡播放 GIF 動畫與音樂正在播放資訊，支援網頁管理介面、傾斜切換 GIF 與 App 切換。

## Hardware

- **MCU**: ESP32 NodeMCU-32S (dual-core 240MHz, 320KB RAM, 4MB Flash)
- **Display**: ST7735 TFT 128×160, 只使用中間 128×128 區域 (Y offset = 16)
- **IMU**: MPU6050 (I2C, 左右傾斜切換 GIF、前後傾斜切換 App)
- **Storage**: SD card (SPI, 20MHz)
- **顯示架構**: 雙 GFXcanvas16 緩衝 (128×128×2 bytes each = 64KB total)

## Architecture

### App System
`main.cpp` 管理 App 生命週期。`apps[]` 陣列、`currentAppIndex`、`APP_COUNT` 定義在 main.cpp。
切換方式：MPU 前後傾斜 或 Web API `POST /api/mode`。

```cpp
class App {
    virtual void onEnter() = 0;
    virtual void onExit() = 0;
    virtual void loop() = 0;
    virtual bool onTilt(int direction) { return false; }
    virtual const char *name() const = 0;
};
```

| Module | Class | Instance | Purpose |
|--------|-------|----------|---------|
| `App/` | `App` | — | 抽象基類（含 overlay 管理） |
| `GifApp/` | `GifApp` | `gifApp` | GIF 播放，透過 FrameLoader 雙核 pipeline |
| `NowPlayingApp/` | `NowPlayingApp` | `nowPlayingApp` | 音樂正在播放 mini frame player |
| `DiceApp/` | `DiceApp` | `diceApp` | 骰子動畫，傾斜觸發，DICE_IDLE/DICE_ROLLING/DICE_RESULT |
| `FishTankApp/` | `FishTankApp` | `fishTankApp` | 動態魚缸，4 魚 + 8 泡泡 + 3 植物，float 物理 |
| `RacingApp/` | `RacingApp` | `racingApp` | 透視公路賽車，MPU 操控，RACING_PLAYING/RACING_GAMEOVER |

```cpp
// main.cpp
App *apps[] = {&gifApp, &nowPlayingApp, &fishTankApp, &diceApp, &racingApp};
```

### App Base Class — Overlay Management
`App` 基類內建 overlay 顯示管理，`main.cpp` 在 loop 中呼叫 `apps[currentAppIndex]->updateOverlay()`，在 `switchApp()` 時呼叫 `->triggerOverlay()`：

```cpp
void triggerOverlay();       // 設 _overlayVisible=true，記錄時間戳
void updateOverlay();        // 超過 OVERLAY_SHOW_MS (2500ms) 後清除
bool isOverlayVisible() const;
```

### Dual-Core Pipeline (GifApp + FrameLoader)
- **Core 0**: `FrameLoader` 背景任務 `"FrameLoader"` — 從 SD 解碼 BMP，檢查 `uploadManager.isUploading()` 後再讀取
- **Core 1**: Arduino `loop()` — 渲染 TFT、處理傾斜、Web server
- GifApp 在 `onEnter()` 呼叫 `frameLoader.begin()`，用 `frameLoader.requestLoad()` / `isLoaded()` / `consumeLoaded()` 驅動 pipeline

### NowPlayingApp
- PC companion (`companion/now_playing.py`) 偵測 Windows SMTC 正在播放的音樂
- Companion 用 Pillow 預渲染捲動動畫幀 (128×128 BMP)，支援 CJK 字型
- 透過 HTTP 上傳 BMP 幀到 SD 卡 `/np/` 目錄
- ESP32 端只做 mini frame player，循序播放 `/np/{n}.bmp`
- API 流程：`POST /api/now-playing` → `POST /api/np/frame/{n}` × N → `POST /api/np/ready`

### MPU Tilt Detection
- **左右傾斜 (Roll)**: 傳給當前 App 的 `onTilt()`，GifApp 用來切換 GIF
- **前後傾斜 (Pitch)**: 切換 App (`switchApp()`)
- 門檻：進入 25°、退出 15°、冷卻 2000ms

### Shared Modules (`lib/`)
每個模組都是獨立的 class + 全域 `extern` 實例：

| Module | Class | Instance | Purpose |
|--------|-------|----------|---------|
| `Display/` | `Display` | `display` | TFT 渲染、BMP 解碼、overlay |
| `MPU/` | `MPU` | `mpu` | 加速度計傾斜偵測 (Roll + Pitch) |
| `GifManager/` | `GifManager` | `gifManager` | SD 卡 GIF CRUD、排序 |
| `FrameLoader/` | `FrameLoader` | `frameLoader` | Core 0 背景 BMP 載入任務（獨立 lib） |
| `WiFiManager/` | `WiFiManager` | `wifiManager` | WiFi 連線：STA 模式 + AP fallback |
| `WebServer/` | — | — | REST API、嵌入式網頁（見下方詳細架構） |

### FrameLoader (`lib/FrameLoader/`)
獨立 lib 模組，不嵌在 GifApp 內：
- `begin()` — 在 Core 0 生成任務 `"FrameLoader"` (stack 4096, priority 1)
- `requestLoad(path)` — 請求載入指定 BMP 路徑
- `isLoaded()` / `isBusy()` — 查詢狀態
- `consumeLoaded()` — 消費已載入的幀（清除 `_frameLoaded`）
- `waitIdle()` — 等待任務空閒
- 每次載入前檢查 `uploadManager.isUploading()`，若上傳中則跳過

### WiFiManager (`lib/WiFiManager/`)
- `begin()` — 讀取 `/wifi.json`，嘗試 STA 模式，失敗時回退 AP 模式 (SSID: "Holocubic", pass: "12345678")
- `isConnected()` — 查詢連線狀態
- `setup()` 中呼叫 `wifiManager.begin()`

### Web Server Architecture
`lib/WebServer/` 拆分為四個模組，避免 God class：

| File | Type | Instance | Purpose |
|------|------|----------|---------|
| `upload_manager.h/.cpp` | `UploadManager` class | `uploadManager` (extern) | 檔案上傳狀態機 + SD 檔案 I/O |
| `gif_routes.h/.cpp` | `GifRoutes` namespace | — | GIF CRUD + frame/original 上傳路由 |
| `np_routes.h/.cpp` | `NpRoutes` namespace | — | NowPlaying metadata + frame 上傳路由 |
| `web_server.h/.cpp` | `HoloWebServer` class | `webServer` (extern) | 協調器：WiFi、mode、HTML 路由 |
| `web_html.h` | PROGMEM 常數 | — | INDEX_HTML + WIFI_HTML 嵌入式網頁 |

**路由註冊流程**: `HoloWebServer::setupRoutes()` 呼叫 `GifRoutes::registerRoutes(_server)` 和 `NpRoutes::registerRoutes(_server)`，WiFi/mode handlers 以 lambda 內聯在 `setupRoutes()` 中。

**UploadManager** — 共用於 GIF 上傳和 NP frame 上傳：
- 擁有 `File _file` / `File _originalFile`、`char _path[64]`、volatile 狀態旗標
- API: `openFile()` / `writeChunk()` / `closeFile()` / `openOriginal()` / `writeOriginalChunk()` / `closeOriginal()`
- `consumeError()`: 回傳目前 error 狀態並清除（供 response lambda 使用）
- `isUploadActive()` bridge 函式定義於 `upload_manager.cpp`，供 `NowPlayingApp` extern 呼叫

**GifRoutes** — 9 個 handler 為 static free functions，直接使用 `uploadManager` 全域實例：
- `_onGifChange` static callback，透過 `GifRoutes::setOnGifChange()` 設定
- `uploadResponseHandler()` 共用 response lambda（檢查 `uploadManager.consumeError()`）

**NpRoutes** — 4 個 handler 為 static free functions：
- NP frame upload response lambda 在 `registerRoutes()` 中內聯定義
- 使用 `nowPlayingApp` extern 實例

**HoloWebServer** — 瘦身協調器：
- `web_server.h` 不包含 `<ArduinoJson.h>`（避免傳遞依賴）
- `isUploading()` 委派至 `uploadManager.isUploading()`
- `checkUploadTimeout()` 委派至 `uploadManager.checkTimeout()`
- `setOnGifChange()` 委派至 `GifRoutes::setOnGifChange()`
- `setOnModeChange(callback)` — `POST /api/mode` 收到時以 app index 呼叫 callback
- `setAppInfo(apps, &APP_COUNT, &currentAppIndex)` — 提供 app 清單供 mode API 使用
- `getLocalIP()` — 回傳 IP 字串（供 overlay 顯示）

#### Upload Error Recovery
- Upload handler response lambda 使用 `uploadManager.consumeError()` 回傳 500 或 200
- `_fileOpen` / `_origFileOpen` (`volatile bool`) 在 UploadManager 中追蹤檔案開啟狀態
- Write error 只設 `_uploadError=true`，**不**在中途 close file（由 `final` 區塊統一處理）
- `checkTimeout()`：從 main loop 呼叫，僅設 flag，**絕不**直接 close file
  - 原因：upload handler 在 async TCP task 中執行，main loop 在 Arduino task，跨 task close file 會造成 heap corruption
- `abort()`：僅從 Web API route (同 task context) 呼叫，可安全 close file
- `UPLOAD_TIMEOUT_MS` (30s)：超時自動清除 `_isUploading` 狀態

### SD Card File Structure
```
/wifi.json              — WiFi credentials
/gifs/
  order.json            — GIF playback order
  <name>/
    config.json          — {frameCount, width, height, defaultDelay}
    0.bmp ... N.bmp      — BMP frames (RGB565 16-bit or BGR 24-bit)
    original.gif         — Original GIF for web preview
/np/
  0.bmp ... N.bmp       — NowPlaying pre-rendered animation frames
```

### Companion Script (`companion/`)
- `now_playing.py`：Windows companion，偵測 SMTC 正在播放的音樂
- 依賴：`winrt-Windows.Media.Control`、`winrt-Windows.Storage.Streams`、`Pillow`、`requests`
- 在 PC 端用 Pillow + CJK 字型渲染捲動動畫幀，上傳到 ESP32
- 每幀 128×128 BMP：上方 100px 專輯封面，下方 28px 半透明文字列
- 上傳含重試機制 (3 次) + 延遲 (300ms after track info, 500ms between retries)

## Coding Conventions

### Memory Management (Critical)
- **禁止**在 hot path 使用 `String` 拼接 — 用 `char buf[N]` + `snprintf`
- 路徑緩衝固定 64 bytes (`char path[64]`)
- 大型 HTML 不用 `request->send(200, "text/html", LARGE_PROGMEM)` — 用 chunked streaming
- 上傳期間設 `_isUploading = true` 暫停 SD 讀取

### Concurrency Safety (Critical)
- ESPAsyncWebServer upload callback 在 async TCP task 中執行（非 Arduino loop task）
- main loop 中的 `checkUploadTimeout()` **只能設 flag**，不能操作 File 物件
- `volatile` 修飾跨 task 共享的布林值 (`_fileOpen`, `_origFileOpen`, FrameLoader 的 `_frameLoaded`, `_loaderBusy`)
- SD 卡存取衝突：上傳時 `_isUploading=true`，FrameLoader 會跳過 SD 讀取

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
- `APP_COUNT` 在 main.cpp 需用 `extern const int` 宣告（C++ const 預設 internal linkage）

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
6. Upload handler 的 response lambda 是在 upload 完成後才呼叫 — 不能在 upload handler 中直接 `request->send()`
7. `checkUploadTimeout()` 絕不能 close file — 會與 async TCP task 競爭導致 heap corruption
8. C++ `const int` 預設 internal linkage — 跨 translation unit 需要 `extern const int` 定義
9. Upload error 時不要在中途 close file — 由 `final` 區塊統一 close，避免 double-close crash
