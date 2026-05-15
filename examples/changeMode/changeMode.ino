/*!
 * @file changeMode.ino
 * @brief On boot: flip gas acquire mode (passive ↔ active), then print current mode every second.
 * @details Set @ref kModbusSlave and UART pins like `readGasModbus.ino`.
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kBaud = 9600;
static const uint8_t       kModbusSlave = 5;

#if defined(ARDUINO_ARCH_ESP32)
#define SENSOR_SERIAL Serial2
#define SENSOR_RX 16
#define SENSOR_TX 17
#else
#define SENSOR_SERIAL Serial1
#endif

DFRobot_IntelligentGasSensor sensor(&SENSOR_SERIAL, kModbusSlave);

static void printAcquireMode(uint8_t m) {
    if (m == sensor.ACQUIRE_MODE_ACTIVE)
        Serial.println(F("ACTIVE"));
    else if (m == sensor.ACQUIRE_MODE_PASSIVE)
        Serial.println(F("PASSIVE"));
    else
        Serial.println(F("INVALID"));
}

void setup() {
    Serial.begin(115200);
    delay(500);
#if defined(ARDUINO_ARCH_ESP32)
    SENSOR_SERIAL.begin(kBaud, SERIAL_8N1, SENSOR_RX, SENSOR_TX);
#else
    SENSOR_SERIAL.begin(kBaud);
#endif

    uint8_t cur = sensor.ACQUIRE_MODE_PASSIVE;
    if (sensor.getAcquireMode(&cur) != 0) {
        Serial.println(F("read failed"));
        return;
    }

    Serial.print(F("current: "));
    printAcquireMode(cur);

    const uint8_t next = (cur == sensor.ACQUIRE_MODE_ACTIVE) ? sensor.ACQUIRE_MODE_PASSIVE : sensor.ACQUIRE_MODE_ACTIVE;
    Serial.print(F("will set: "));
    printAcquireMode(next);

    if (sensor.setAcquireMode(next, true) != 0) {
        Serial.println(F("change: FAILED"));
        return;
    }
    Serial.println(F("change: OK"));
}

void loop() {
    uint8_t m = sensor.ACQUIRE_MODE_PASSIVE;
    if (sensor.getAcquireMode(&m) != 0) {
        Serial.println(F("read failed"));
    } else {
        printAcquireMode(m);
    }
    delay(1000);
}
