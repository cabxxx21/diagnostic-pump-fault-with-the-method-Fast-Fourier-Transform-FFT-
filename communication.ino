//communication.ino
#include "globals.h"

// === request data display ===

void requestDisplayLive() {
  DisplayPacket p;
  memset(&p, 0, sizeof(p));
  
  p.sysState = sysState; 
  p.confirmedFault = confirmedFault;
  p.motorRPM = motorRPM; 
  p.isoClass = isoClass;
  
  p.liveRMS_Rad = liveRMS_Rad; 
  p.live1X_Rad = live1X_Rad;
  p.live2X_Rad = live2X_Rad; 
  p.live3X_Rad = live3X_Rad;
  
  p.liveRMS_Ax = liveRMS_Ax; 
  p.live1X_Ax = live1X_Ax;
  p.live2X_Ax = live2X_Ax; 
  p.live3X_Ax = live3X_Ax;
  
  p.tempObj = tempObj; 
  p.tempAmb = tempAmb;
  
  p.bootElapsed = millis() - stateEnterMs; 
  p.bootTotal = BOOT_DELAY_MS;
  p.capIdx = capIdx; 
  p.baselineN = BASELINE_N;
  p.axisMapMode = axisMapMode; 
  p.matlabMode = matlabMode;
  
  taskENTER_CRITICAL(&sdMux);
  p.sdPresent = sdPresent; 
  p.sdLogging = sdLogging;
  p.sdLogIntervalMs = sdLogIntervalMs; 
  p.sdRowsWritten = sdRowsWritten;
  p.sdAutoLogStat = sdAutoLog;
  strncpy(p.sdFileName, sdFileName, sizeof(p.sdFileName));
  taskEXIT_CRITICAL(&sdMux);
  
  taskENTER_CRITICAL(&ntpMux);
  p.wifiState = wifiState; 
  p.ntpSynced = ntpSynced;
  strncpy(p.lastSyncStr, lastSyncStr, sizeof(p.lastSyncStr));
  
  p.lastAlertStatus = telegramEnabled ? (lastAlertOk ? 2 : 3) : 0;
  if (wifiState == 3) {
    p.lastAlertStatus = 1; 
  }
  taskEXIT_CRITICAL(&ntpMux);
  
  if (xDisplayQueue != NULL) {
    xQueueSend(xDisplayQueue, &p, 0);
  }
}

// === kirim data ke sd ===

void sendSDData() {
  SDPacket pkt;
  memset(&pkt, 0, sizeof(pkt));
  
  pkt.cmd = SD_CMD_DATA; 
  pkt.timestamp = millis();
  pkt.sysState = sysState; 
  pkt.confirmedFault = confirmedFault;
  pkt.motorRPM = motorRPM; 
  pkt.isoClass = isoClass;
  pkt.liveRMS_Rad = liveRMS_Rad; 
  pkt.liveRMS_Ax = liveRMS_Ax;
  pkt.tempObj = tempObj; 
  pkt.tempAmb = tempAmb;
  
  if (xSDQueue != NULL) {
    xQueueSend(xSDQueue, &pkt, 0);
  }
}

// === kirim command sd ===

void sendSDCommand(SDCmd cmd) {
  SDPacket pkt; 
  memset(&pkt, 0, sizeof(pkt));
  pkt.cmd = cmd; 
  
  if (xSDQueue != NULL) {
    xQueueSend(xSDQueue, &pkt, 0);
  }
}

// === set interval sd ===

void sendSDInterval(uint32_t interval) {
  SDPacket pkt; 
  memset(&pkt, 0, sizeof(pkt));
  
  pkt.cmd = SD_CMD_SET_INTERVAL; 
  pkt.logIntervalMs = interval;
  
  if (xSDQueue != NULL) {
    xQueueSend(xSDQueue, &pkt, 0);
  }
}

// === kirim command ui ===

void sendUICmd(UICmd cmd, float fv = 0, uint8_t bv = 0, uint32_t lv = 0) {
  UIPacket pkt; 
  memset(&pkt, 0, sizeof(pkt));
  
  pkt.cmd = cmd; 
  pkt.floatValue = fv; 
  pkt.byteValue = bv; 
  pkt.longValue = lv;
  
  if (xUIQueue != NULL) {
    xQueueSend(xUIQueue, &pkt, 0);
  }
}

// === proses command ui ===

void processUICommands() {
  UIPacket pkt;
  if (xUIQueue != NULL) {
    while (xQueueReceive(xUIQueue, &pkt, 0) == pdPASS) {
      switch (pkt.cmd) {
        
        case UI_CMD_SET_PARAMS:
          motorRPM = pkt.floatValue; 
          isoClass = pkt.byteValue;
          setISODefaults(); 
          eepromSave(); 
          break;
          
        case UI_CMD_START_BOOT: 
          changeState(ST_BOOT_DELAY); 
          break;
          
        case UI_CMD_START_CAPTURE:
          capIdx = 0; 
          captureTriggered = false;
          changeState(ST_BASELINE_CAPTURE); 
          break;
          
        case UI_CMD_REBASELINE:
          capIdx = 0; 
          captureTriggered = false;
          changeState(ST_INPUT_PARAMS); 
          break;
          
        case UI_CMD_TOGGLE_AXIS_MAP:
          axisMapMode = (axisMapMode == 0) ? 1 : 0;
          applyAxisMap(); 
          eepromSave(); 
          break;
          
        case UI_CMD_TOGGLE_MATLAB: 
          matlabMode = !matlabMode; 
          break;
          
        case UI_CMD_SD_START_LOG:
          isSdLoggingExpected = true; 
          sendSDCommand(SD_CMD_START_LOG); 
          break;
          
        case UI_CMD_SD_STOP_LOG:
          isSdLoggingExpected = false; 
          sendSDCommand(SD_CMD_STOP_LOG); 
          break;
          
        case UI_CMD_SD_SET_INTERVAL:
          sdLogIntervalMs = pkt.longValue; 
          currentIntervalIdx = pkt.byteValue;
          sendSDInterval(pkt.longValue); 
          eepromSave(); 
          break;
          
        case UI_CMD_SD_SET_AUTO: 
          sdAutoLog = pkt.byteValue; 
          eepromSave(); 
          break;
          
        case UI_CMD_RESET_EEPROM:
          baselineValid = false; 
          motorRPM = 0; 
          isoClass = 1;
          eepromSave(); 
          changeState(ST_INPUT_PARAMS); 
          break;
          
        case UI_CMD_FORCE_NTP: 
          sendSDCommand(SD_CMD_FORCE_NTP); 
          break;

        case UI_CMD_TOGGLE_THEME: 
          currentTheme = (currentTheme == 0) ? 1 : 0; 
          eepromSave(); 
          break;
          
        default: 
          break;
      }
    }
  }
}