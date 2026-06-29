#include "globals.h"

// === helper draw menu item ===

void drawMenuItem(int y, const char* label, const char* value, bool isActive, bool isEditing) {
  uint16_t bgColor, txtColor, valColor, accentColor;
  
  if (currentTheme == 0) { // DARK
    bgColor     = isActive ? HMI_BG_CARD : HMI_BG_DARK;
    txtColor    = isActive ? HMI_CYAN    : HMI_TEXT_MAIN;
    valColor    = HMI_YELLOW;
    accentColor = HMI_CYAN;
  } else {                 // LIGHT
    bgColor     = isActive ? HMI_BG_CARD_L : HMI_BG_LIGHT;
    txtColor    = isActive ? 0xFC00        : HMI_TEXT_DARK;
    valColor    = 0x2B5C;
    accentColor = 0xFC00;
  }
  
  sprMenu.fillSprite(bgColor);
  if (isActive) sprMenu.fillRect(0, 0, 3, 30, accentColor);

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
  uint16_t bg = currentTheme == 0 ? HMI_BG_DARK : HMI_BG_LIGHT;
  uint16_t bc = currentTheme == 0 ? HMI_BG_CARD : HMI_BG_CARD_L;
  uint16_t bd = HMI_BORDER;
  uint16_t tx = currentTheme == 0 ? HMI_TEXT_DIM : HMI_TEXT_DARK;
  uint16_t acc = currentTheme == 0 ? HMI_CYAN : 0xFC00;

  tft.fillScreen(bg);
  tft.fillRect(0, 0, 480, 32, bc); 
  tft.drawFastHLine(0, 32, 480, bd);
  
  tft.setTextFont(2); 
  tft.setTextColor(acc, bc); 
  tft.setCursor(10, 8); 
  tft.print("PUMP MONITOR v6.0");

  tft.fillRoundRect(8, 40, 305, 85, 4, bc); 
  tft.drawRoundRect(8, 40, 305, 85, 4, bd);
  tft.setTextColor(tx, bc); 
  tft.setTextFont(1); 
  tft.setCursor(18, 45); 
  tft.print("RADIAL VIBRATION (g)");

  tft.fillRoundRect(8, 130, 305, 85, 4, bc); 
  tft.drawRoundRect(8, 130, 305, 85, 4, bd);
  tft.setTextColor(tx, bc); 
  tft.setTextFont(1); 
  tft.setCursor(18, 135); 
  tft.print("AXIAL VIBRATION (g)");

  tft.fillRoundRect(8, 220, 305, 95, 4, bc); 
  tft.drawRoundRect(8, 220, 305, 95, 4, HMI_GREEN);
  tft.setTextColor(HMI_GREEN, bc); 
  tft.setTextFont(1); 
  tft.setCursor(18, 225); 
  tft.print("DIAGNOSED FAULT");

  tft.fillRoundRect(320, 40, 152, 120, 4, bc); 
  tft.drawRoundRect(320, 40, 152, 120, 4, bd);
  tft.setTextColor(tx); 
  tft.setTextFont(2); 
  tft.setCursor(330, 45); 
  tft.print("RPM");

  tft.fillRoundRect(320, 165, 152, 55, 4, bc); 
  tft.drawRoundRect(320, 165, 152, 55, 4, bd);
  tft.setTextColor(tx); 
  tft.setTextFont(2); 
  tft.setCursor(330, 170); 
  tft.print("TEMP OBJ");

  tft.fillRoundRect(320, 225, 152, 90, 4, bc); 
  tft.drawRoundRect(320, 225, 152, 90, 4, bd);
}

void drawStaticMenuBg(const char* title) {
  uint16_t bg = currentTheme == 0 ? HMI_BG_DARK : HMI_BG_LIGHT;
  uint16_t bc = currentTheme == 0 ? HMI_BG_CARD : HMI_BG_CARD_L;
  uint16_t acc = currentTheme == 0 ? HMI_CYAN : 0xFC00;

  tft.fillScreen(bg);
  tft.fillRect(0, 0, 480, 32, bc);
  tft.drawFastHLine(0, 32, 480, HMI_BORDER);
  tft.setTextFont(2); 
  tft.setTextColor(acc, bc); 
  tft.setCursor(10, 8);
  tft.print(title);
}

// === dynamic updates ===

void updateLiveDynamic(DisplayPacket &p) {
  uint16_t bg = currentTheme == 0 ? HMI_BG_CARD : HMI_BG_CARD_L;
  uint16_t tx = currentTheme == 0 ? HMI_TEXT_MAIN : HMI_TEXT_DARK;
  uint16_t ac = currentTheme == 0 ? HMI_CYAN : 0x2B5C;

  const char* sn[] = {"INIT","COLD","INPUT","BOOT","STAB","CAP","VAL","RUN","STBY","ALRT","DNG"};
  uint16_t sc = (p.sysState >= ST_ALERT) ? HMI_RED : (p.sysState == ST_RUNNING ? HMI_GREEN : (currentTheme == 0 ? HMI_YELLOW : 0xFC00));
  
  static uint8_t lastDrawnState = 99;
  if (lastDrawnState != p.sysState) {
      tft.fillRect(400, 5, 80, 27, bg); 
      tft.setTextFont(2); tft.setTextColor(sc, bg); tft.setTextDatum(TR_DATUM);
      tft.drawString(sn[p.sysState], 470, 10); tft.setTextDatum(TL_DATUM);
      lastDrawnState = p.sysState;
  }

  tft.setTextFont(6); tft.setTextColor(tx, bg);
  tft.setCursor(18, 65); tft.printf("%.3f  ", p.liveRMS_Rad);
  tft.setCursor(18, 155); tft.printf("%.3f  ", p.liveRMS_Ax);
  
  tft.setTextFont(2); tft.setTextColor(ac, bg);
  tft.setCursor(185, 60); tft.printf("1X:%.3f", p.live1X_Rad);
  tft.setCursor(195, 80); tft.printf("2X:%.3f", p.live2X_Rad);
  tft.setCursor(205, 100); tft.printf("3X:%.3f", p.live3X_Rad);
  tft.setCursor(185, 150); tft.printf("1X:%.3f", p.live1X_Ax);
  tft.setCursor(195, 170); tft.printf("2X:%.3f", p.live2X_Ax);
  tft.setCursor(205, 190); tft.printf("3X:%.3f", p.live3X_Ax);

  uint16_t fc = (p.confirmedFault == F_NONE) ? HMI_GREEN : HMI_RED;
  static uint16_t lfc = HMI_GREEN;
  if (lfc != fc) { tft.fillRect(9, 236, 303, 78, bg); lfc = fc; }
  tft.drawRoundRect(8, 220, 305, 95, 4, fc);
  tft.setTextColor(fc, bg); tft.setTextFont(1); tft.setCursor(18, 225); tft.print("DIAGNOSED FAULT");
  tft.setTextFont(4); tft.setCursor(18, 245); 
  const char* fn[] = {"NORMAL","UNBAL","MIS_A","MIS_P","LOOSE","STOP"};
  tft.printf("%-7s  ", fn[p.confirmedFault]);

  tft.setTextFont(7); tft.setTextColor(ac, bg); tft.setTextDatum(MC_DATUM);
  char rs[8]; sprintf(rs, "%4.0f", p.motorRPM); tft.drawString(rs, 385, 100); tft.setTextDatum(TL_DATUM);

  tft.setTextFont(4);
  tft.setTextColor(p.tempObj > 0.0f ? (currentTheme == 0 ? HMI_YELLOW : 0xFC00) : HMI_RED, bg);
  tft.setCursor(330, 190); tft.printf("%.1fC     ", p.tempObj);
}

// === main render functions ===

void renderScreenLive(DisplayPacket &p) {
  updateLiveDynamic(p);
}

void renderScreenParams(int8_t c, NavMode n, float ev, DisplayPacket &p) {
  int y = 40; char b1[16], b2[16];
  sprintf(b1, "%d", (n == NAV_EDIT && c == 0) ? (int)ev : (int)p.motorRPM);
  sprintf(b2, "%d", (n == NAV_EDIT && c == 1) ? (int)ev : (int)p.isoClass);
  drawMenuItem(y, "RPM", b1, c == 0, n == NAV_EDIT && c == 0); y += 36;
  drawMenuItem(y, "ISO Class", b2, c == 1, n == NAV_EDIT && c == 1); y += 36;
  drawMenuItem(y, "START CAPTURE", "", c == 2, false);
}

void renderScreenRuntime(int8_t c, DisplayPacket &p) {
  int y = 40; char axBuf[4];
  sprintf(axBuf, "%d", p.axisMapMode);
  drawMenuItem(y, "System Status", ">", c == 0, false); y += 36;
  drawMenuItem(y, "Rebaseline", "", c == 1, false); y += 36;
  drawMenuItem(y, "Axis Map", axBuf, c == 2, false); y += 36;
  drawMenuItem(y, "Theme", currentTheme == 0 ? "Dark" : "Light", c == 3, false); y += 36; 
  drawMenuItem(y, "MATLAB", p.matlabMode ? "ON" : "OFF", c == 4, false); y += 36; 
  drawMenuItem(y, "SD Card", ">", c == 5, false); y += 36; 
  drawMenuItem(y, "Reset EEPROM", "", c == 6, false); y += 36;
  drawMenuItem(y, "EXIT", "", c == 7, false); y += 36;
}

void renderScreenSD(int8_t c, NavMode n, float ev, DisplayPacket &p) {
  int y = 40;
  drawMenuItem(y, "Auto Log", p.sdAutoLogStat ? "ON" : "OFF", c == 0, false); y += 36;
  char intBuf[16];
  if (n == NAV_EDIT && c == 1) sprintf(intBuf, "%lus", INTERVAL_OPTIONS[(int)ev] / 1000);
  else sprintf(intBuf, "%lus", p.sdLogIntervalMs / 1000);
  drawMenuItem(y, "Interval", intBuf, c == 1, n == NAV_EDIT && c == 1); y += 36;
  drawMenuItem(y, "Back", "", c == 2, false); y += 36;
}

void renderScreenNetwork(int8_t c, DisplayPacket &p) {
  int y = 40;
  uint16_t bgCard = currentTheme == 0 ? HMI_BG_CARD  : HMI_BG_CARD_L;
  uint16_t border = currentTheme == 0 ? HMI_BORDER   : HMI_BORDER;
  uint16_t txtDim = currentTheme == 0 ? HMI_TEXT_DIM : HMI_TEXT_DARK;

  tft.fillRoundRect(10, y, 460, 70, 4, bgCard); 
  tft.drawRoundRect(10, y, 460, 70, 4, border);
  tft.setTextColor(txtDim, bgCard); 
  tft.setTextFont(2); 
  tft.setCursor(20, y+8);
  
  const char* wStr[] = {"OFF", "CONNECTING", "SYNC", "SENDING"};
  tft.printf("WiFi: %s          ", wStr[p.wifiState]);
  tft.setCursor(20, y+28); tft.printf("NTP: %s       ", p.ntpSynced ? "OK" : "NO");
  tft.setCursor(20, y+48); tft.printf("Last: %s          ", p.lastSyncStr);
  
  y += 80;
  drawMenuItem(y, "Force NTP", "", c == 0, false); y += 36;
  drawMenuItem(y, "Back", "", c == 1, false); y += 36;
}