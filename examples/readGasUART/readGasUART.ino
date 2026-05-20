/*!
 * @file readGasUART.ino
 * @brief Basic poll over TTL UART (no RS-485 DE pin).
 *
 * Wiring: host TX/RX <-> sensor host UART (cross TX/RX), common GND.
 * Default: 9600 8N1, Modbus slave address 1.
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kBaud = 9600;
static const uint8_t         kSlave = 1;

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX 36
#define HOST_TX 17
#else
#define HOST_SERIAL Serial1
#endif

DFRobot_IntelligentGasSensor sensor(&HOST_SERIAL, kSlave);

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, HOST_RX, HOST_TX);
#else
    HOST_SERIAL.begin(kBaud);
#endif
}

void loop() {
    if (sensor.readMeasurementWithTimestamp() != 0) {
        Serial.println(F("read error"));
        delay(1000);
        return;
    }
    if (!sensor.lastMeasure.dataValid) {
        Serial.println(F("waiting for valid measurement..."));
        delay(1000);
        return;
    }
    if (sensor.lastMeasure.timestamp[0]) {
        Serial.print('[');
        Serial.print(sensor.lastMeasure.timestamp);
        Serial.print(F("] "));
    }
    Serial.print(sensor.lastMeasure.gasType);
    Serial.print(' ');
    Serial.print(sensor.getConcentrationFloat(), 2);
    Serial.print(' ');
    Serial.println(sensor.lastMeasure.unit);
    delay(1000);
}
