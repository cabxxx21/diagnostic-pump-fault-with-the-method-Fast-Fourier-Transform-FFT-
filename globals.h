//globals.h
#ifndef GLOBALS_H
#define GLOBALS_H

#include "types.h"

// === spi & sensor objects ===

extern SPIClass* hspiADXL;
extern SPIClass spiSD;
extern SemaphoreHandle_t xSPIMutex;
extern Adafruit_MLX90614 mlx;

extern bool mlxOK;
extern float tempObj, tempAmb;

extern TFT_eSPI tft;
extern TFT_eSprite spr;
extern TFT_eSprite sprMenu;

// === rtos handles & mutex ===

extern portMUX_TYPE sdMux;
extern portMUX_TYPE ntpMux;

extern QueueHandle_t xDisplayQueue;
extern QueueHandle_t xSDQueue;
extern QueueHandle_t xAlertQueue;
extern QueueHandle_t xUIQueue;

extern TaskHandle_t fftTaskHandle;
extern TaskHandle_t loggerTaskHandle;
extern TaskHandle_t uiTaskHandle;

// === state & fault globals ===

extern State sysState;
extern Fault fault;
extern Fault confirmedFault;

extern unsigned long stateEnterMs;
extern unsigned long lastStabCheckMs;
extern unsigned long lastStandbyMs;
extern unsigned long runningEnterMs;

extern float motorRPM;
extern uint8_t isoClass;
extern bool matlabMode;
extern volatile bool lcdInitialized;

// === fft & signal globals ===

extern double vRealX[SAMPLES];
extern double vRealY[SAMPLES];
extern double vRealZ[SAMPLES];
extern double vImag[SAMPLES];

extern ArduinoFFT<double> FFT_X;
extern ArduinoFFT<double> FFT_Y;
extern ArduinoFFT<double> FFT_Z;

extern float plotSpecR[SAMPLES / 2];
extern float plotSpecA[SAMPLES / 2];

extern float liveRMS_Rad, live1X_Rad, live2X_Rad, live3X_Rad, live4X_Rad, live5X_Rad;
extern float liveRMS_Ax, live1X_Ax, live2X_Ax, live3X_Ax, live4X_Ax, live5X_Ax;

extern int sampleIdx;
extern bool acqComplete;
extern bool isAcquiring;
extern unsigned long lastSampleUs;

extern double acqSumX, acqSumX2;
extern double acqSumY, acqSumY2;
extern double acqSumZ, acqSumZ2;

// === baseline & threshold globals ===

extern float blRMS_Rad, blRMS_Ax;
extern float bl1X_Rad, bl1X_Ax;
extern float bl2X_Rad, bl2X_Ax;
extern float bl3X_Rad, bl3X_Ax;

extern float thAlertRMS_Rad, thDangerRMS_Rad;
extern float thAlertRMS_Ax, thDangerRMS_Ax;

extern float thAlert1X_Rad, thDanger1X_Rad;
extern float thAlert1X_Ax, thDanger1X_Ax;

extern float thAlert2X_Rad, thDanger2X_Rad;
extern float thAlert2X_Ax, thDanger2X_Ax;

extern float thAlert3X_Rad, thDanger3X_Rad;
extern float thAlert3X_Ax, thDanger3X_Ax;

extern bool baselineValid;

extern float capRMS_Rad[BASELINE_N];
extern float cap1X_Rad[BASELINE_N];
extern float cap2X_Rad[BASELINE_N];
extern float cap3X_Rad[BASELINE_N];

extern float capRMS_Ax[BASELINE_N];
extern float cap1X_Ax[BASELINE_N];
extern float cap2X_Ax[BASELINE_N];
extern float cap3X_Ax[BASELINE_N];

extern int capIdx;
extern bool captureTriggered;

// === diagnosis globals ===

extern float stabPrevRMS;
extern int stabStableCount;
extern float smooth_ub, smooth_am, smooth_pm, smooth_lo;

extern Fault faultHistory[FAULT_HISTORY_SIZE];
extern uint8_t historyIdx;

// === axis mapping globals ===

extern AxisRole mapX, mapY, mapZ;
extern uint8_t axisMapMode;

// === sd, wifi, ntp, telegram globals ===

extern bool sdPresent;
extern bool sdLogging;
extern bool sdInitialized;
extern bool isSdLoggingExpected;

extern uint8_t currentTheme; // 0 = Dark, 1 = Light

extern uint32_t sdLogIntervalMs;
extern uint32_t sdRowsWritten;
extern uint8_t sdAutoLog;
extern char sdFileName[32];
extern uint8_t currentIntervalIdx;
extern int sdInitAttempts;

extern uint8_t wifiState;
extern bool ntpSynced;
extern bool forceNtpSyncFlag;
extern bool telegramEnabled;
extern bool lastAlertOk;
extern uint8_t lastAlertedFault;
extern char lastSyncStr[20];

#endif // GLOBALS_H