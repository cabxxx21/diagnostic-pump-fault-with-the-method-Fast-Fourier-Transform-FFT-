//ui_renders.ino
#include "globals.h"

// === helper draw menu item ===

void drawMenuItem(int y, const char* label, const char* value, bool isActive, bool isEditing) {
  uint16_t bgColor, txtColor, valColor, accentColor;
  
  // Cek tema yang aktif
  if (currentTheme == 0) { // DARK THEME
    bgColor = isActive ? HMI_BG_CARD : HMI_BG_DARK;
    txtColor = isActive ? HMI_CYAN : HMI_TEXT_MAIN;
    valColor = HMI_YELLOW;
    accentColor = HMI_CYAN;
  } else { // LIGHT THEME
    bgColor = isActive ? LT_BG_CARD : LT_BG_MAIN;
    txtColor = isActive ? LT_ACCENT_ORG : LT_TEXT_MAIN;
    valColor = LT_ACCENT_BLU;
    accentColor = LT_ACCENT_ORG;
  }
  
  sprMenu.fillSprite(bgColor);
  
  if (isActive) {
    sprMenu.fillRect(0, 0, 3, 30, accentColor);
  }

  sprMenu.setTextFont(2);
  sprMenu.setTextColor(txtColor, bgColor);
  sprMenu.setCursor(8, 6);
  sprMenu.print(isActive ? "> " : "  ");
  sprMenu.print(label);

  if (value[0] != '\0') { 
    sprMenu.setTextColor(valColor, bgColor);
    sprMenu.setTextDatum(TR_DATUM); 
    
    if (isEditing) {
      char editBuf[32];
      snprintf(editBuf, sizeof(editBuf), "< %s >", value);
      sprMenu.drawString(editBuf, 450, 6);
    } else {
      sprMenu.drawString(value, 450, 6);
    }
    sprMenu.setTextDatum(TL_DATUM); 
  } else {
    sprMenu.fillRect(350, 0, 110, 30, bgColor);
  }
  
  sprMenu.pushSprite(10, y);
}

// === static backgrounds ===

void drawStaticLiveBg() {
  tft.fillScreen(HMI_BG_DARK);
  
  // header
  tft.fillRect(0, 0, 480, 32, HMI_BG_CARD); 
  tft.drawFastHLine(0, 32, 480, HMI_BORDER);
  tft.setTextFont(2); 
  tft.setTextColor(HMI_CYAN, HMI_BG_CARD); 
  tft.setCursor(10, 8); 
  tft.print("PUMP MONITOR v6.0");

  // box radial
  tft.fillRoundRect(8, 40, 305, 85, 4, HMI_BG_CARD); 
  tft.drawRoundRect(8, 40, 305, 85, 4, HMI_BORDER);
  tft.setTextColor(HMI_TEXT_DIM, HMI_BG_CARD); 
  tft.setTextFont(1); 
  tft.setCursor(18, 45); 
  tft.print("RADIAL VIBRATION (g)");

  // box axial
  tft.fillRoundRect(8, 130, 305, 85, 4, HMI_BG_CARD); 
  tft.drawRoundRect(8, 130, 305, 85, 4, HMI_BORDER);
  tft.setTextColor(HMI_TEXT_DIM, HMI_BG_CARD); 
  tft.setTextFont(1); 
  tft.setCursor(18, 135); 
  tft.print("AXIAL VIBRATION (g)");

  // box fault
  tft.fillRoundRect(8, 220, 305, 95, 4, HMI_BG_CARD); 
  tft.drawRoundRect(8, 220, 305, 95, 4, HMI_GREEN);
  tft.setTextColor(HMI_GREEN, HMI_BG_CARD); 
  tft.setTextFont(1); 
  tft.setCursor(18, 225); 
  tft.print("DIAGNOSED FAULT");

  // box rpm
  tft.fillRoundRect(320, 40, 152, 120, 4, HMI_BG_CARD); 
  tft.drawRoundRect(320, 40, 152, 120, 4, HMI_BORDER);
  tft.setTextColor(HMI_TEXT_DIM); 
  tft.setTextFont(2); 
  tft.setCursor(330, 45); 
  tft.print("RPM");

  // box temp
  tft.fillRoundRect(320, 165, 152, 55, 4, HMI_BG_CARD); 
  tft.drawRoundRect(320, 165, 152, 55, 4, HMI_BORDER);
  tft.setTextColor(HMI_TEXT_DIM); 
  tft.setTextFont(2); 
  tft.setCursor(330, 170); 
  tft.print("TEMP OBJ");

  // box indikator
  tft.fillRoundRect(320, 225, 152, 90, 4, HMI_BG_CARD); 
  tft.drawRoundRect(320, 225, 152, 90, 4, HMI_BORDER);
}

void drawStaticMenuBg(const char* title) {
  tft.fillScreen(HMI_BG_DARK);
  tft.fillRect(0, 0, 480, 32, HMI_BG_CARD);
  tft.drawFastHLine(0, 32, 480, HMI_BORDER);
  tft.setTextFont(2); 
  tft.setTextColor(HMI_CYAN, HMI_BG_CARD); 
  tft.setCursor(10, 8);
  tft.print(title);
}

// === dynamic updates ===

void updateLiveDynamic(DisplayPacket &p) {
  const char* sn[] = {"INIT","COLD","INPUT","BOOT","STAB","CAP","VAL","RUN","STBY","ALRT","DNG"};
  uint16_t stateClr = (p.sysState >= ST_ALERT) ? HMI_RED : (p.sysState == ST_RUNNING ? HMI_GREEN : HMI_YELLOW);
  
  static uint8_t lastDrawnState = 99;
  if (lastDrawnState != p.sysState) {
      tft.fillRect(400, 5, 80, 27, HMI_BG_CARD);
      tft.setTextFont(2); 
      tft.setTextColor(stateClr, HMI_BG_CARD);
      tft.setTextDatum(TR_DATUM);
      tft.drawString(sn[p.sysState], 470, 10);
      tft.setTextDatum(TL_DATUM);
      lastDrawnState = p.sysState;
  }

  tft.setTextFont(6); 
  tft.setTextColor(HMI_TEXT_MAIN, HMI_BG_CARD);
  tft.setCursor(18, 65); 
  tft.printf("%.3f  ", p.liveRMS_Rad);
  
  tft.setTextFont(2); 
  tft.setTextColor(HMI_CYAN, HMI_BG_CARD);
  tft.setCursor(185, 60);
  tft.printf("1X:%.3f", p.live1X_Rad);
  
  tft.setCursor(195, 80); 
  tft.printf("2X:%.3f", p.live2X_Rad);
  
  tft.setCursor(205, 100); 
  tft.printf("3X:%.3f", p.live3X_Rad);

  tft.setTextFont(6); 
  tft.setTextColor(HMI_TEXT_MAIN, HMI_BG_CARD);
  tft.setCursor(18, 155); 
  tft.printf("%.3f  ", p.liveRMS_Ax);
  
  tft.setTextFont(2); 
  tft.setTextColor(HMI_CYAN, HMI_BG_CARD);
  tft.setCursor(185, 150); 
  tft.printf("1X:%.3f", p.live1X_Ax);
  
  tft.setCursor(195, 170); 
  tft.printf("2X:%.3f", p.live2X_Ax);
  
  tft.setCursor(205, 190); 
  tft.printf("3X:%.3f", p.live3X_Ax);

  // FAULT BOX 
  uint16_t faultColor = (p.confirmedFault == F_NONE) ? HMI_GREEN : HMI_RED;
  static uint16_t lastFaultColor = HMI_GREEN;
  
  if (lastFaultColor != faultColor) {
    tft.fillRect(9, 236, 303, 78, HMI_BG_CARD); 
    lastFaultColor = faultColor;
  }
  
  tft.drawRoundRect(8, 220, 305, 95, 4, faultColor);
  tft.setTextColor(faultColor, HMI_BG_CARD); 
  tft.setTextFont(1); 
  tft.setCursor(18, 225); 
  tft.print("DIAGNOSED FAULT");
  
  const char* fn[] = {"NORMAL","UNBAL","MIS_A","MIS_P","LOOSE","STOP"};
  tft.setTextFont(4); 
  tft.setTextColor(faultColor, HMI_BG_CARD);
  tft.setCursor(18, 245); 
  tft.printf("%-7s  ", fn[p.confirmedFault]);

  // RPM
  tft.setTextFont(7); 
  tft.setTextColor(HMI_BLUE, HMI_BG_CARD);
  tft.setTextDatum(MC_DATUM);
  char rpmStr[8];
  sprintf(rpmStr, "%4.0f", p.motorRPM);
  tft.drawString(rpmStr, 385, 100); 
  tft.setTextDatum(TL_DATUM);

  // Temp
  tft.setTextFont(4);
  if (p.tempObj > 0.0f) {
    tft.setTextColor(HMI_YELLOW, HMI_BG_CARD);
    tft.setCursor(330, 190); 
    tft.printf("%.1fC     ", p.tempObj);
  } else {
    tft.setTextColor(HMI_RED, HMI_BG_CARD);
    tft.setCursor(330, 190); 
    tft.print("ERROR   "); 
  }

  // INDICATOR 
  tft.setTextFont(2);
  int16_t indRow1Y = 248;
  int16_t indRow2Y = 275;
  
  // SD 
  tft.setTextColor(p.sdPresent ? HMI_GREEN : HMI_RED, HMI_BG_CARD); 
  tft.setTextDatum(ML_DATUM);
  tft.drawString("SD", 335, indRow1Y);
  tft.fillCircle(362, indRow1Y, 4, p.sdPresent ? HMI_GREEN : HMI_RED);
  
  // NET 
  tft.setTextColor(p.wifiState > 0 ? HMI_GREEN : HMI_RED, HMI_BG_CARD); 
  tft.drawString("NET", 400, indRow1Y);
  tft.fillCircle(435, indRow1Y, 4, p.wifiState > 0 ? HMI_GREEN : HMI_RED);

  // NTP 
  tft.setTextColor(p.ntpSynced ? HMI_GREEN : HMI_RED, HMI_BG_CARD); 
  tft.drawString("NTP", 335, indRow2Y);
  tft.fillCircle(372, indRow2Y, 4, p.ntpSynced ? HMI_GREEN : HMI_RED);

  // TG 
  bool isSending = (p.lastAlertStatus == 1);
  tft.setTextColor(isSending ? HMI_GREEN : HMI_RED, HMI_BG_CARD); 
  tft.drawString("TG", 400, indRow2Y);
  tft.fillCircle(425, indRow2Y, 4, isSending ? HMI_GREEN : HMI_RED);
  
  tft.setTextDatum(TL_DATUM);
}

// === main render functions ===

void renderScreenLive(DisplayPacket &p) {
  updateLiveDynamic(p);
}

void renderScreenParams(int8_t cursor, NavMode nav, float editVal, DisplayPacket &p) {
  int y = 40;
  
  char rpmBuf[16];
  char isoBuf[16];
  
  if (nav == NAV_EDIT && cursor == 0) {
    sprintf(rpmBuf, "%d", (int)editVal);
  } else {
    sprintf(rpmBuf, "%d", (int)p.motorRPM);
  }
  
  if (nav == NAV_EDIT && cursor == 1) {
    sprintf(isoBuf, "%d", (int)editVal);
  } else {
    sprintf(isoBuf, "%d", p.isoClass);
  }
  
  drawMenuItem(y, "RPM", rpmBuf, cursor == 0, nav == NAV_EDIT && cursor == 0); 
  y += 36;
  
  drawMenuItem(y, "ISO Class", isoBuf, cursor == 1, nav == NAV_EDIT && cursor == 1); 
  y += 36;
  
  drawMenuItem(y, "START CAPTURE", "", cursor == 2, false); 
  y += 36;

  if (nav == NAV_EDIT) {
    sprMenu.fillSprite(HMI_BG_DARK);
    sprMenu.setTextColor(HMI_TEXT_DIM, HMI_BG_DARK); 
    sprMenu.setTextFont(1); 
    sprMenu.setCursor(10, 10);
    sprMenu.print("UP/DN: Change | SELECT: Save");
    sprMenu.pushSprite(10, y);
  } else {
    tft.fillRect(10, y, 460, 30, HMI_BG_DARK);
  }
}

void renderScreenRuntime(int8_t cursor, DisplayPacket &p) {
  int y = 40;
  char axBuf[4];
  sprintf(axBuf, "%d", p.axisMapMode);
  
  drawMenuItem(y, "System Status", ">", cursor == 0, false); y += 36;
  drawMenuItem(y, "Rebaseline", "", cursor == 1, false); y += 36;
  drawMenuItem(y, "Axis Map", axBuf, cursor == 2, false); y += 36;
  drawMenuItem(y, "Theme", currentTheme == 0 ? "Dark" : "Light", cursor == 3, false); y += 36;
  drawMenuItem(y, "MATLAB", p.matlabMode ? "ON" : "OFF", cursor == 4, false); y += 36; 
  drawMenuItem(y, "SD Card", ">", cursor == 5, false); y += 36; 
  drawMenuItem(y, "Reset EEPROM", "", cursor == 6, false); y += 36;
  drawMenuItem(y, "EXIT", "", cursor == 7, false); y += 36;     
}

void renderScreenSD(int8_t cursor, NavMode nav, float editVal, DisplayPacket &p) {
  int y = 40;
  drawMenuItem(y, "Auto Log", p.sdAutoLogStat ? "ON" : "OFF", cursor == 0, false); y += 36;
  
  char intBuf[16];
  if (nav == NAV_EDIT && cursor == 1) {
    sprintf(intBuf, "%lus", INTERVAL_OPTIONS[(int)editVal] / 1000);
  } else {
    sprintf(intBuf, "%lus", p.sdLogIntervalMs / 1000);
  }
  
  drawMenuItem(y, "Interval", intBuf, cursor == 1, nav == NAV_EDIT && cursor == 1); y += 36;
  drawMenuItem(y, "Back", "", cursor == 2, false); y += 36;
}

void renderScreenNetwork(int8_t cursor, DisplayPacket &p) {
  int y = 40;
  
  tft.fillRoundRect(10, y, 460, 70, 4, HMI_BG_CARD); 
  tft.drawRoundRect(10, y, 460, 70, 4, HMI_BORDER);
  tft.setTextColor(HMI_TEXT_DIM, HMI_BG_CARD); 
  tft.setTextFont(2); 
  tft.setCursor(20, y+8);
  
  const char* wStr[] = {"OFF", "CONNECTING", "SYNC", "SENDING"};
  tft.printf("WiFi: %s          ", wStr[p.wifiState]);
  
  tft.setCursor(20, y+28); 
  tft.printf("NTP: %s       ", p.ntpSynced ? "OK" : "NO");
  
  tft.setCursor(20, y+48); 
  tft.printf("Last: %s          ", p.lastSyncStr);
  
  y += 80;
  
  drawMenuItem(y, "Force NTP", "", cursor == 0, false); y += 36;
  drawMenuItem(y, "Back", "", cursor == 1, false); y += 36;
}