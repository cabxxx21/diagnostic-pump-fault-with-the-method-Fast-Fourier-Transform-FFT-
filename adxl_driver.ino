#include "globals.h"

void adxl_writeRegister(uint8_t reg, uint8_t value) {
  spiFSPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE3));
  digitalWrite(ADXL_CS, LOW); 
  
  spiFSPI.transfer(reg); 
  spiFSPI.transfer(value);
  
  digitalWrite(ADXL_CS, HIGH); 
  spiFSPI.endTransaction();
}

uint8_t adxl_readRegister(uint8_t reg) {
  spiFSPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE3));
  digitalWrite(ADXL_CS, LOW); 
  
  spiFSPI.transfer(reg | 0x80);
  uint8_t val = spiFSPI.transfer(0x00);
  
  digitalWrite(ADXL_CS, HIGH); 
  spiFSPI.endTransaction(); 
  
  return val;
}

void adxl_readAccel(float* x, float* y, float* z) {
  uint8_t buf[6] = {0};
  
  spiFSPI.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE3));
  digitalWrite(ADXL_CS, LOW); 
  
  spiFSPI.transfer(0xF2);
  for (int i = 0; i < 6; i++) {
    buf[i] = spiFSPI.transfer(0x00);
  }
  
  digitalWrite(ADXL_CS, HIGH); 
  spiFSPI.endTransaction();

  int16_t rx = (int16_t)((buf[1] << 8) | buf[0]);
  int16_t ry = (int16_t)((buf[3] << 8) | buf[2]);
  int16_t rz = (int16_t)((buf[5] << 8) | buf[4]);
  
  *x = rx * 0.004f * 9.80665f; 
  *y = ry * 0.004f * 9.80665f; 
  *z = rz * 0.004f * 9.80665f;
}