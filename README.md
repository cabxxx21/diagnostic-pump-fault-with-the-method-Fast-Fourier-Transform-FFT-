# Diagnostic Pump Fault with FFT and FreeRTOS

An advanced dual-core embedded system architecture built on the ESP32-S3 using FreeRTOS to perform real-time pump fault diagnosis via Fast Fourier Transform (FFT) analysis.

## 🛠️ Hardware Specification & Bus Topology
To maximize data throughput and avoid bus contention, the peripheral communication is optimized across separate SPI buses:
* **Microcontroller:** ESP32-S3 (Dual-Core)
* **Vibration Sensor:** ADXL345 (Connected via dedicated **FSPI** bus)
* **Display & Storage:** ILI9488 TFT Display & SD Card (Connected via **Shared HSPI** bus)
* **Temperature Sensor:** MLX90614 Infrared Thermometer (Connected via I2C)

## 🧠 Dual-Core FreeRTOS Architecture
This project utilizes the dual-core capability of the ESP32-S3 to completely separate heavy mathematical/background tasks from the user interface response:

### 🔹 Core 0: Data Processing & Communication
* `task_fft.ino` - Handles continuous vibration data sampling and high-speed FFT computations.
* `task_mlx.ino` - Non-blocking tasks to monitor pump surface temperature using the MLX90614 sensor.
* `task_logger.ino` - Background management task handling Wi-Fi connectivity, NTP time synchronization, local SD card data logging, and sending remote alert notifications via Telegram Bot API.

### 🔹 Core 1: User Interface & Control
* `ui_task.ino` & `ui_render.ino` - Drives the ILI9488 TFT display rendering engine.
* Handles physics button/navigation interrupts for fluid, zero-lag UI interaction.
* `state_machine.ino` - Synchronizes system state changes based on diagnostic results and user inputs.

## 🔌 Pin Mapping Configuration
*Please manually fill in your physical ESP32-S3 GPIO numbers below:*
* **ADXL345 (FSPI):** MOSI: Pin 4 | MISO: Pin 5 | SCK: 6  | CS: 7
* **ILI9488 & SD Card (Shared HSPI):** MOSI: Pin 11 | MISO: Pin 13 | SCK: Pin 12 | TFT_CS: Pin 10 | SD_CS: Pin 8
* **MLX90614 (I2C):** SDA: Pin 1 | SCL: Pin 2

## 📚 Required Libraries
Ensure these libraries are installed in your Arduino IDE before compilation:
1. `Adafruit_ADXL345`
2. `Adafruit_MLX90614`
3. `TFT_eSPI` (or equivalent ILI9488 driver configured for HSPI)
4. `arduinoFFT`
