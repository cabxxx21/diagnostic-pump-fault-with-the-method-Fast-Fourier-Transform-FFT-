#include "globals.h"

// === tombol (debounce & long press) ===

BtnEvent pollButtons() {
  unsigned long now = millis();
  BtnEvent ev = BTN_NONE;

  // cek up
  bool curUp = digitalRead(btnUp.pin);
  if (curUp != btnUp.lastRaw) {
    btnUp.lastDebounce = now;
  }
  btnUp.lastRaw = curUp;
  
  if ((now - btnUp.lastDebounce) > DEBOUNCE_MS) {
    if (curUp == LOW && btnUp.stable == HIGH) {
      btnUp.pressStart = now; 
      btnUp.longFired = false; 
      btnUp.stable = LOW;
      ev = BTN_UP;
    } else if (curUp == HIGH && btnUp.stable == LOW) {
      btnUp.stable = HIGH;
    }
    
    if (curUp == LOW && btnUp.stable == LOW && !btnUp.longFired && (now - btnUp.pressStart > LONG_PRESS_MS)) {
      btnUp.longFired = true; 
      btnUp.pressStart = now; 
      ev = BTN_UP;
    }
    
    if (curUp == LOW && btnUp.stable == LOW && btnUp.longFired && (now - btnUp.pressStart > REPEAT_MS)) {
      btnUp.pressStart = now; 
      ev = BTN_UP;
    }
  }
  if (ev != BTN_NONE) return ev;

  // cek down 
  bool curDown = digitalRead(btnDown.pin);
  if (curDown != btnDown.lastRaw) {
    btnDown.lastDebounce = now;
  }
  btnDown.lastRaw = curDown;
  
  if ((now - btnDown.lastDebounce) > DEBOUNCE_MS) {
    if (curDown == LOW && btnDown.stable == HIGH) {
      btnDown.pressStart = now; 
      btnDown.longFired = false; 
      btnDown.stable = LOW;
      ev = BTN_DOWN;
    } else if (curDown == HIGH && btnDown.stable == LOW) {
      btnDown.stable = HIGH;
    }
    
    if (curDown == LOW && btnDown.stable == LOW && !btnDown.longFired && (now - btnDown.pressStart > LONG_PRESS_MS)) {
      btnDown.longFired = true; 
      btnDown.pressStart = now; 
      ev = BTN_DOWN;
    }
    
    if (curDown == LOW && btnDown.stable == LOW && btnDown.longFired && (now - btnDown.pressStart > REPEAT_MS)) {
      btnDown.pressStart = now; 
      ev = BTN_DOWN;
    }
  }
  if (ev != BTN_NONE) return ev;

  // cek select 
  bool curOk = digitalRead(btnOk.pin);
  if (curOk != btnOk.lastRaw) {
    btnOk.lastDebounce = now;
  }
  btnOk.lastRaw = curOk;
  
  if ((now - btnOk.lastDebounce) > DEBOUNCE_MS) {
    if (curOk == LOW && btnOk.stable == HIGH) {
      btnOk.pressStart = now; 
      btnOk.stable = LOW;
      ev = BTN_SELECT;
    } else if (curOk == HIGH && btnOk.stable == LOW) {
      btnOk.stable = HIGH;
    }
  }

  return ev;
}

// === handle menu logic ===

void handleMenuParamsUI(BtnEvent btn, UIScreen &scr, int8_t &cur, NavMode &nav, float &editVal, DisplayPacket &p) {
  const int ITEMS = 3;
  if (nav == NAV_BROWSE) {
    if (btn == BTN_UP) cur = (cur - 1 + ITEMS) % ITEMS;
    if (btn == BTN_DOWN) cur = (cur + 1) % ITEMS;
    
    if (btn == BTN_SELECT) {
      if (cur == 0) { 
        nav = NAV_EDIT; 
        editVal = p.motorRPM; 
      }
      else if (cur == 1) { 
        nav = NAV_EDIT; 
        editVal = p.isoClass; 
      }
      else if (cur == 2) { 
        if (p.motorRPM > 0) {
          sendUICmd(UI_CMD_START_BOOT);
        }
        scr = SCREEN_LIVE; 
      }
    }
  } else { 
    if (cur == 0) { // mode edit rpm
      if (btn == BTN_UP) editVal += 100;
      if (btn == BTN_DOWN) editVal = max(0.0f, editVal - 100);
      
      if (btn == BTN_SELECT) { 
        sendUICmd(UI_CMD_SET_PARAMS, editVal, p.isoClass); 
        nav = NAV_BROWSE; 
      }
    } else if (cur == 1) { // mode edit iso
      if (btn == BTN_UP) editVal = min(4.0f, editVal + 1);
      if (btn == BTN_DOWN) editVal = max(1.0f, editVal - 1);
      
      if (btn == BTN_SELECT) { 
        sendUICmd(UI_CMD_SET_PARAMS, p.motorRPM, (uint8_t)editVal); 
        nav = NAV_BROWSE; 
      }
    }
  }
}

void handleMenuRuntimeUI(BtnEvent btn, UIScreen &scr, int8_t &cur, NavMode &nav, DisplayPacket &p) {
  const int ITEMS = 8; 
  if (btn == BTN_UP) cur = (cur - 1 + ITEMS) % ITEMS;
  if (btn == BTN_DOWN) cur = (cur + 1) % ITEMS;
  
  if (btn == BTN_SELECT) {
    switch (cur) {
      case 0: scr = SCREEN_MENU_NETWORK; cur = 0; break; 
      case 1: sendUICmd(UI_CMD_REBASELINE); scr = SCREEN_LIVE; break;
      case 2: sendUICmd(UI_CMD_TOGGLE_AXIS_MAP); break; 
      case 3: sendUICmd(UI_CMD_TOGGLE_THEME); break; 
      case 4: sendUICmd(UI_CMD_TOGGLE_MATLAB); break;    
      case 5: scr = SCREEN_MENU_SD; cur = 0; break;     
      case 6: sendUICmd(UI_CMD_RESET_EEPROM); break;
      case 7: scr = SCREEN_LIVE; break;                  
    }
  }
}

void handleMenuSDUI(BtnEvent btn, UIScreen &scr, int8_t &cur, NavMode &nav, float &editVal, DisplayPacket &p) {
  const int ITEMS = 3;
  if (nav == NAV_BROWSE) {
    if (btn == BTN_UP) cur = (cur - 1 + ITEMS) % ITEMS;
    if (btn == BTN_DOWN) cur = (cur + 1) % ITEMS;
    
    if (btn == BTN_SELECT) {
      if (cur == 0) { 
        sendUICmd(UI_CMD_SD_SET_AUTO, 0, p.sdAutoLogStat ? 0 : 1); 
      } 
      else if (cur == 1) { 
        nav = NAV_EDIT; 
        editVal = currentIntervalIdx; 
      }           
      else if (cur == 2) { 
        scr = SCREEN_MENU_RUNTIME; 
        cur = 4; 
      }                     
    }
  } else { // mode edit interval
    if (btn == BTN_UP) editVal = min((float)(NUM_INTERVAL_OPTIONS-1), editVal + 1);
    if (btn == BTN_DOWN) editVal = max(0.0f, editVal - 1);
    
    if (btn == BTN_SELECT) {
      uint32_t newInterval = INTERVAL_OPTIONS[(int)editVal];
      sendUICmd(UI_CMD_SD_SET_INTERVAL, 0, (uint8_t)editVal, newInterval);
      nav = NAV_BROWSE;
    }
  }
}

void handleMenuNetworkUI(BtnEvent btn, UIScreen &scr, int8_t &cur) {
  const int ITEMS = 2;
  if (btn == BTN_UP) cur = (cur - 1 + ITEMS) % ITEMS;
  if (btn == BTN_DOWN) cur = (cur + 1) % ITEMS;
  
  if (btn == BTN_SELECT) {
    if (cur == 0) {
      sendUICmd(UI_CMD_FORCE_NTP);
    }
    else if (cur == 1) { 
      scr = SCREEN_MENU_RUNTIME; 
      cur = 0; 
    } 
  }
}

// === core 1 task ui ===

void TaskUI_Code(void *pvParameters) {
  while (!sdInitDone) {
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }

  if (lcdNeedsReinit) {
    tft.init();
    tft.setRotation(1);
    lcdNeedsReinit = false;
  }
  
  tft.fillScreen(currentTheme == 0 ? HMI_BG_DARK : HMI_BG_LIGHT);

  sprMenu.createSprite(460, 30);
  sprMenu.setTextFont(2);

  UIScreen prevScreen = (UIScreen)99; 
  uint8_t prevTheme = 99; 
  NavMode navMode = NAV_BROWSE;
  int8_t menuCursor = 0;
  float editValue = 0;
  
  DisplayPacket p; 
  memset(&p, 0, sizeof(p));

  bool needsRedraw = true;
  unsigned long lastRenderMs = 0;

  for (;;) {
    // 1. Cek data baru dari FFT
    if (xDisplayQueue != NULL && xQueueReceive(xDisplayQueue, &p, 0) == pdPASS) {
      if (currentScreen == SCREEN_LIVE) {
        needsRedraw = true;
      }
    }

    // 2. Cek tombol
    BtnEvent btn = pollButtons();

    if (btn != BTN_NONE) {
      needsRedraw = true; 
      
      if (currentScreen == SCREEN_LIVE) {
        if (btn == BTN_SELECT) {
          currentScreen = (p.sysState == ST_INPUT_PARAMS || p.sysState == ST_COLD_RECOVERY) 
                          ? SCREEN_MENU_PARAMS 
                          : SCREEN_MENU_RUNTIME;
          menuCursor = 0; 
          navMode = NAV_BROWSE;
        }
      } else {
        if (currentScreen == SCREEN_MENU_PARAMS) {
          handleMenuParamsUI(btn, currentScreen, menuCursor, navMode, editValue, p);
        }
        else if (currentScreen == SCREEN_MENU_RUNTIME) {
          handleMenuRuntimeUI(btn, currentScreen, menuCursor, navMode, p);
        }
        else if (currentScreen == SCREEN_MENU_SD) {
          handleMenuSDUI(btn, currentScreen, menuCursor, navMode, editValue, p);
        }
        else if (currentScreen == SCREEN_MENU_NETWORK) {
          handleMenuNetworkUI(btn, currentScreen, menuCursor);
        }
      }
    }

    // 3. Live screen render 100ms
    if (currentScreen == SCREEN_LIVE && millis() - lastRenderMs >= 100) {
      needsRedraw = true;
      lastRenderMs = millis();
    }

    // 4. Cek perubahan tema
    if (prevTheme != currentTheme) {
      prevScreen = (UIScreen)99; 
      needsRedraw = true;
      prevTheme = currentTheme;
    }

    // 5. Render execution (Logika sudah simpel)
    if (needsRedraw) {
      needsRedraw = false;
      
      if (xSPIMutex != NULL && xSemaphoreTake(xSPIMutex, 50) == pdTRUE) {
        digitalWrite(SD_CS, HIGH);
        tft.startWrite();
        
        // Render Static Background
        if (prevScreen != currentScreen) {
          if (currentScreen == SCREEN_LIVE) drawStaticLiveBg();
          else if (currentScreen == SCREEN_MENU_PARAMS) drawStaticMenuBg("SET PARAMETER");
          else if (currentScreen == SCREEN_MENU_RUNTIME) drawStaticMenuBg("RUNTIME MENU");
          else if (currentScreen == SCREEN_MENU_SD) drawStaticMenuBg("SD CARD SETTINGS");
          else if (currentScreen == SCREEN_MENU_NETWORK) drawStaticMenuBg("NETWORK & TIME");
          prevScreen = currentScreen;
        }

        // Render Dynamic Content
        if (currentScreen == SCREEN_LIVE) updateLiveDynamic(p);
        else if (currentScreen == SCREEN_MENU_PARAMS) renderScreenParams(menuCursor, navMode, editValue, p);
        else if (currentScreen == SCREEN_MENU_RUNTIME) renderScreenRuntime(menuCursor, p);
        else if (currentScreen == SCREEN_MENU_SD) renderScreenSD(menuCursor, navMode, editValue, p);
        else if (currentScreen == SCREEN_MENU_NETWORK) renderScreenNetwork(menuCursor, p);
        
        tft.endWrite();
        xSemaphoreGive(xSPIMutex);
      } else {
        needsRedraw = true; 
      }
    }

    vTaskDelay(1 / portTICK_PERIOD_MS); 
  }
}y