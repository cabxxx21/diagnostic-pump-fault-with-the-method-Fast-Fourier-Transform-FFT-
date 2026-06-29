#ifndef CONFIG_H
#define CONFIG_H

// ==========================================================================
//  PUMP FAULT DIAGNOSTIC SYSTEM v6.0 — CONFIG FILE
// ==========================================================================

// library
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <arduinoFFT.h>
#include <EEPROM.h>
#include <esp_task_wdt.h>
#include <Adafruit_MLX90614.h>
#include <SD.h>
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TFT_eSPI.h>

// objek tft & sprite
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
TFT_eSprite sprMenu = TFT_eSprite(&tft);

// wifi, ntp & telegram
const char *WIFI_SSID = "@@**@@**";
const char *WIFI_PASS = "****";
const long GMT_OFFSET_SEC = 7 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// warna ui/html (rgb565)
#define COLOR_BG_MAIN   0x1084 // #121212
#define COLOR_BG_CARD   0x1CE7 // #1E1E1E
#define COLOR_BG_MENU   0x2945 // #2A2A2A
#define COLOR_BORDER    0x3186 // #333333
#define COLOR_TEXT_MAIN 0xD69A // #E0E0E0
#define COLOR_TEXT_DIM  0x7BEF // #757575
#define COLOR_CYAN      0x07FF // #00E5FF
#define COLOR_GREEN     0x07E0 // #00E676
#define COLOR_YELLOW    0xFFE0 // #FFD600
#define COLOR_RED       0xF800 // #FF3D00
#define COLOR_BLUE      0x2B5F // #2979FF

// warna hmi (dark industrial)
#define HMI_BG_DARK     TFT_BLACK
#define HMI_BG_CARD     0x18E3 
#define HMI_BORDER      0x3186 
#define HMI_CYAN        TFT_CYAN
#define HMI_GREEN       TFT_GREEN
#define HMI_YELLOW      TFT_YELLOW
#define HMI_RED         TFT_RED
#define HMI_BLUE        TFT_BLUE
#define HMI_TEXT_MAIN   TFT_WHITE
#define HMI_TEXT_DIM    0x7BEF

// warna hmi light theme
#define HMI_BG_LIGHT     TFT_WHITE
#define HMI_BG_CARD_L    0xE71C
#define HMI_TEXT_DARK    TFT_BLACK

// pin sensor & sd card
#define ADXL_SCK        6  
#define ADXL_MISO       5  
#define ADXL_MOSI       4  
#define ADXL_CS         7

#define SD_SCLK         12
#define SD_MISO         13
#define SD_MOSI         11
#define SD_CS           8

#define MLX_SDA         1
#define MLX_SCL         2

// pin tombol
#define PIN_UP          15
#define PIN_SELECT      16
#define PIN_DOWN        17

// konfigurasi fft
#define SAMPLES         1024
#define SAMPLING_FREQ   800.0
#define FREQ_RES        (SAMPLING_FREQ / SAMPLES)
#define BIN_TOLERANCE   3
#define HANN_GAIN       0.5
#define FFT_NORM        (2.0 / ((double)SAMPLES * HANN_GAIN))

#define NTP_RETRY_INTERVAL_MS 300000
#define WIFI_TIMEOUT_MS       15000
#define TG_HTTP_TIMEOUT_MS    10000
#define ALERT_COOLDOWN_MS     300000

#define TELEGRAM_BOT_TOKEN    "*******"
#define TELEGRAM_CHAT_ID      "*******"

// parameter sistem & eeprom
#define BOOT_DELAY_MS         8000UL
#define WDT_TIMEOUT_S         20
#define EEPROM_SIZE           1024
#define EEPROM_MAGIC          0xA5A5F5F5UL
#define DEBOUNCE_MS           10
#define LONG_PRESS_MS         300
#define REPEAT_MS             100

// baseline & stabilitas
#define BASELINE_N             50
#define STAB_MAX_MS            300000UL
#define STAB_CHECK_MS          10000UL
#define STANDBY_CHECK_MS       5000UL
#define RUN_UP_IGNORE_MS       10000UL
#define STAB_RMS_TOL           0.05f
#define STAB_NEED_COUNT        3
#define STD_DEV_RATIO_MAX_RMS  0.10f 
#define STD_DEV_RATIO_MAX_SPEC 0.20f 
#define NOISE_FLOOR            0.10f

// motor stop & smoothing
#define MOTOR_STOP_RATIO       0.20f
#define MOTOR_STOP_FLOOR       0.03f
#define CONFIDENCE_THRESHOLD   30.0f
#define SMOOTHING_ALPHA        0.3f
#define FAULT_HISTORY_SIZE     5
#define VOTES_TO_CONFIRM       3

// threshold mutliplier
#define ALERT_RMS_M            2.0f
#define DANGER_RMS_M           4.0f
#define ALERT_1X_M             2.5f
#define DANGER_1X_M            5.0f
#define ALERT_2X_M             3.0f
#define DANGER_2X_M            6.0f
#define ALERT_3X_M             3.5f
#define DANGER_3X_M            7.0f

// --- DIAGNOSTIC MAGIC NUMBERS ---
#define UB_SCORE_BASE         50.0f
#define UB_RAD_AX_RATIO       1.5f
#define UB_RAD_AX_ADD         30.0f
#define UB_2X_RATIO           0.5f
#define UB_3X_RATIO           0.3f
#define UB_HARM_ADD           20.0f

#define AM_BASE_ADD           40.0f
#define AM_AX_RAD_RATIO       1.2f
#define AM_AX_RAD_ADD         50.0f

#define PM_BASE_ADD           40.0f
#define PM_2X_RATIO           1.0f
#define PM_2X_ADD             40.0f

#define LO_BASE_ADD           20.0f
#define LO_3X_RATIO           0.3f
#define LO_MAX_ADD            80.0f
#define LO_NORM_RATIO         0.6f

// tabel iso & sd options
struct ISOThresh {
  float alertRMS, dangerRMS;
};
const ISOThresh ISO_DEF[4] = {
  { 1.5f, 4.0f }, { 2.5f, 6.0f }, { 3.5f, 9.0f }, { 5.0f, 9.0f }
};

const uint32_t INTERVAL_OPTIONS[] = {
  1000, 2000, 5000, 10000, 30000, 60000
};
const uint8_t NUM_INTERVAL_OPTIONS = 6;

// ukuran queue rtos
#define DISPLAY_QUEUE_SIZE 10
#define SD_QUEUE_SIZE      10
#define ALERT_QUEUE_SIZE   10
#define UI_QUEUE_SIZE      10

#endif // CONFIG_H
