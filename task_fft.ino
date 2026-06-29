//task_fft.ino
#include "globals.h"

// === helper & mapping ===

float freq1X() { return motorRPM / 60.0f; }
float freq2X() { return 2.0f * freq1X(); }
float freq3X() { return 3.0f * freq1X(); }
float freq4X() { return 4.0f * freq1X(); }
float freq5X() { return 5.0f * freq1X(); }

float movingAvg(float *arr, int len, int win) {
  int start = max(0, len - win);
  float s = 0.0f;
  for (int i = start; i < len; i++) {
    s += arr[i];
  }
  return s / (float)(len - start);
}

float stdDev(float *arr, int len) {
  if (len < 2) return 999.0f;
  float m = 0.0f;
  for (int i = 0; i < len; i++) {
    m += arr[i];
  }
  m /= len;
  
  float ss = 0.0f;
  for (int i = 0; i < len; i++) {
    ss += sq(arr[i] - m);
  }
  return sqrtf(ss / (len - 1));
}

uint32_t crc32(const uint8_t *d, size_t n) {
  uint32_t c = 0xFFFFFFFF;
  for (size_t i = 0; i < n; i++) {
    c ^= d[i];
    for (int j = 0; j < 8; j++) {
      c = (c & 1) ? (c >> 1) ^ 0xEDB88320UL : c >> 1;
    }
  }
  return ~c;
}

void applyAxisMap() {
  if (axisMapMode == 1) { 
    mapX = ROLE_A; mapY = ROLE_V; mapZ = ROLE_H; 
  } else { 
    mapX = ROLE_H; mapY = ROLE_A; mapZ = ROLE_V; 
  }
}

float mapToRadial(float x, float y, float z) {
  float h = (mapX == ROLE_H) ? x : (mapY == ROLE_H) ? y : z;
  float v = (mapX == ROLE_V) ? x : (mapY == ROLE_V) ? y : z;
  return max(h, v);
}

float mapToAxial(float x, float y, float z) {
  return (mapX == ROLE_A) ? x : (mapY == ROLE_A) ? y : z;
}

void forceLocalTime(int targetHour, int targetMin, int targetSec) {
  time_t now;
  time(&now);
  struct tm currentLocalTm;
  localtime_r(&now, &currentLocalTm);
  
  long currentSecs = currentLocalTm.tm_hour * 3600 + currentLocalTm.tm_min * 60 + currentLocalTm.tm_sec;
  long targetSecs = targetHour * 3600 + targetMin * 60 + targetSec;
  long diff = targetSecs - currentSecs;
  
  if (diff <= 0) diff += 86400;
  
  time_t targetEpoch = now + diff;
  struct timeval tv = { .tv_sec = targetEpoch, .tv_usec = 0 };
  settimeofday(&tv, NULL);
}

// === fft & akuisisi ===

void startAcquisition() {
  sampleIdx = 0;
  acqSumX = 0; acqSumX2 = 0;
  acqSumY = 0; acqSumY2 = 0;
  acqSumZ = 0; acqSumZ2 = 0;
  acqComplete = false;
  isAcquiring = true;
  lastSampleUs = micros();
}

bool runAcquisition() {
  const unsigned long dt = (unsigned long)(1000000.0 / SAMPLING_FREQ);
  int samplesRead = 0; 
  
  while (micros() - lastSampleUs >= dt && sampleIdx < SAMPLES && samplesRead < 10) {
    lastSampleUs += dt;
    float ax, ay, az;
    adxl_readAccel(&ax, &ay, &az);
    
    vRealX[sampleIdx] = ax; 
    acqSumX += ax; 
    acqSumX2 += ax * ax;
    
    vRealY[sampleIdx] = ay; 
    acqSumY += ay; 
    acqSumY2 += ay * ay;
    
    vRealZ[sampleIdx] = az; 
    acqSumZ += az; 
    acqSumZ2 += az * az;
    
    sampleIdx++;
    samplesRead++;
  }
  
  if (sampleIdx >= SAMPLES) {
    acqComplete = true;
    isAcquiring = false;
    return true;
  }
  return false;
}

float extractMag(double *vData, float targetFreq) {
  if (targetFreq < FREQ_RES || targetFreq >= SAMPLING_FREQ / 2.0f) return 0.0f;
  
  int bin = round(targetFreq / FREQ_RES);
  int lo = max(1, bin - BIN_TOLERANCE);
  int hi = min((SAMPLES / 2) - 1, bin + BIN_TOLERANCE);
  float peak = 0.0f;
  
  for (int i = lo; i <= hi; i++) {
    if ((float)vData[i] > peak) {
      peak = (float)vData[i];
    }
  }
  return peak;
}

void runFFTOnAxis(double *vDataTarget) {
  memset(vImag, 0, sizeof(vImag));
  ArduinoFFT<double> *pFFT = nullptr;
  
  if (vDataTarget == vRealX) pFFT = &FFT_X;
  else if (vDataTarget == vRealY) pFFT = &FFT_Y;
  else if (vDataTarget == vRealZ) pFFT = &FFT_Z;
  else return;

  pFFT->dcRemoval();
  pFFT->windowing(FFT_WIN_TYP_HANN, FFT_FORWARD);
  pFFT->compute(FFT_FORWARD);
  pFFT->complexToMagnitude();

  for (int i = 0; i < SAMPLES / 2; i++) {
    vDataTarget[i] *= FFT_NORM;
  }
}

void processFFT() {
  double varX = (acqSumX2 / SAMPLES) - sq(acqSumX / SAMPLES);
  double varY = (acqSumY2 / SAMPLES) - sq(acqSumY / SAMPLES);
  double varZ = (acqSumZ2 / SAMPLES) - sq(acqSumZ / SAMPLES);
  
  float rmsX = sqrtf(max(0.0, varX));
  float rmsY = sqrtf(max(0.0, varY));
  float rmsZ = sqrtf(max(0.0, varZ));
  
  liveRMS_Rad = mapToRadial(rmsX, rmsY, rmsZ);
  liveRMS_Ax = mapToAxial(rmsX, rmsY, rmsZ);

  float stopThreshold = max(blRMS_Rad * MOTOR_STOP_RATIO, MOTOR_STOP_FLOOR);
  bool isMotorStopped = (liveRMS_Rad < stopThreshold && liveRMS_Ax < stopThreshold);

  if (motorRPM <= 0.0f || isnan(motorRPM) || isMotorStopped) {
    live1X_Rad = 0; live2X_Rad = 0; live3X_Rad = 0; live4X_Rad = 0; live5X_Rad = 0;
    live1X_Ax = 0; live2X_Ax = 0; live3X_Ax = 0; live4X_Ax = 0; live5X_Ax = 0;
    return;
  }

  runFFTOnAxis(vRealX);
  float x1 = extractMag(vRealX, freq1X());
  float x2 = extractMag(vRealX, freq2X());
  float x3 = extractMag(vRealX, freq3X());
  float x4 = extractMag(vRealX, freq4X());
  float x5 = extractMag(vRealX, freq5X());
  
  runFFTOnAxis(vRealY);
  float y1 = extractMag(vRealY, freq1X());
  float y2 = extractMag(vRealY, freq2X());
  float y3 = extractMag(vRealY, freq3X());
  float y4 = extractMag(vRealY, freq4X());
  float y5 = extractMag(vRealY, freq5X());
  
  runFFTOnAxis(vRealZ);
  float z1 = extractMag(vRealZ, freq1X());
  float z2 = extractMag(vRealZ, freq2X());
  float z3 = extractMag(vRealZ, freq3X());
  float z4 = extractMag(vRealZ, freq4X());
  float z5 = extractMag(vRealZ, freq5X());

  live1X_Rad = mapToRadial(x1, y1, z1); 
  live2X_Rad = mapToRadial(x2, y2, z2); 
  live3X_Rad = mapToRadial(x3, y3, z3); 
  live4X_Rad = mapToRadial(x4, y4, z4); 
  live5X_Rad = mapToRadial(x5, y5, z5);
  
  live1X_Ax = mapToAxial(x1, y1, z1); 
  live2X_Ax = mapToAxial(x2, y2, z2); 
  live3X_Ax = mapToAxial(x3, y3, z3); 
  live4X_Ax = mapToAxial(x4, y4, z4); 
  live5X_Ax = mapToAxial(x5, y5, z5);

  for (int i = 0; i < SAMPLES / 2; i++) {
    float h = (mapX == ROLE_H) ? (float)vRealX[i] : (mapY == ROLE_H) ? (float)vRealY[i] : (float)vRealZ[i];
    float v = (mapX == ROLE_V) ? (float)vRealX[i] : (mapY == ROLE_V) ? (float)vRealY[i] : (float)vRealZ[i];
    plotSpecR[i] = max(h, v);
    plotSpecA[i] = (mapX == ROLE_A) ? (float)vRealX[i] : (mapY == ROLE_A) ? (float)vRealY[i] : (float)vRealZ[i];
  }
}

float quickRMS() {
  const int N = 50;
  const unsigned long dt = 1250;
  float sx2 = 0, sy2 = 0, sz2 = 0;
  float ax, ay, az;
  
  for (int i = 0; i < N; i++) {
    unsigned long t = micros();
    adxl_readAccel(&ax, &ay, &az);
    sx2 += sq(ax); 
    sy2 += sq(ay); 
    sz2 += sq(az);
    while (micros() - t < dt);
  }
  return max(sqrtf(sx2 / N), max(sqrtf(sy2 / N), sqrtf(sz2 / N)));
}

// === diagnosa fault ===

float calcUnbalanceScore() { 
  float s = 0.0f; 
  if (live1X_Rad > thAlert1X_Rad) {
    s += 50.0f * min(1.0f, (live1X_Rad - bl1X_Rad) / (thAlert1X_Rad - bl1X_Rad + 0.01f)); 
  }
  if (live1X_Rad > live1X_Ax * 1.5f) {
    s += 30.0f; 
  }
  if (live2X_Rad < live1X_Rad * 0.5f && live3X_Rad < live1X_Rad * 0.3f) {
    s += 20.0f; 
  }
  return s; 
}

float calcAngularMisalignScore() { 
  float s = 0.0f; 
  if (live1X_Ax > thAlert1X_Ax) s += 40.0f; 
  if (live1X_Ax > live1X_Rad * 1.2f) s += 50.0f; 
  return s; 
}

float calcParallelMisalignScore() { 
  float s = 0.0f; 
  if (live2X_Rad > thAlert1X_Rad) s += 40.0f; 
  if (live2X_Rad > live1X_Rad * 1.0f) s += 40.0f; 
  return s; 
}

float calcLoosenessScore() { 
  float s = 0.0f; 
  if (live1X_Rad > thAlert1X_Rad || live1X_Ax > thAlert1X_Ax) {
    s += 20.0f; 
  }
  float r = live3X_Rad / (live1X_Rad + 0.01f); 
  if (r > 0.3f) {
    s += 80.0f * min(1.0f, r / 0.6f); 
  }
  return s; 
}

Fault diagnose() {
  float stopThreshold = max(blRMS_Rad * 0.20f, 0.03f);
  if (liveRMS_Rad < stopThreshold && liveRMS_Ax < stopThreshold) {
    return F_MOTOR_STOPPED;
  }

  float raw_ub = calcUnbalanceScore(); 
  float raw_am = calcAngularMisalignScore(); 
  float raw_pm = calcParallelMisalignScore(); 
  float raw_lo = calcLoosenessScore();
  
  smooth_ub = (SMOOTHING_ALPHA * raw_ub) + ((1.0f - SMOOTHING_ALPHA) * smooth_ub); 
  smooth_am = (SMOOTHING_ALPHA * raw_am) + ((1.0f - SMOOTHING_ALPHA) * smooth_am); 
  smooth_pm = (SMOOTHING_ALPHA * raw_pm) + ((1.0f - SMOOTHING_ALPHA) * smooth_pm); 
  smooth_lo = (SMOOTHING_ALPHA * raw_lo) + ((1.0f - SMOOTHING_ALPHA) * smooth_lo);
  
  float max_smooth = max(max(smooth_ub, smooth_am), max(smooth_pm, smooth_lo)); 
  Fault instantWinner = F_NONE;
  
  if (max_smooth >= CONFIDENCE_THRESHOLD) { 
    if (max_smooth == smooth_lo) instantWinner = F_LOOSENESS; 
    else if (max_smooth == smooth_am) instantWinner = F_MISALIGNMENT_ANGULAR; 
    else if (max_smooth == smooth_pm) instantWinner = F_MISALIGNMENT_PARALLEL; 
    else if (max_smooth == smooth_ub) instantWinner = F_UNBALANCE; 
  }
  
  faultHistory[historyIdx] = instantWinner; 
  historyIdx = (historyIdx + 1) % FAULT_HISTORY_SIZE; 
  
  uint8_t votes[6] = {0}; 
  for (int i = 0; i < FAULT_HISTORY_SIZE; i++) {
    votes[faultHistory[i]]++;
  }
  
  Fault newConfirmed = confirmedFault; 
  uint8_t maxVotes = 0; 
  Fault challenger = F_NONE;
  
  for (int i = 0; i < 5; i++) { 
    if (i != confirmedFault && votes[i] > maxVotes) { 
      maxVotes = votes[i]; 
      challenger = (Fault)i; 
    } 
  }
  
  if (maxVotes >= VOTES_TO_CONFIRM) {
    newConfirmed = challenger; 
  } else if (confirmedFault == F_NONE && maxVotes >= 2) {
    newConfirmed = challenger;
  }
  
  confirmedFault = newConfirmed; 
  return confirmedFault;
}

// === task loop core 0 ===

void TaskFFT_Code(void *pvParameters) {
  esp_task_wdt_add(NULL);
  unsigned long lastDisplayMs = 0;
  unsigned long lastSDSendLocalMs = 0;
  static bool sdAutoStarted = false; // variabel untuk auto start sd

  for (;;) {
    esp_task_wdt_reset();
    processUICommands();
    unsigned long now = millis();

    switch (sysState) {
      case ST_INIT: { 
        break; 
      }
      
      case ST_COLD_RECOVERY: {
        changeState(ST_RUNNING); 
        break;
      }
      
      case ST_INPUT_PARAMS: { 
        vTaskDelay(100 / portTICK_PERIOD_MS); 
        break; 
      }
      
      case ST_BOOT_DELAY: {
        startAcquisition(); 
        while (!acqComplete) { 
          runAcquisition(); 
          taskYIELD(); 
        }
        processFFT(); 
        isAcquiring = false;
        
        if (now - stateEnterMs >= BOOT_DELAY_MS) {
          changeState(ST_STABILIZATION);
        }
        break;
      }
      
      case ST_STABILIZATION: {
        startAcquisition(); 
        while (!acqComplete) { 
          runAcquisition(); 
          taskYIELD(); 
        }
        processFFT(); 
        isAcquiring = false;
        
        if (now - lastStabCheckMs >= STAB_CHECK_MS) {
          lastStabCheckMs = now; 
          float tol = stabPrevRMS * STAB_RMS_TOL;
          
          if (fabs(liveRMS_Rad - stabPrevRMS) <= max(tol, 0.01f)) {
            stabStableCount++; 
          } else {
            stabStableCount = 0;
          }
          stabPrevRMS = liveRMS_Rad;
          
          Serial.printf("[STAB] RMS: %.3f | Stable Count: %d / %d\n", liveRMS_Rad, stabStableCount, STAB_NEED_COUNT);
          
          if (stabStableCount >= STAB_NEED_COUNT) changeState(ST_BASELINE_CAPTURE);
          if (now - stateEnterMs >= STAB_MAX_MS) changeState(ST_BASELINE_CAPTURE);
        } 
        break;
      }
      
      case ST_BASELINE_CAPTURE: {
        startAcquisition(); 
        while (!acqComplete) { 
          runAcquisition(); 
          taskYIELD(); 
        }
        processFFT(); 
        isAcquiring = false;
        
        if (capIdx < BASELINE_N) {
          capRMS_Rad[capIdx] = liveRMS_Rad; 
          cap1X_Rad[capIdx] = live1X_Rad; 
          cap2X_Rad[capIdx] = live2X_Rad; 
          cap3X_Rad[capIdx] = live3X_Rad;
          
          capRMS_Ax[capIdx] = liveRMS_Ax; 
          cap1X_Ax[capIdx] = live1X_Ax; 
          cap2X_Ax[capIdx] = live2X_Ax; 
          cap3X_Ax[capIdx] = live3X_Ax;
          
          capIdx++; 
          Serial.printf("[CAPTURE] Progress: %d / %d (RMS R: %.3f, A: %.3f)\n", capIdx, BASELINE_N, liveRMS_Rad, liveRMS_Ax);
        }
        
        if (capIdx >= BASELINE_N) { 
          changeState(ST_BASELINE_VALIDATE); 
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        break;
      }
      
      case ST_BASELINE_VALIDATE: {
        float sdRMS_Rad = stdDev(capRMS_Rad, BASELINE_N); 
        float sdRMS_Ax = stdDev(capRMS_Ax, BASELINE_N);
        float meanRMS_Rad = 0, meanRMS_Ax = 0;
        
        for (int i = 0; i < BASELINE_N; i++) { 
          meanRMS_Rad += capRMS_Rad[i]; 
          meanRMS_Ax += capRMS_Ax[i]; 
        }
        meanRMS_Rad /= BASELINE_N; 
        meanRMS_Ax /= BASELINE_N;
        
        bool rmsOk = (sdRMS_Rad / (meanRMS_Rad + 0.001f)) < STD_DEV_RATIO_MAX_RMS && (sdRMS_Ax / (meanRMS_Ax + 0.001f)) < STD_DEV_RATIO_MAX_RMS;
        bool specOk = true; 
        
        float sd1X_Rad = stdDev(cap1X_Rad, BASELINE_N); 
        float sd2X_Rad = stdDev(cap2X_Rad, BASELINE_N);
        float mean1X_Rad = 0, mean2X_Rad = 0, mean3X_Rad = 0; 
        float mean1X_Ax = 0, mean2X_Ax = 0, mean3X_Ax = 0;
        
        for (int i = 0; i < BASELINE_N; i++) { 
          mean1X_Rad += cap1X_Rad[i]; 
          mean2X_Rad += cap2X_Rad[i]; 
          mean3X_Rad += cap3X_Rad[i]; 
          mean1X_Ax += cap1X_Ax[i]; 
          mean2X_Ax += cap2X_Ax[i]; 
          mean3X_Ax += cap3X_Ax[i]; 
        }
        mean1X_Rad /= BASELINE_N; 
        mean2X_Rad /= BASELINE_N; 
        mean3X_Rad /= BASELINE_N; 
        mean1X_Ax /= BASELINE_N; 
        mean2X_Ax /= BASELINE_N; 
        mean3X_Ax /= BASELINE_N;
        
        if (sd1X_Rad / (mean1X_Rad + 0.001f) > STD_DEV_RATIO_MAX_SPEC) specOk = false;
        if (sd2X_Rad / (mean2X_Rad + 0.001f) > STD_DEV_RATIO_MAX_SPEC) specOk = false;
        
        if (rmsOk && specOk) {
          blRMS_Rad = meanRMS_Rad; 
          blRMS_Ax = meanRMS_Ax; 
          bl1X_Rad = mean1X_Rad; 
          bl1X_Ax = mean1X_Ax; 
          bl2X_Rad = mean2X_Rad; 
          bl2X_Ax = mean2X_Ax; 
          bl3X_Rad = mean3X_Rad; 
          bl3X_Ax = mean3X_Ax;
          
          baselineValid = true; 
          recalcThresholds(); 
          eepromSave();
          Serial.println(F("[BASELINE] Validasi OK! Masuk RUNNING."));
          changeState(ST_RUNNING);
        } else { 
          Serial.println(F("[BASELINE] Validasi Gagal! Mengulang capture..."));
          capIdx = 0; 
          changeState(ST_BASELINE_CAPTURE); 
        }
        break;
      }
      
      case ST_RUNNING: {
        if (!isAcquiring) startAcquisition();
        if (isAcquiring && !acqComplete) { 
          runAcquisition(); 
          taskYIELD(); 
        } 
        
        if (acqComplete) {
          processFFT(); 
          diagnose(); 
          isAcquiring = false;
          
          bool alertRad = (liveRMS_Rad > thAlertRMS_Rad || live1X_Rad > thAlert1X_Rad || live2X_Rad > thAlert2X_Rad || live3X_Rad > thAlert3X_Rad);
          bool dangerRad = (liveRMS_Rad > thDangerRMS_Rad || live1X_Rad > thDanger1X_Rad || live2X_Rad > thDanger2X_Rad || live3X_Rad > thDanger3X_Rad);
          bool alertAx = (liveRMS_Ax > thAlertRMS_Ax || live1X_Ax > thAlert1X_Ax || live2X_Ax > thAlert2X_Ax || live3X_Ax > thAlert3X_Ax);
          bool dangerAx = (liveRMS_Ax > thDangerRMS_Ax || live1X_Ax > thDanger1X_Ax || live2X_Ax > thDanger2X_Ax || live3X_Ax > thDanger3X_Ax);
          
          if (dangerRad || dangerAx) {
            changeState(ST_DANGER); 
          } else if (alertRad || alertAx) {
            changeState(ST_ALERT);
          }
          
          float stopTh = max(blRMS_Rad * MOTOR_STOP_RATIO, MOTOR_STOP_FLOOR);
          if (liveRMS_Rad < stopTh && liveRMS_Ax < stopTh && (now - runningEnterMs > RUN_UP_IGNORE_MS)) {
            changeState(ST_STANDBY);
          }
          startAcquisition();
        } 
        break;
      }
      
      case ST_STANDBY: {
        float rms = quickRMS(); 
        float stopTh = max(blRMS_Rad * MOTOR_STOP_RATIO, MOTOR_STOP_FLOOR);
        if (rms > stopTh * 2.0f) {
          changeState(ST_RUNNING);
        }
        vTaskDelay(200 / portTICK_PERIOD_MS); 
        break;
      }
      
      case ST_ALERT: {
        if (!isAcquiring) startAcquisition();
        if (isAcquiring && !acqComplete) { 
          runAcquisition(); 
          taskYIELD(); 
        } 
        
        if (acqComplete) {
          processFFT(); 
          diagnose(); 
          isAcquiring = false;
          
          bool dangerRad = (liveRMS_Rad > thDangerRMS_Rad || live1X_Rad > thDanger1X_Rad); 
          bool dangerAx = (liveRMS_Ax > thDangerRMS_Ax || live1X_Ax > thDanger1X_Ax);
          bool alertRad = (liveRMS_Rad > thAlertRMS_Rad || live1X_Rad > thAlert1X_Rad); 
          bool alertAx = (liveRMS_Ax > thAlertRMS_Ax || live1X_Ax > thAlert1X_Ax);
          
          if (dangerRad || dangerAx) {
            changeState(ST_DANGER); 
          } else if (!alertRad && !alertAx) { 
            changeState(ST_RUNNING); 
            triggerAlert(AT_BACK_TO_NORMAL); 
          }
          
          float stopTh = max(blRMS_Rad * MOTOR_STOP_RATIO, MOTOR_STOP_FLOOR);
          if (liveRMS_Rad < stopTh && liveRMS_Ax < stopTh) { 
            changeState(ST_STANDBY); 
            triggerAlert(AT_MOTOR_STOPPED); 
          }
          startAcquisition();
        } 
        break;
      }
      
      case ST_DANGER: {
        if (!isAcquiring) startAcquisition();
        if (isAcquiring && !acqComplete) { 
          runAcquisition(); 
          taskYIELD(); 
        } 
        
        if (acqComplete) {
          processFFT(); 
          diagnose(); 
          isAcquiring = false;
          
          bool dangerRad = (liveRMS_Rad > thDangerRMS_Rad || live1X_Rad > thDanger1X_Rad); 
          bool dangerAx = (liveRMS_Ax > thDangerRMS_Ax || live1X_Ax > thDanger1X_Ax);
          bool alertRad = (liveRMS_Rad > thAlertRMS_Rad || live1X_Rad > thAlert1X_Rad); 
          bool alertAx = (liveRMS_Ax > thAlertRMS_Ax || live1X_Ax > thAlert1X_Ax);
          
          if (!dangerRad && !dangerAx) { 
            if (alertRad || alertAx) {
              changeState(ST_ALERT); 
            } else { 
              changeState(ST_RUNNING); 
              triggerAlert(AT_BACK_TO_NORMAL); 
            } 
          }
          
          float stopTh = max(blRMS_Rad * MOTOR_STOP_RATIO, MOTOR_STOP_FLOOR);
          if (liveRMS_Rad < stopTh && liveRMS_Ax < stopTh) { 
            changeState(ST_STANDBY); 
            triggerAlert(AT_MOTOR_STOPPED); 
          }
          startAcquisition();
        } 
        break;
      }
    }

    if (now - lastDisplayMs >= 50) { 
      lastDisplayMs = now; 
      requestDisplayLive(); 
    }

    // auto-start sd logging saat st_running
    if (sysState == ST_RUNNING && sdPresent && sdAutoLog && !sdAutoStarted) {
      sendSDCommand(SD_CMD_START_LOG);
      isSdLoggingExpected = true;
      sdAutoStarted = true;
      Serial.println(F("[SYSTEM] Auto-Start SD Logging..."));
    }
    
    if (sysState != ST_RUNNING) {
      sdAutoStarted = false; // reset flag jika keluar dari running
    }

    if (isSdLoggingExpected && sdAutoLog && (sysState >= ST_RUNNING) && (now - lastSDSendLocalMs >= sdLogIntervalMs)) { 
      lastSDSendLocalMs = now; 
      sendSDData(); 
    }
    
    if (matlabMode && acqComplete) { }
    
    vTaskDelay(1);
  }
}