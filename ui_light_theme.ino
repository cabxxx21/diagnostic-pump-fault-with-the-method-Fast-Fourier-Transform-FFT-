//ui_light_theme.ino
#include "globals.h"

// === helper draw menu item LIGHT ===
void drawMenuItemLight(int y, const char* label, const char* value, bool isActive, bool isEditing) {
  uint16_t bgColor = isActive ? LT_BG_CARD : LT_BG_MAIN;
  uint16_t txtColor = isActive ? LT_ACCENT_ORG : LT_TEXT_MAIN;
  uint16_t valColor = LT_ACCENT_BLU;
  
  sprMenu.fillSprite(bgColor);
  
  if (isActive) {
    sprMenu.fillRect(0, 0, 3, 30, LT_ACCENT_ORG); 
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

// === static backgrounds LIGHT ===
void drawStaticLiveBgLight() {
  tft.fillScreen(LT_BG_MAIN);
  
  // header
  tft.fillRect(0, 0, 480, 32, LT_BG_CARD); 
  tft.drawFastHLine(0, 32, 480, LT_BORDER);
  tft.setTextFont(2); 
  tft.setTextColor(LT_ACCENT_ORG, LT_BG_CARD); 
  tft.setCursor(10, 8); 
  tft.print("PUMP MONITOR v6.0");

  // box radial
  tft.fillRoundRect(8, 40, 305, 85, 4, LT_BG_CARD); 
  tft.drawRoundRect(8, 40, 305, 85, 4, LT_BORDER);
  tft.setTextColor(LT_TEXT_DIM, LT_BG_CARD); 
  tft.setTextFont(1); 
  tft.setCursor(18, 45); 
  tft.print("RADIAL VIBRATION (g)");

  // box axial
  tft.fillRoundRect(8, 130, 305, 85, 4, LT_BG_CARD); 
  tft.drawRoundRect(8, 130, 305, 85, 4, LT_BORDER);
  tft.setTextColor(LT_TEXT_DIM, LT_BG_CARD); 
  tft.setTextFont(1); 
  tft.setCursor(18, 135); 
  tft.print("AXIAL VIBRATION (g)");

  // box fault
  tft.fillRoundRect(8, 220, 305, 95, 4, LT_BG_CARD); 
  tft.drawRoundRect(8, 220, 305, 95, 4, HMI_GREEN);
  tft.setTextColor(HMI_GREEN, LT_BG_CARD); 
  tft.setTextFont(1); 
  tft.setCursor(18, 225); 
  tft.print("DIAGNOSED FAULT");

  // box rpm
  tft.fillRoundRect(320, 40, 152, 120, 4, LT_BG_CARD); 
  tft.drawRoundRect(320, 40, 152, 120, 4, LT_BORDER);
  tft.setTextColor(LT_TEXT_DIM); 
  tft.setTextFont(2); 
  tft.setCursor(330, 45); 
  tft.print("RPM");

  // box temp
  tft.fillRoundRect(320, 165, 152, 55, 4, LT_BG_CARD); 
  tft.drawRoundRect(320, 165, 152, 55, 4, LT_BORDER);
  tft.setTextColor(LT_TEXT_DIM); 
  tft.setTextFont(2); 
  tft.setCursor(330, 170); 
  tft.print("TEMP OBJ");

  // box indikator
  tft.fillRoundRect(320, 225, 152, 90, 4, LT_BG_CARD); 
  tft.drawRoundRect(320, 225, 152, 90, 4, LT_BORDER);
}

void drawStaticMenuBgLight(const char* title) {
  tft.fillScreen(LT_BG_MAIN);
  tft.fillRect(0, 0, 480, 32, LT_BG_CARD);
  tft.drawFastHLine(0, 32, 480, LT_BORDER);
  tft.setTextFont(2); 
  tft.setTextColor(LT_ACCENT_ORG, LT_BG_CARD); 
  tft.setCursor(10, 8);
  tft.print(title);
}

// === dynamic updates LIGHT ===
void updateLiveDynamicLight(DisplayPacket &p) {
  const char* sn[] = {"INIT","COLD","INPUT","BOOT"," STAB "," CAP ","VAL"," RUN ","STBY","ALRT","DNG"};
  uint16_t stateClr = (p.sysState >= ST_ALERT) ? HMI_RED : (p.sysState == ST_RUNNING ? HMI_GREEN : LT_ACCENT_ORG); // Oranye saat stabil/running
  
  static uint8_t lastDrawnState = 99;
  if (lastDrawnState != p.sysState) {
      tft.fillRect(400, 5, 80, 27, LT_BG_CARD); 
      tft.setTextFont(2); 
      tft.setTextColor(stateClr, LT_BG_CARD);
      tft.setTextDatum(TR_DATUM);
      tft.drawString(sn[p.sysState], 470, 10);
      tft.setTextDatum(TL_DATUM);
      lastDrawnState = p.sysState;
  }

  // RADIAL
  tft.setTextFont(6); 
  tft.setTextColor(LT_TEXT_MAIN, LT_BG_CARD);
  tft.setCursor(18, 65); 
  tft.printf("%.3f  ", p.liveRMS_Rad);
  
  tft.setTextFont(2); 
  tft.setTextColor(LT_ACCENT_BLU, LT_BG_CARD); 
  tft.setCursor(185, 60);   
  tft.printf("1X:%.3f", p.live1X_Rad);
  tft.setCursor(195, 80); 
  tft.printf("2X:%.3f", p.live2X_Rad);
  tft.setCursor(205, 100); 
  tft.printf("3X:%.3f", p.live3X_Rad);

  // AXIAL
  tft.setTextFont(6); 
  tft.setTextColor(LT_TEXT_MAIN, LT_BG_CARD);
  tft.setCursor(18, 155); 
  tft.printf("%.3f  ", p.liveRMS_Ax);
  
  tft.setTextFont(2); 
  tft.setTextColor(LT_ACCENT_BLU, LT_BG_CARD);
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
    tft.fillRect(9, 236, 303, 78, LT_BG_CARD); 
    lastFaultColor = faultColor;
  }
  
  tft.drawRoundRect(8, 220, 305, 95, 4, faultColor);
  tft.setTextColor(faultColor, LT_BG_CARD);
  tft.setTextFont(1); 
  tft.setCursor(18, 225); 
  tft.print("DIAGNOSED FAULT");
  
  const char* fn[] = {"NORMAL","UNBAL","MIS_A","MIS_P","LOOSE","STOP"};
  tft.setTextFont(4); 
  tft.setTextColor(faultColor, LT_BG_CARD);
  tft.setCursor(18, 245); 
  tft.printf("%-7s  ", fn[p.confirmedFault]);

  // RPM 
  tft.setTextFont(7); 
  tft.setTextColor(LT_ACCENT_BLU, LT_BG_CARD);
  tft.setTextDatum(MC_DATUM);
  char rpmStr[8];
  sprintf(rpmStr, "%4.0f", p.motorRPM);
  tft.drawString(rpmStr, 385, 100); 
  tft.setTextDatum(TL_DATUM);

  // Temp
  tft.setTextFont(4);
  if (p.tempObj > 0.0f) {
    tft.setTextColor(LT_ACCENT_ORG, LT_BG_CARD);
    tft.setCursor(330, 190); 
    tft.printf("%.1fC     ", p.tempObj);
  } else {
    tft.setTextColor(HMI_RED, LT_BG_CARD);
    tft.setCursor(330, 190); 
    tft.print("ERROR   "); 
  }

  // INDICATOR
  tft.setTextFont(2);
  int16_t indRow1Y = 248;
  int16_t indRow2Y = 275;
  
  tft.setTextDatum(ML_DATUM);
  
  // SD
  tft.setTextColor(p.sdPresent ? HMI_GREEN : HMI_RED, LT_BG_CARD); 
  tft.drawString("SD", 335, indRow1Y);
  tft.fillCircle(362, indRow1Y, 4, p.sdPresent ? HMI_GREEN : HMI_RED);
  
  // NET
  tft.setTextColor(p.wifiState > 0 ? HMI_GREEN : HMI_RED, LT_BG_CARD); 
  tft.drawString("NET", 400, indRow1Y);
  tft.fillCircle(435, indRow1Y, 4, p.wifiState > 0 ? HMI_GREEN : HMI_RED);

  // NTP
  tft.setTextColor(p.ntpSynced ? HMI_GREEN : HMI_RED, LT_BG_CARD); 
  tft.drawString("NTP", 335, indRow2Y);
  tft.fillCircle(372, indRow2Y, 4, p.ntpSynced ? HMI_GREEN : HMI_RED);

  // TG
  bool isSending = (p.lastAlertStatus == 1);
  tft.setTextColor(isSending ? HMI_GREEN : HMI_RED, LT_BG_CARD); 
  tft.drawString("TG", 400, indRow2Y);
  tft.fillCircle(425, indRow2Y, 4, isSending ? HMI_GREEN : HMI_RED);
  
  tft.setTextDatum(TL_DATUM);
}