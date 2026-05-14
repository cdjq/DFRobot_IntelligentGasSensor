/*!
 * @file readGasModbus.ino
 * @brief Minimal example: read gas type and concentration over Modbus RTU.
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kModbusBaud = 9600;

#if defined(ARDUINO_ARCH_ESP32)
#define MODBUS_SERIAL Serial2
#define MODBUS_RX 16
#define MODBUS_TX 17
#define RS485_DE_PIN 4
#define USE_RS485 0
#if USE_RS485
DFRobot_IntelligentGasSensor sensor(&MODBUS_SERIAL, RS485_DE_PIN, 1);
#else
DFRobot_IntelligentGasSensor sensor(&MODBUS_SERIAL, 1);
#endif
#else
#define MODBUS_SERIAL Serial1
DFRobot_IntelligentGasSensor sensor(&MODBUS_SERIAL, 1);
#endif

void setup() {
    Serial.begin(115200);
    delay(500);
#if defined(ARDUINO_ARCH_ESP32)
    MODBUS_SERIAL.begin(kModbusBaud, SERIAL_8N1, MODBUS_RX, MODBUS_TX);
#else
    MODBUS_SERIAL.begin(kModbusBaud);
#endif
    sensor.setTimeoutTimeMs(150);
}

void loop() {
    if (sensor.readMeasurement() != 0) {
        Serial.println(F("read error"));
        delay(1000);
        return;
    }
    Serial.print(sensor.lastMeasure.gasType);
    Serial.print(' ');
    Serial.print(sensor.getConcentrationFloat(), 2);
    Serial.print(' ');
    Serial.println(sensor.lastMeasure.unit);
    delay(1000);
}
