/*!
 * @file readGasMultiSlaveOneInstance.ino
 * @brief Poll multiple Modbus slaves on one bus using a **single** `DFRobot_IntelligentGasSensor`
 *        and `setClientSlaveAddr()` to switch the target ID (no EEPROM change on sensors).
 *
 * Compare with `readGasMultiSlave`: that sketch uses three objects sharing the same UART;
 * this one uses one object and switches `_slave` each round.
 *
 * RS-485 (default) vs TTL: same comment toggle as `readGasMultiSlave`.
 * ESP32: RX=36, TX=17, DE=16.  RP2040 (SEN07xx): Serial1, DE=29.
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kBaud = 9600;

/** Modbus slave IDs on the bus — edit to match your modules */
static const uint8_t kSlaveIds[] = { 1, 5, 3 };
static const size_t kSlaveCount = sizeof(kSlaveIds) / sizeof(kSlaveIds[0]);

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX 36
#define HOST_TX 17
static const int kDePin = 16;
#else
#define HOST_SERIAL Serial1
static const int kDePin = 29;
#endif

// RS-485: initial ID should be kSlaveIds[0] (first poll target)
DFRobot_IntelligentGasSensor sensor(&HOST_SERIAL, kSlaveIds[0], kDePin);
// TTL: comment line above, uncomment below
// DFRobot_IntelligentGasSensor sensor(&HOST_SERIAL, kSlaveIds[0]);

static void readAndPrint(void) {
    uint8_t id = sensor.getClientSlaveAddr();
    Serial.print(F("  ID "));
    if (id < 10)
        Serial.print(' ');
    Serial.print(id);
    Serial.print(F("  |  "));
    if (sensor.readMeasurementWithTimestamp() != 0) {
        Serial.println(F("read fail"));
        return;
    }
    if (!sensor.lastMeasure.dataValid) {
        Serial.println(F("  (no valid data yet)"));
        return;
    }
    if (sensor.lastMeasure.timestamp[0]) {
        Serial.print('[');
        Serial.print(sensor.lastMeasure.timestamp);
        Serial.print(F("]  "));
    }
    Serial.print(sensor.lastMeasure.gasType);
    Serial.print(F("  "));
    Serial.print(sensor.getConcentrationFloat(), 2);
    Serial.print(F("  "));
    Serial.println(sensor.lastMeasure.unit);
}

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, HOST_RX, HOST_TX);
#else
    HOST_SERIAL.begin(kBaud);
#endif
}

void loop() {
    Serial.println(F("----------"));
    for (size_t i = 0; i < kSlaveCount; i++) {
        sensor.setClientSlaveAddr(kSlaveIds[i]);
        readAndPrint();
    }
    Serial.println();
    delay(500);
}
