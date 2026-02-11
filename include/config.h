#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// WiFi
#define WIFI_CONFIG_FILE "/wifi.json"
#define AP_SSID "Holocubic"
#define AP_PASSWORD "12345678"
#define WIFI_CONNECT_TIMEOUT 15

// Hardware Pins
#define SD_CS 5

// TFT Display (ST7735)
#define TFT_CS 13
#define TFT_DC 2
#define TFT_RST 4

// SPI
#define PIN_SCK 18
#define PIN_MISO 19
#define PIN_MOSI 23

// I2C (MPU6050)
#define I2C_SDA 21
#define I2C_SCL 22

// MPU6050
#define MPU_ADDR 0x68
#define REG_PWR_MGMT_1 0x6B
#define REG_ACCEL_XOUT 0x3B

// Display
#define TFT_WIDTH 128
#define TFT_HEIGHT 160
#define CANVAS_WIDTH 128
#define CANVAS_HEIGHT 128
#define CANVAS_X 0
#define CANVAS_Y 16

// Overlay
#define OVERLAY_HEIGHT 16
#define OVERLAY_SHOW_MS 2500

// NTP
#define NTP_SERVER "pool.ntp.org"
#define NTP_GMT_OFFSET 28800
#define NTP_DAYLIGHT_OFFSET 0

// Tilt Detection
#define ENTER_TILT_DEG 25.0f
#define EXIT_TILT_DEG 15.0f
#define SWITCH_COOLDOWN_MS 2000

// SD Card Paths
#define GIFS_ROOT "/gifs"
#define ORDER_FILE "/gifs/order.json"
#define GIF_CONFIG_FILE "config.json"

// Performance
#define SPI_FREQUENCY 40000000
#define SD_SPI_FREQUENCY 20000000
#define I2C_FREQUENCY 400000

// Buffers
#define MAX_ROW_BUFFER ((CANVAS_WIDTH * 3 + 3) & ~3)
#define MAX_IMAGE_SIZE 128
#define MAX_GIF_NAME_LEN 32

// Now Playing
#define NP_DIR "/np"
#define NP_FRAME_DELAY_MS 400

// Upload
#define UPLOAD_TIMEOUT_MS 30000

#endif // CONFIG_H
