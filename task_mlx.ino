//task_mlx.ino
#include "globals.h"

// === task sensor suhu mlx ===

void TaskMLX_Code(void *pvParameters) {
  for (;;) {
    if (mlxOK) {
      float tObj = mlx.readObjectTempC();
      float tAmb = mlx.readAmbientTempC();
      
      if (!isnan(tObj)) {
        tempObj = tObj;
      }
      
      if (!isnan(tAmb)) {
        tempAmb = tAmb;
      }
    }
    
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}