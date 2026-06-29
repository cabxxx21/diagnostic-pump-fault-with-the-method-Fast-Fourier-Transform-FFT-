// main file - pump fault diagnostic system v6.0

#include "globals.h"

// konfigurasi spi
SPIClass spiHSPI(HSPI);  // tft & sd card
SPIClass spiFSPI(FSPI);  // adxl345
extern SPIClass spiSD;

SemaphoreHandle_t xSPIMutex = NULL;

// sensor
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// flag sinkronisasi
volatile bool sdInitDone = false; 
volatile bool lcdNeedsReinit = false;
volatile bool lcdInitialized = false;

// variabel sensor
bool mlxOK = false;
float tempObj = 0.0f;
float tempAmb = 0.0f;

// monitoring cpu
volatile uint32_t idleCountCore0 = 0;
volatile uint32_t idleCountCore1 = 0;

// ui & rtos
UIScreen currentScreen = SCREEN_LIVE;
uint8_t currentTheme = 0; // default dark 1 for light 

portMUX_TYPE sdMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE ntpMux = portMUX_INITIALIZER_UNLOCKED;

QueueHandle_t xDisplayQueue = NULL;
QueueHandle_t xSDQueue = NULL;
QueueHandle_t xAlertQueue = NULL;
QueueHandle_t xUIQueue = NULL;

TaskHandle_t fftTaskHandle = NULL;
TaskHandle_t loggerTaskHandle = NULL;
TaskHandle_t uiTaskHandle = NULL;

// state & metrik sistem
State sysState = ST_INIT;
Fault fault = F_NONE;
Fault confirmedFault = F_NONE;

unsigned long stateEnterMs = 0;
unsigned long lastStabCheckMs = 0;
unsigned long lastStandbyMs = 0;
unsigned long runningEnterMs = 0;

float motorRPM = 0.0f;
uint8_t isoClass = 1;
bool matlabMode = false;

// variabel fft
double vRealX[SAMPLES], vRealY[SAMPLES], vRealZ[SAMPLES], vImag[SAMPLES];
ArduinoFFT<double> FFT_X = ArduinoFFT<double>(vRealX, vImag, SAMPLES, SAMPLING_FREQ);
ArduinoFFT<double> FFT_Y = ArduinoFFT<double>(vRealY, vImag, SAMPLES, SAMPLING_FREQ);
ArduinoFFT<double> FFT_Z = ArduinoFFT<double>(vRealZ, vImag, SAMPLES, SAMPLING_FREQ);
float plotSpecR[SAMPLES / 2], plotSpecA[SAMPLES / 2];

// data akuisisi
float liveRMS_Rad = 0, live1X_Rad = 0, live2X_Rad = 0, live3X_Rad = 0, live4X_Rad = 0, live5X_Rad = 0;
float liveRMS_Ax = 0, live1X_Ax = 0, live2X_Ax = 0, live3X_Ax = 0, live4X_Ax = 0, live5X_Ax = 0;

int sampleIdx = 0;
bool acqComplete = false;
bool isAcquiring = false;
unsigned long lastSampleUs = 0;
double acqSumX = 0, acqSumX2 = 0, acqSumY = 0, acqSumY2 = 0, acqSumZ = 0, acqSumZ2 = 0;

// baseline & threshold
float blRMS_Rad, blRMS_Ax, bl1X_Rad, bl1X_Ax, bl2X_Rad, bl2X_Ax, bl3X_Rad, bl3X_Ax;
float thAlertRMS_Rad, thDangerRMS_Rad, thAlertRMS_Ax, thDangerRMS_Ax;
float thAlert1X_Rad, thDanger1X_Rad, thAlert1X_Ax, thDanger1X_Ax;
float thAlert2X_Rad, thDanger2X_Rad, thAlert2X_Ax, thDanger2X_Ax;
float thAlert3X_Rad, thDanger3X_Rad, thAlert3X_Ax, thDanger3X_Ax;

bool baselineValid = false;
float capRMS_Rad[BASELINE_N], cap1X_Rad[BASELINE_N], cap2X_Rad[BASELINE_N], cap3X_Rad[BASELINE_N];
float capRMS_Ax[BASELINE_N], cap1X_Ax[BASELINE_N], cap2X_Ax[BASELINE_N], cap3X_Ax[BASELINE_N];
int capIdx = 0;
bool captureTriggered = false;

// riwayat & stabilitas
float stabPrevRMS = 0.0f;
int stabStableCount = 0;
float smooth_ub = 0.0f, smooth_am = 0.0f, smooth_pm = 0.0f, smooth_lo = 0.0f;
Fault faultHistory[FAULT_HISTORY_SIZE] = { F_NONE };
uint8_t historyIdx = 0;

// mapping axis
AxisRole mapX = ROLE_H;
AxisRole mapY = ROLE_A;
AxisRole mapZ = ROLE_V;
uint8_t axisMapMode = 0;

// sd card logging
bool sdPresent = false;
bool sdLogging = false;
bool sdInitialized = false;
bool isSdLoggingExpected = false;
uint32_t sdLogIntervalMs = 5000;
uint32_t sdRowsWritten = 0;
uint8_t sdAutoLog = 1;
char sdFileName[32] = "";
uint8_t currentIntervalIdx = 2;
int sdInitAttempts = 0;

// wifi & ntp
uint8_t wifiState = 0;
bool ntpSynced = false;
bool forceNtpSyncFlag = false;
bool telegramEnabled = true;
bool lastAlertOk = false;
uint8_t lastAlertedFault = 255;
char lastSyncStr[20] = "Never";

// pin tombol
Button btnUp = {PIN_UP, HIGH, HIGH, 0, 0, false};
Button btnDown = {PIN_DOWN, HIGH, HIGH, 0, 0, false};
Button btnOk = {PIN_SELECT, HIGH, HIGH, 0, 0, false};

void setup() {
  Serial.begin(115200);
  delay(500);

  xSPIMutex = xSemaphoreCreateMutex();
  if (xSPIMutex == NULL) {
    Serial.println(F("[FATAL] Gagal alokasi xSPIMutex! Halt."));
    while(1);
  }
  
  EEPROM.begin(EEPROM_SIZE);
  
  // 1. init tft di hspi
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK); 
  
  // 2. init sd card
  Serial.println(F("[SD] Initializing SD Card on HSPI..."));
  
  pinMode(SD_CS, OUTPUT); 
  digitalWrite(SD_CS, HIGH); // deselect sd card

  if (SD.begin(SD_CS, tft.getSPIinstance())) { 
    sdPresent = true; 
    sdInitialized = true;
    Serial.println(F("[SD] SD Card Siap!"));
  } else {
    sdPresent = false;
    sdInitialized = false;
    Serial.println(F("[SD] Gagal mount SD Card!"));
  }
  sdInitDone = true; 

  // 3. init adxl in fspi
  spiFSPI.begin(ADXL_SCK, ADXL_MISO, ADXL_MOSI, -1);
  
  pinMode(ADXL_CS, OUTPUT); 
  digitalWrite(ADXL_CS, HIGH);
  
  if (adxl_readRegister(0x00) != 0xE5) { 
    Serial.println(F("[ADXL] GAGAL! Cek wiring.")); 
  } else {
    Serial.println(F("[ADXL] ADXL345 OK on FSPI!"));
  }
  
  adxl_writeRegister(0x31, 0x0B);
  adxl_writeRegister(0x2C, 0x0F);
  adxl_writeRegister(0x2D, 0x08);

  // 4. setup pin
  pinMode(PIN_UP, INPUT_PULLUP); 
  pinMode(PIN_SELECT, INPUT_PULLUP); 
  pinMode(PIN_DOWN, INPUT_PULLUP);

  // 5. alokasi queue rtos
  xDisplayQueue = xQueueCreate(DISPLAY_QUEUE_SIZE, sizeof(DisplayPacket));
  xSDQueue = xQueueCreate(SD_QUEUE_SIZE, sizeof(SDPacket));
  xAlertQueue = xQueueCreate(ALERT_QUEUE_SIZE, sizeof(AlertPacket));
  xUIQueue = xQueueCreate(UI_QUEUE_SIZE, sizeof(UIPacket));

  if (xDisplayQueue == NULL || xSDQueue == NULL || xAlertQueue == NULL || xUIQueue == NULL) {
      Serial.println(F("[FATAL] Gagal membuat Queue! HEAP PENUH."));
      while(1);
  }

  // 6. config sistem & boot
  esp_task_wdt_config_t wdt_config = { .timeout_ms = 30000, .idle_core_mask = 0, .trigger_panic = false };
  esp_task_wdt_reconfigure(&wdt_config);

  bool eepromValid = eepromLoad();
  if (eepromValid && baselineValid) {
    sysState = ST_RUNNING;
    stateEnterMs = millis();
    runningEnterMs = millis();
    Serial.println(F("[BOOT] Baseline valid -> ST_RUNNING"));
    Serial.printf("[BOOT] RPM=%.0f ISO=%d blRMS_Rad=%.3f blRMS_Ax=%.3f\n", motorRPM, isoClass, blRMS_Rad, blRMS_Ax);
  } else {
    sysState = ST_INPUT_PARAMS;
    Serial.println(F("[BOOT] Baseline belum ada -> ST_INPUT_PARAMS"));
  }
  
  if (!eepromValid) {
    setISODefaults();
  }

  // 7. sensor i2c (mlx90614)
  Wire.begin(MLX_SDA, MLX_SCL);
  if (mlx.begin()) { 
    mlxOK = true; 
    Serial.println(F("[MLX] OK")); 
  } else {
    Serial.println(F("[MLX] Gagal!"));
  }

  // 8. bikin task rtos
  xTaskCreatePinnedToCore(TaskFFT_Code, "FFT", 12000, NULL, 3, &fftTaskHandle, 0);
  xTaskCreatePinnedToCore(TaskMLX_Code, "MLX", 4000, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(TaskLogger_Code, "Logger", 10000, NULL, 1, &loggerTaskHandle, 0);
  xTaskCreatePinnedToCore(TaskUI_Code, "UI", 8000, NULL, 2, &uiTaskHandle, 1);
}

void loop() { 
  vTaskDelete(NULL); 
}