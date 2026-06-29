// ======================================================================
//  PUMP FAULT DIAGNOSTIC SYSTEM v6.0 — MAIN FILE
// ======================================================================
#include "globals.h"

//spi global
SPIClass spiHSPI(HSPI);  // spicllas hspi untuk lcd&tft
SPIClass spiFSPI(FSPI);  

// flag sinkronisasi
volatile bool sdInitDone = false; 
volatile bool lcdNeedsReinit = false;

//spi class 
extern SPIClass spiFSPI;  // Dedicated untuk ADXL345
extern SPIClass spiSD;

SemaphoreHandle_t xSPIMutex = NULL;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

bool mlxOK = false;
float tempObj = 0.0f, tempAmb = 0.0f;

// ui & rtos 
UIScreen currentScreen = SCREEN_LIVE;
portMUX_TYPE sdMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE ntpMux = portMUX_INITIALIZER_UNLOCKED;

QueueHandle_t xDisplayQueue = NULL, xSDQueue = NULL, xAlertQueue = NULL, xUIQueue = NULL;
TaskHandle_t fftTaskHandle = NULL, loggerTaskHandle = NULL, uiTaskHandle = NULL;

// state sistem
State sysState = ST_INIT;
Fault fault = F_NONE, confirmedFault = F_NONE;
unsigned long stateEnterMs = 0, lastStabCheckMs = 0, lastStandbyMs = 0, runningEnterMs = 0;
float motorRPM = 0.0f;
uint8_t isoClass = 1;
bool matlabMode = false;
volatile bool lcdInitialized = false;

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
bool acqComplete = false, isAcquiring = false;
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

// history
float stabPrevRMS = 0.0f;
int stabStableCount = 0;
float smooth_ub = 0.0f, smooth_am = 0.0f, smooth_pm = 0.0f, smooth_lo = 0.0f;
Fault faultHistory[FAULT_HISTORY_SIZE] = { F_NONE };
uint8_t historyIdx = 0;

// map axis
AxisRole mapX = ROLE_H, mapY = ROLE_A, mapZ = ROLE_V;
uint8_t axisMapMode = 0;

uint8_t currentTheme = 0;

// sd card
bool sdPresent = false, sdLogging = false, sdInitialized = false, isSdLoggingExpected = false;
uint32_t sdLogIntervalMs = 5000, sdRowsWritten = 0;
uint8_t sdAutoLog = 1;
char sdFileName[32] = "";
uint8_t currentIntervalIdx = 2;
int sdInitAttempts = 0;

// wifi & ntp
uint8_t wifiState = 0;
bool ntpSynced = false, forceNtpSyncFlag = false, telegramEnabled = true, lastAlertOk = false;
uint8_t lastAlertedFault = 255;
char lastSyncStr[20] = "Never";

// tombol
Button btnUp = {PIN_UP, HIGH, HIGH, 0, 0, false};
Button btnDown = {PIN_DOWN, HIGH, HIGH, 0, 0, false};
Button btnOk = {PIN_SELECT, HIGH, HIGH, 0, 0, false};


void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("--- SYSTEM BOOTSTART ---"));

  xSPIMutex = xSemaphoreCreateMutex();
  if (xSPIMutex == NULL) {
    Serial.println(F("[FATAL] Failed allocation xSPIMutex! Abort."));
    while(1);
  }
  
  EEPROM.begin(EEPROM_SIZE);

  //init group
  tft.init();
  Serial.println(F("[INIT] LCD ON-LINE!"));
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK); 
  
  pinMode(SD_CS, OUTPUT); 
  digitalWrite(SD_CS, HIGH);

  if (SD.begin(SD_CS, tft.getSPIinstance())) { 
    sdPresent = true; 
    sdInitialized = true;
    Serial.println(F("[INIT] SD Card OK on HSPI!"));
  } else {
    sdPresent = false;
    sdInitialized = false;
    Serial.println(F("[INIT] Failed mount SD Card!"));
  }
  
  sdInitDone = true; 

  spiFSPI.begin(ADXL_SCK, ADXL_MISO, ADXL_MOSI, -1);
  
  pinMode(ADXL_CS, OUTPUT); 
  digitalWrite(ADXL_CS, HIGH);
  
  if (adxl_readRegister(0x00) != 0xE5) { 
    Serial.println(F("[INIT] ADXL FAILED! Check Wiring!.")); 
  } else {
    Serial.println(F("[INIT] ADXL345 OK on FSPI!"));
  }
  adxl_writeRegister(0x31, 0x0B);
  adxl_writeRegister(0x2C, 0x0F);
  adxl_writeRegister(0x2D, 0x08);

  // init sensor suhu mlx
  Wire.begin(MLX_SDA, MLX_SCL);
  if (mlx.begin()) { 
    mlxOK = true; 
    Serial.println(F("[INIT] MLX90614 OK")); 
  } else {
    Serial.println(F("[INIT] MLX90614 FAILED!"));
  }

  // init pin tombol
  pinMode(PIN_UP, INPUT_PULLUP); 
  pinMode(PIN_SELECT, INPUT_PULLUP); 
  pinMode(PIN_DOWN, INPUT_PULLUP);

  // alokasi queue
  xDisplayQueue = xQueueCreate(DISPLAY_QUEUE_SIZE, sizeof(DisplayPacket));
  xSDQueue      = xQueueCreate(SD_QUEUE_SIZE, sizeof(SDPacket));
  xAlertQueue   = xQueueCreate(ALERT_QUEUE_SIZE, sizeof(AlertPacket));
  xUIQueue      = xQueueCreate(UI_QUEUE_SIZE, sizeof(UIPacket));

  if (xDisplayQueue == NULL || xSDQueue == NULL || xAlertQueue == NULL || xUIQueue == NULL) {
      Serial.println(F("[FATAL] Gagal membuat Queue! HEAP PENUH."));
      while(1);
  }

  // config wdt
  esp_task_wdt_config_t wdt_config = { .timeout_ms = 30000, .idle_core_mask = 0, .trigger_panic = false };
  esp_task_wdt_reconfigure(&wdt_config);

  // load eeprom
  bool eepromValid = eepromLoad();
  if (eepromValid && baselineValid) {
    sysState = ST_RUNNING;
    stateEnterMs = millis();
    runningEnterMs = millis();
    Serial.println(F("[SYSTEM] Baseline valid > langsung ST_RUNNING"));
    Serial.printf("[SYSTEM] RPM=%.0f ISO=%d blRMS_Rad=%.3f blRMS_Ax=%.3f\n",
                  motorRPM, isoClass, blRMS_Rad, blRMS_Ax);
  } else {
    sysState = ST_INPUT_PARAMS;
    Serial.println(F("[EEPROM] Baseline Belum Ada > ST_INPUT_PARAMS"));
  }
  
  if (!eepromValid) {
    setISODefaults();
  }

  // pembagian task rtos
  xTaskCreatePinnedToCore(TaskFFT_Code,    "FFT",    12000, NULL, 3, &fftTaskHandle, 0);
  xTaskCreatePinnedToCore(TaskMLX_Code,    "MLX",    4000,  NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(TaskLogger_Code, "Logger", 10000, NULL, 1, &loggerTaskHandle, 0);
  xTaskCreatePinnedToCore(TaskUI_Code,      "UI",    8000,  NULL, 2, &uiTaskHandle, 1);
}

void loop() { 
  vTaskDelete(NULL); 
}