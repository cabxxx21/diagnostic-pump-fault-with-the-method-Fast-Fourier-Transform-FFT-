//types.h
#ifndef TYPES_H
#define TYPES_H

#include "config.h"

// === enums ===

// state sistem
enum State : uint8_t {
  ST_INIT, 
  ST_COLD_RECOVERY, 
  ST_INPUT_PARAMS, 
  ST_BOOT_DELAY,
  ST_STABILIZATION, 
  ST_BASELINE_CAPTURE, 
  ST_BASELINE_VALIDATE,
  ST_RUNNING, 
  ST_STANDBY, 
  ST_ALERT, 
  ST_DANGER
};

// status fault
enum Fault : uint8_t {
  F_NONE, 
  F_UNBALANCE, 
  F_MISALIGNMENT_ANGULAR,
  F_MISALIGNMENT_PARALLEL, 
  F_LOOSENESS, 
  F_MOTOR_STOPPED
};

enum AxisRole { 
  ROLE_H, 
  ROLE_V, 
  ROLE_A 
};

// command sd card
enum SDCmd {
  SD_CMD_DATA, 
  SD_CMD_SET_INTERVAL, 
  SD_CMD_START_LOG,
  SD_CMD_STOP_LOG, 
  SD_CMD_FORCE_NTP
};

// tipe alert
enum AlertType : uint8_t {
  AT_FAULT_DETECTED, 
  AT_FAULT_CHANGED, 
  AT_BACK_TO_NORMAL, 
  AT_MOTOR_STOPPED
};

// command ui
enum UICmd : uint8_t { 
  UI_CMD_NONE, 
  UI_CMD_SET_PARAMS, 
  UI_CMD_START_BOOT,
  UI_CMD_START_CAPTURE, 
  UI_CMD_REBASELINE, 
  UI_CMD_TOGGLE_AXIS_MAP,
  UI_CMD_TOGGLE_MATLAB, 
  UI_CMD_SD_START_LOG, 
  UI_CMD_SD_STOP_LOG,
  UI_CMD_SD_SET_INTERVAL, 
  UI_CMD_SD_SET_AUTO, 
  UI_CMD_RESET_EEPROM,
  UI_CMD_FORCE_NTP,
  UI_CMD_TOGGLE_THEME 
};

enum BtnEvent { 
  BTN_NONE, 
  BTN_UP, 
  BTN_DOWN, 
  BTN_SELECT 
};

// screen ui
enum UIScreen : uint8_t {
  SCREEN_LIVE,
  SCREEN_MENU_PARAMS,
  SCREEN_MENU_RUNTIME,
  SCREEN_MENU_SD,
  SCREEN_MENU_NETWORK
};

extern UIScreen currentScreen;

enum NavMode { 
  NAV_BROWSE, 
  NAV_EDIT 
};

// status jaringan
enum NetState : uint8_t { 
  NS_OFF, 
  NS_CONNECTING, 
  NS_SYNCING, 
  NS_SENDING_ALERTS 
};

// === structs ===

struct DisplayPacket {
  uint8_t sysState;
  uint8_t confirmedFault;
  float motorRPM;
  uint8_t isoClass;
  
  // data radial
  float liveRMS_Rad;
  float live1X_Rad;
  float live2X_Rad;
  float live3X_Rad;
  
  // data axial
  float liveRMS_Ax;
  float live1X_Ax;
  float live2X_Ax;
  float live3X_Ax;
  
  // suhu
  float tempObj;
  float tempAmb;
  
  // info sistem
  unsigned long bootElapsed;
  unsigned long bootTotal;
  int capIdx;
  int baselineN;
  uint8_t axisMapMode;
  bool matlabMode;
  
  // info sd
  bool sdPresent;
  bool sdLogging;
  bool sdAutoLogStat;
  uint32_t sdLogIntervalMs;
  char sdFileName[32];
  uint32_t sdRowsWritten;
  
  // info network
  uint8_t wifiState;
  bool ntpSynced;
  char lastSyncStr[20];
  uint8_t lastAlertStatus;
};

struct SDPacket {
  SDCmd cmd;
  unsigned long timestamp;
  uint8_t sysState;
  uint8_t confirmedFault;
  float motorRPM;
  uint8_t isoClass;
  float liveRMS_Rad;
  float liveRMS_Ax;
  float tempObj;
  float tempAmb;
  uint32_t logIntervalMs;
};

struct AlertPacket {
  AlertType type;
  uint8_t confirmedFault;
  uint8_t sysState;
  float motorRPM;
  uint8_t isoClass;
  
  float liveRMS_Rad;
  float liveRMS_Ax;
  float live1X_Rad;
  float live1X_Ax;
  float live2X_Rad;
  float live2X_Ax;
  float live3X_Rad;
  float live3X_Ax;
  
  float tempObj;
  float tempAmb;
};

struct UIPacket {
  UICmd cmd;
  float floatValue;
  uint8_t byteValue;
  uint32_t longValue;
};

#pragma pack(push, 1)
struct EepromData {
  uint32_t magic;
  float motorRPM;
  uint8_t isoClass;
  uint8_t axisMapCfg;
  
  // baseline rms
  float blRMS_Rad;
  float blRMS_Ax;
  
  // baseline harmonic
  float bl1X_Rad;
  float bl1X_Ax;
  float bl2X_Rad;
  float bl2X_Ax;
  float bl3X_Rad;
  float bl3X_Ax;
  
  // threshold rms
  float thAlertRMS_Rad;
  float thDangerRMS_Rad;
  float thAlertRMS_Ax;
  float thDangerRMS_Ax;
  
  // threshold harmonic 1x
  float thAlert1X_Rad;
  float thDanger1X_Rad;
  float thAlert1X_Ax;
  float thDanger1X_Ax;
  
  // threshold harmonic 2x
  float thAlert2X_Rad;
  float thDanger2X_Rad;
  float thAlert2X_Ax;
  float thDanger2X_Ax;
  
  // threshold harmonic 3x
  float thAlert3X_Rad;
  float thDanger3X_Rad;
  float thAlert3X_Ax;
  float thDanger3X_Ax;
  
  // setingan lain
  bool baselineValid;
  uint32_t sdLogIntervalMs;
  uint8_t sdAutoLog;
  uint8_t currentThemeCfg;
  uint32_t crc;
};
#pragma pack(pop)

struct Button {
  uint8_t pin;
  bool lastRaw;
  bool stable;
  unsigned long lastDebounce;
  unsigned long pressStart;
  bool longFired;
};

#endif // TYPES_H