//eeprom_manager.ino
#include "globals.h"

// === inisialisasi default iso ===

void setISODefaults() {
  uint8_t idx = constrain(isoClass, 1, 4) - 1;
  
  thAlertRMS_Rad = thAlertRMS_Ax = ISO_DEF[idx].alertRMS;
  thDangerRMS_Rad = thDangerRMS_Ax = ISO_DEF[idx].dangerRMS;
  
  thAlert1X_Rad = thAlert1X_Ax = ISO_DEF[idx].alertRMS * 0.5f;
  thAlert2X_Rad = thAlert2X_Ax = ISO_DEF[idx].alertRMS * 0.4f;
  thAlert3X_Rad = thAlert3X_Ax = ISO_DEF[idx].alertRMS * 0.3f;
}

// === kalkulasi ulang batas threshold ===

void recalcThresholds() {
  uint8_t idx = constrain(isoClass, 1, 4) - 1;
  
  thAlertRMS_Rad = max(blRMS_Rad * ALERT_RMS_M, ISO_DEF[idx].alertRMS * 0.5f);
  thDangerRMS_Rad = max(blRMS_Rad * DANGER_RMS_M, ISO_DEF[idx].dangerRMS * 0.5f);
  
  thAlertRMS_Ax = max(blRMS_Ax * ALERT_RMS_M, ISO_DEF[idx].alertRMS * 0.3f);
  thDangerRMS_Ax = max(blRMS_Ax * DANGER_RMS_M, ISO_DEF[idx].dangerRMS * 0.3f);
  
  float fl1 = 0.10f;
  float fl2 = 0.08f;
  float fl3 = 0.05f;
  
  thAlert1X_Rad = max(bl1X_Rad * ALERT_1X_M, fl1);
  thDanger1X_Rad = max(bl1X_Rad * DANGER_1X_M, fl1 * 2);
  
  thAlert1X_Ax = max(bl1X_Ax * ALERT_1X_M, fl1);
  thDanger1X_Ax = max(bl1X_Ax * DANGER_1X_M, fl1 * 2);
  
  thAlert2X_Rad = max(bl2X_Rad * ALERT_2X_M, fl2);
  thDanger2X_Rad = max(bl2X_Rad * DANGER_2X_M, fl2 * 2);
  
  thAlert2X_Ax = max(bl2X_Ax * ALERT_2X_M, fl2);
  thDanger2X_Ax = max(bl2X_Ax * DANGER_2X_M, fl2 * 2);
  
  thAlert3X_Rad = max(bl3X_Rad * ALERT_3X_M, fl3);
  thDanger3X_Rad = max(bl3X_Rad * DANGER_3X_M, fl3 * 2);
  
  thAlert3X_Ax = max(bl3X_Ax * ALERT_3X_M, fl3);
  thDanger3X_Ax = max(bl3X_Ax * DANGER_3X_M, fl3 * 2);
}

// === simpan data ke eeprom ===

void eepromSave() {
  EepromData d;
  memset(&d, 0, sizeof(d));
  d.magic = EEPROM_MAGIC;
  d.motorRPM = motorRPM;
  d.isoClass = isoClass;
  d.axisMapCfg = axisMapMode;
  
  d.blRMS_Rad = blRMS_Rad; 
  d.blRMS_Ax = blRMS_Ax;
  d.bl1X_Rad = bl1X_Rad; 
  d.bl1X_Ax = bl1X_Ax;
  d.bl2X_Rad = bl2X_Rad; 
  d.bl2X_Ax = bl2X_Ax;
  d.bl3X_Rad = bl3X_Rad; 
  d.bl3X_Ax = bl3X_Ax;
  
  d.thAlertRMS_Rad = thAlertRMS_Rad; 
  d.thDangerRMS_Rad = thDangerRMS_Rad;
  d.thAlertRMS_Ax = thAlertRMS_Ax; 
  d.thDangerRMS_Ax = thDangerRMS_Ax;
  
  d.thAlert1X_Rad = thAlert1X_Rad; 
  d.thDanger1X_Rad = thDanger1X_Rad;
  d.thAlert1X_Ax = thAlert1X_Ax; 
  d.thDanger1X_Ax = thDanger1X_Ax;
  
  d.thAlert2X_Rad = thAlert2X_Rad; 
  d.thDanger2X_Rad = thDanger2X_Rad;
  d.thAlert2X_Ax = thAlert2X_Ax; 
  d.thDanger2X_Ax = thDanger2X_Ax;
  
  d.thAlert3X_Rad = thAlert3X_Rad; 
  d.thDanger3X_Rad = thDanger3X_Rad;
  d.thAlert3X_Ax = thAlert3X_Ax; 
  d.thDanger3X_Ax = thDanger3X_Ax;
  
  d.baselineValid = baselineValid;
  d.sdLogIntervalMs = sdLogIntervalMs;
  d.sdAutoLog = sdAutoLog;
  d.currentThemeCfg = currentTheme;
  
  d.crc = crc32((uint8_t *)&d, offsetof(EepromData, crc));
  
  EEPROM.put(0, d);
  EEPROM.commit();
}

// === baca data dari eeprom ===

bool eepromLoad() {
  EepromData d;
  EEPROM.get(0, d); 
  
  if (d.magic != EEPROM_MAGIC || crc32((uint8_t *)&d, offsetof(EepromData, crc)) != d.crc) {
    currentTheme = 0; //1 1 untuk light jika eeprom kosong 
    return false;
  }
  
  motorRPM = d.motorRPM; 
  isoClass = d.isoClass; 
  axisMapMode = d.axisMapCfg;
  
  applyAxisMap();
  
  blRMS_Rad = d.blRMS_Rad; 
  blRMS_Ax = d.blRMS_Ax;
  bl1X_Rad = d.bl1X_Rad; 
  bl1X_Ax = d.bl1X_Ax;
  bl2X_Rad = d.bl2X_Rad; 
  bl2X_Ax = d.bl2X_Ax;
  bl3X_Rad = d.bl3X_Rad; 
  bl3X_Ax = d.bl3X_Ax;
  
  thAlertRMS_Rad = d.thAlertRMS_Rad; 
  thDangerRMS_Rad = d.thDangerRMS_Rad;
  thAlertRMS_Ax = d.thAlertRMS_Ax; 
  thDangerRMS_Ax = d.thDangerRMS_Ax;
  
  thAlert1X_Rad = d.thAlert1X_Rad; 
  thDanger1X_Rad = d.thDanger1X_Rad;
  thAlert1X_Ax = d.thAlert1X_Ax; 
  thDanger1X_Ax = d.thDanger1X_Ax;
  
  thAlert2X_Rad = d.thAlert2X_Rad; 
  thDanger2X_Rad = d.thDanger2X_Rad;
  thAlert2X_Ax = d.thAlert2X_Ax; 
  thDanger2X_Ax = d.thDanger2X_Ax;
  
  thAlert3X_Rad = d.thAlert3X_Rad; 
  thDanger3X_Rad = d.thDanger3X_Rad;
  thAlert3X_Ax = d.thAlert3X_Ax; 
  thDanger3X_Ax = d.thDanger3X_Ax;
  
  baselineValid = d.baselineValid;
  
  if (d.sdLogIntervalMs > 0) {
    sdLogIntervalMs = d.sdLogIntervalMs;
  }
  
  sdAutoLog = d.sdAutoLog;
  currentTheme = d.currentThemeCfg; 
  
  currentIntervalIdx = 0;
  
  for (int i = 0; i < NUM_INTERVAL_OPTIONS; i++) {
    if (INTERVAL_OPTIONS[i] == sdLogIntervalMs) { 
      currentIntervalIdx = i; 
      break; 
    }
  }
  
  if (isnan(blRMS_Rad) || isnan(blRMS_Ax)) { 
    baselineValid = false; 
    return false; 
  }
  
  if (isnan(thAlertRMS_Rad) || isnan(thDangerRMS_Rad) || isnan(motorRPM)) { 
    setISODefaults(); 
  }
  
  return true;
}