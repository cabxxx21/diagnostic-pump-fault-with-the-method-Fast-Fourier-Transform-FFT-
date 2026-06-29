#include "globals.h"

// helper untuk format string state & fault
static const char* _stateStr[] = {
  "INIT", "COLD", "INPUT", "BOOT", 
  "STAB", "CAP", "VAL", "RUN", 
  "STBY", "ALRT", "DNG"
};

static const char* _faultStr[] = {
  "NONE", "UNBAL", "MIS_A", "MIS_P", "LOOSE", "STOP"
};

// === fungsi telegram ===

bool sendTelegramMessage(const char* message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[TG] GAGAL - WiFi tidak tersambung!"));
    return false;
  }

  WiFiClientSecure client; 
  client.setInsecure();
  HTTPClient https;
  
  String url = "https://api.telegram.org/bot" + String(TELEGRAM_BOT_TOKEN) + "/sendMessage";
  
  if (!https.begin(client, url)) {
    Serial.println(F("[TG] GAGAL - HTTPS begin error!"));
    return false;
  }
  
  https.setTimeout(10000); 
  https.addHeader("Content-Type", "application/json");
  
  String jsonMsg = message;
  jsonMsg.replace("\\", "\\\\");   
  jsonMsg.replace("\"", "\\\"");   
  jsonMsg.replace("\n", "\\n");    
  jsonMsg.replace("\r", "\\r");    
  jsonMsg.replace("\t", "\\t");    
  
  String payload = "{\"chat_id\":\"" + String(TELEGRAM_CHAT_ID) + 
                   "\",\"text\":\"" + jsonMsg + 
                   "\",\"parse_mode\":\"HTML\"}";
  
  Serial.println(F("[TG] Mengirim pesan..."));
  int httpCode = https.POST(payload);
  
  if (httpCode > 0) {
    String response = https.getString();
    Serial.printf("[TG] HTTP Code: %d\n", httpCode);
    
    if (httpCode == 200) {
      Serial.println(F("[TG] Berhasil terkirim!"));
    } else {
      Serial.println(F("[TG] GAGAL kirim! Response:"));
      Serial.println(response); 
    }
  } else {
    Serial.printf("[TG] HTTP Error: %s\n", https.errorToString(httpCode).c_str());
  }
  
  https.end();
  return (httpCode == 200);
}

void formatAlertMessage(AlertPacket* pkt, char* buf, size_t len) {
  const char* faultStr[] = {
    "NORMAL", "UNBALANCE", "MISALIGN-ANG", 
    "MISALIGN-PAR", "LOOSENESS", "MOTOR STOPPED"
  };
  
  const char* typeStr[] = {
    "FAULT DETECTED", "FAULT CHANGED", "BACK TO NORMAL", "MOTOR STOPPED"
  };
  
  char timeStr[25] = "N/A"; 
  struct tm timeinfo;
  
  if (getLocalTime(&timeinfo, 1000)) {
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  }
  
  int n = 0;
  n += snprintf(buf+n, len-n, "🏭 <b>PUMP MONITOR</b>\n");
  n += snprintf(buf+n, len-n, "━━━━━━━━━━━━━━━━\n");
  n += snprintf(buf+n, len-n, "📋 %s\n", typeStr[pkt->type]);
  n += snprintf(buf+n, len-n, "🕐 %s\n", timeStr);
  n += snprintf(buf+n, len-n, "🔧 %s\n", faultStr[pkt->confirmedFault]);
  n += snprintf(buf+n, len-n, "⚙️ RPM: %.0f | ISO: %d\n", pkt->motorRPM, pkt->isoClass);
  n += snprintf(buf+n, len-n, "━━━━━━━━━━━━━━━━\n");
  
  n += snprintf(buf+n, len-n, "📐 <b>Radial:</b>\n");
  n += snprintf(buf+n, len-n, " RMS: %.3f 1X: %.3f 2X: %.3f 3X: %.3f\n", pkt->liveRMS_Rad, pkt->live1X_Rad, pkt->live2X_Rad, pkt->live3X_Rad);
  
  n += snprintf(buf+n, len-n, "📐 <b>Axial:</b>\n");
  n += snprintf(buf+n, len-n, " RMS: %.3f 1X: %.3f 2X: %.3f 3X: %.3f\n", pkt->liveRMS_Ax, pkt->live1X_Ax, pkt->live2X_Ax, pkt->live3X_Ax);
  
  if (pkt->tempObj > 0.0f || pkt->tempAmb > 0.0f) {
    n += snprintf(buf+n, len-n, "🌡️ Obj: %.1f°C Amb: %.1f°C\n", pkt->tempObj, pkt->tempAmb);
  }
}

// === task rtos logger ===

void TaskLogger_Code(void *pvParameters) {
  bool localSdLogging = false;
  bool localSdPresent = sdPresent;
  uint32_t localLogIntervalMs = sdLogIntervalMs;
  File logFile;
  char currentActivePath[40] = "";
  int lastKnownYear = 2024;
  bool localNtpSynced = false;

  unsigned long lastNtpSyncMs = 0;
  unsigned long lastAlertMs = 0;
  unsigned long lastFlushMs = 0;
  unsigned long lastStatusSendMs = millis();
  unsigned long wifiConnectStart = 0;
  
  uint8_t consecWriteFails = 0;
  NetState netState = NS_OFF;

  #define SD_FLUSH_INTERVAL_MS 10000

  for (;;) {
    unsigned long now = millis();

    if (WiFi.status() == WL_CONNECTED) {
      if (netState != NS_SYNCING && netState != NS_SENDING_ALERTS) {
        netState = NS_SYNCING;
      }
      
      if (forceNtpSyncFlag || !ntpSynced || (now - lastNtpSyncMs > NTP_RETRY_INTERVAL_MS)) {
        configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, "pool.ntp.org", "time.nist.gov");
        struct tm timeinfo;
        
        if (getLocalTime(&timeinfo, 1000)) {
          ntpSynced = true;
          strftime(lastSyncStr, sizeof(lastSyncStr), "%H:%M:%S", &timeinfo);
          lastNtpSyncMs = now; 
          forceNtpSyncFlag = false;
        }
      }
    } else {
      if (netState != NS_CONNECTING) {
        ntpSynced = false; 
        netState = NS_CONNECTING;
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        wifiConnectStart = now;
        Serial.println(F("[WIFI] Mulai sambung..."));
      } else if (now - wifiConnectStart >= WIFI_TIMEOUT_MS) {
        netState = NS_OFF;
        WiFi.mode(WIFI_OFF);
        Serial.println(F("[WIFI] Timeout, matikan."));
      }
    }

    if (xSDQueue != NULL) {
      SDPacket sdPkt;
      while (xQueueReceive(xSDQueue, &sdPkt, 0) == pdPASS) {

        switch (sdPkt.cmd) {
          case SD_CMD_SET_INTERVAL:
            localLogIntervalMs = sdPkt.logIntervalMs;
            break;

          case SD_CMD_FORCE_NTP:
            forceNtpSyncFlag = true;
            break;

          case SD_CMD_START_LOG:
            if (localSdPresent && !localSdLogging) {
              localSdLogging = true;
              sdRowsWritten = 0;
              currentActivePath[0] = '\0';
              Serial.println(F("[SD] Logging started."));
            }
            break;

          case SD_CMD_STOP_LOG:
            if (localSdLogging) {
              if (xSPIMutex != NULL && xSemaphoreTake(xSPIMutex, 2000) == pdTRUE) {
                digitalWrite(TFT_CS, HIGH);   
                digitalWrite(SD_CS, LOW);     
                
                if (logFile) {
                  logFile.close();
                }
                
                digitalWrite(SD_CS, HIGH);    
                xSemaphoreGive(xSPIMutex);
              }
              localSdLogging = false;
              currentActivePath[0] = '\0';
              Serial.println(F("[SD] Logging stopped."));
            }
            break;

          case SD_CMD_DATA:
            if (localSdPresent && localSdLogging) {
              char rowBuf[200];
              char timeStr[25];
              struct tm timeinfo;

              taskENTER_CRITICAL(&ntpMux);
              bool hasNtp = ntpSynced;
              taskEXIT_CRITICAL(&ntpMux);

              if (getLocalTime(&timeinfo) && hasNtp) {
                int year = timeinfo.tm_year + 1900;
                lastKnownYear = year;
                localNtpSynced = true;
                strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%S", &timeinfo);
              } else {
                localNtpSynced = false;
                snprintf(timeStr, sizeof(timeStr), "%lu", sdPkt.timestamp);
              }

              uint8_t sIdx = (sdPkt.sysState < 11) ? sdPkt.sysState : 0;
              uint8_t fIdx = (sdPkt.confirmedFault < 6) ? sdPkt.confirmedFault : 0;

              snprintf(rowBuf, sizeof(rowBuf),
                "%s,%s,%s,%.0f,%d,%.3f,%.3f,%.1f,%.1f",
                timeStr, _stateStr[sIdx], _faultStr[fIdx],
                sdPkt.motorRPM, sdPkt.isoClass,
                sdPkt.liveRMS_Rad, sdPkt.liveRMS_Ax,
                sdPkt.tempObj, sdPkt.tempAmb);

              if (xSPIMutex != NULL && xSemaphoreTake(xSPIMutex, 2000) == pdTRUE) {
                
                digitalWrite(TFT_CS, HIGH);  
                digitalWrite(SD_CS, LOW);    

                char targetPath[40];
                if (hasNtp && getLocalTime(&timeinfo)) {
                  int year = timeinfo.tm_year + 1900;
                  int month = timeinfo.tm_mon + 1;
                  snprintf(targetPath, sizeof(targetPath), "/%04d/%02d.csv", year, month);
                } else {
                  snprintf(targetPath, sizeof(targetPath), "/%04d/unrecorded.csv", lastKnownYear);
                }

                bool needOpenNewFile = (strcmp(targetPath, currentActivePath) != 0);

                if (needOpenNewFile) {
                  if (logFile) {
                    logFile.close();
                  }
                  
                  char dirPath[10];
                  snprintf(dirPath, sizeof(dirPath), "/%04d", lastKnownYear);
                  SD.mkdir(dirPath);
                  
                  logFile = SD.open(targetPath, FILE_WRITE);
                  if (logFile) {
                    if (logFile.size() == 0) {
                      logFile.println("timestamp,state,fault,RPM,ISO,RMS_Rad,RMS_Ax,TempObj,TempAmb");
                    }
                    strncpy(currentActivePath, targetPath, sizeof(currentActivePath));
                    strncpy(sdFileName, currentActivePath, sizeof(sdFileName));
                    Serial.printf("[SD] File aktif: %s\n", currentActivePath);
                  } else {
                    Serial.println(F("[SD] Gagal buka file, coba re-mount..."));
                    SD.end();
                    digitalWrite(SD_CS, HIGH); delay(5); digitalWrite(SD_CS, LOW);
                    
                    if (SD.begin(SD_CS, tft.getSPIinstance())) {
                      SD.mkdir(dirPath);
                      logFile = SD.open(targetPath, FILE_WRITE);
                    }
                    
                    if (logFile) {
                      if (logFile.size() == 0) logFile.println("timestamp,state,fault,RPM,ISO,RMS_Rad,RMS_Ax,TempObj,TempAmb");
                      strncpy(currentActivePath, targetPath, sizeof(currentActivePath));
                      strncpy(sdFileName, currentActivePath, sizeof(sdFileName));
                    } else {
                      localSdPresent = false; 
                      localSdLogging = false;
                      SD.end(); 
                      currentActivePath[0] = '\0';
                      Serial.println(F("[SD] Re-mount gagal total, SD disabled!"));
                    }
                  }
                }

                if (logFile) {
                  size_t written = logFile.println(rowBuf);
                  if (written > 0) {
                    consecWriteFails = 0;
                    sdRowsWritten++;
                    
                    if (sdRowsWritten % 10 == 0) {
                      Serial.printf("[SD] Wrote row #%lu -> %s\n", sdRowsWritten, currentActivePath);
                    }

                    if (now - lastFlushMs >= SD_FLUSH_INTERVAL_MS) {
                      logFile.flush();
                      lastFlushMs = now;
                    }
                  } else {
                    consecWriteFails++;
                    if (consecWriteFails >= 3) {
                      localSdPresent = false; 
                      localSdLogging = false;
                      logFile.close(); 
                      SD.end(); 
                      currentActivePath[0] = '\0';
                      Serial.println(F("[SD] 3x gagal tulis -> unmount!"));
                    }
                  }
                }

                digitalWrite(SD_CS, HIGH);  
                
                xSemaphoreGive(xSPIMutex);
              } else {
                Serial.println(F("[SD] SPI Mutex timeout!"));
              }
            }
            break;
        }
      }
    }

    // === telegram alerts ===
    
    if (xAlertQueue != NULL) {
      AlertPacket alPkt;
      while (xQueueReceive(xAlertQueue, &alPkt, 0) == pdPASS) {
        
        if (WiFi.status() != WL_CONNECTED) {
          Serial.println(F("[TG] Skip alert - WiFi belum nyala"));
          lastAlertOk = false;
          continue;
        }
        
        if (now - lastAlertMs >= ALERT_COOLDOWN_MS || alPkt.type == AT_BACK_TO_NORMAL) {
          char msgBuf[600]; 
          formatAlertMessage(&alPkt, msgBuf, sizeof(msgBuf));
          
          Serial.println(F("[TG] Message content:"));
          Serial.println(msgBuf);
          
          taskENTER_CRITICAL(&ntpMux); 
          wifiState = NS_SENDING_ALERTS; 
          taskEXIT_CRITICAL(&ntpMux);
          
          bool success = sendTelegramMessage(msgBuf);
          
          if (success) {
            lastAlertOk = true; 
            lastAlertedFault = alPkt.confirmedFault;
          } else { 
            lastAlertOk = false; 
          }
          
          lastAlertMs = now;
          
          taskENTER_CRITICAL(&ntpMux); 
          wifiState = (WiFi.status() == WL_CONNECTED) ? NS_SYNCING : NS_OFF; 
          taskEXIT_CRITICAL(&ntpMux);
        } else {
          Serial.printf("[TG] Cooldown, skip alert (%lus sisa)\n", 
                       (ALERT_COOLDOWN_MS - (now - lastAlertMs)) / 1000);
        }
      }
    }

    // === kirim status ke main setiap 1 detik ===
    if (now - lastStatusSendMs >= 1000) {
      lastStatusSendMs = now;
      
      static uint8_t lastNetState = 99;
      if ((uint8_t)netState != lastNetState) {
        const char* ns[] = {"OFF","CONNECTING","SYNCING","SENDING"};
        Serial.printf("[NET] State: %s | WiFi: %d | NTP: %d\n", 
                     ns[netState], WiFi.status() == WL_CONNECTED, ntpSynced);
        lastNetState = (uint8_t)netState;
      }

      taskENTER_CRITICAL(&sdMux);
      ::sdPresent = localSdPresent;
      ::sdLogging = localSdLogging;
      ::sdLogIntervalMs = localLogIntervalMs;
      taskEXIT_CRITICAL(&sdMux);
    }

    taskENTER_CRITICAL(&ntpMux); 
    wifiState = (uint8_t)netState; 
    taskEXIT_CRITICAL(&ntpMux);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}