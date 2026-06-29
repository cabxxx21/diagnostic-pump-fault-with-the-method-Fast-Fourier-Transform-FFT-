//state_machines.ino
#include "globals.h"

// === ganti state sistem ===

void changeState(State newState) {
  State prev = sysState;
  sysState = newState;
  stateEnterMs = millis();

  switch (newState) {
    case ST_BOOT_DELAY: 
      startAcquisition(); 
      break;
      
    case ST_STABILIZATION:
      stabStableCount = 0; 
      stabPrevRMS = 0;
      lastStabCheckMs = millis(); 
      startAcquisition(); 
      break;
      
    case ST_BASELINE_CAPTURE:
      capIdx = 0; 
      captureTriggered = false; 
      startAcquisition(); 
      break;
      
    case ST_BASELINE_VALIDATE: 
      break;
      
    case ST_RUNNING:
      runningEnterMs = millis(); 
      startAcquisition(); 
      break;
      
    case ST_STANDBY: 
      lastStandbyMs = millis(); 
      break;
      
    case ST_ALERT: 
      triggerAlert(AT_FAULT_DETECTED); 
      break;
      
    case ST_DANGER:
      if (prev != ST_DANGER) {
        triggerAlert(AT_FAULT_DETECTED); 
      }
      break;
      
    default: 
      break;
  }
}

// === trigger alert telegram ===

void triggerAlert(AlertType type) {
  if (!telegramEnabled) {
    return;
  }
  
  AlertPacket pkt; 
  memset(&pkt, 0, sizeof(pkt));
  
  pkt.type = type; 
  pkt.confirmedFault = confirmedFault;
  pkt.sysState = sysState; 
  pkt.motorRPM = motorRPM; 
  pkt.isoClass = isoClass;
  
  pkt.liveRMS_Rad = liveRMS_Rad; 
  pkt.liveRMS_Ax = liveRMS_Ax;
  
  pkt.live1X_Rad = live1X_Rad; 
  pkt.live1X_Ax = live1X_Ax;
  
  pkt.live2X_Rad = live2X_Rad; 
  pkt.live2X_Ax = live2X_Ax;
  
  pkt.live3X_Rad = live3X_Rad; 
  pkt.live3X_Ax = live3X_Ax;
  
  pkt.tempObj = tempObj; 
  pkt.tempAmb = tempAmb;
  
  xQueueSend(xAlertQueue, &pkt, 0);
}