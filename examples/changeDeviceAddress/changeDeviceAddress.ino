/*!
 * @file changeDeviceAddress.ino
 * @brief Change the sensor Modbus slave ID using setDeviceAddress(), then read once to verify.
 *
 * Before uploading:
 * - Set kCurrentSlaveAddr / kNewSlaveAddr (1–247).
 * - RS-485 (default) vs TTL: comment one sensor line, uncomment the other (same as readGasMultiSlave).
 *
 * ESP32: RX=36, TX=17, DE=16.  RP2040 (SEN07xx): Serial1, DE=29.
 *
 * After a successful change, the sensor answers only on the new ID until you change it again.
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kBaud = 9600;

static const uint8_t kCurrentSlaveAddr = 1;
static const uint8_t kNewSlaveAddr     = 5;

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX 36
#define HOST_TX 17
static const int kDePin = 16;
#else
#define HOST_SERIAL Serial1
static const int kDePin = 29;
#endif

// RS-485 (default)
DFRobot_IntelligentGasSensor sensor(&HOST_SERIAL, kCurrentSlaveAddr, kDePin);
// TTL UART: comment the line above, uncomment below
// DFRobot_IntelligentGasSensor sensor(&HOST_SERIAL, kCurrentSlaveAddr);

void setup() {
    Serial.begin(115200);

#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, HOST_RX, HOST_TX);
#else
    HOST_SERIAL.begin(kBaud);
#endif

    Serial.print(F("Talking to slave "));
    Serial.print((unsigned)kCurrentSlaveAddr);
    Serial.println(F(" — checking link..."));

    if (sensor.readMeasurementWithTimestamp() != 0) {
        Serial.println(F("Cannot read at current address. Fix kCurrentSlaveAddr / wiring / baud."));
        for (;;)
            delay(1000);
    }
    if (!sensor.lastMeasure.dataValid) {
        Serial.println(F("Link OK but measurement not valid yet; check sensor / wait and retry."));
        for (;;)
            delay(1000);
    }
    Serial.println(F("Read OK. Changing address..."));

    uint8_t err = sensor.setDeviceAddress(kNewSlaveAddr);
    if (err != 0) {
        Serial.print(F("setDeviceAddress failed, code "));
        Serial.println(err);
        for (;;)
            delay(1000);
    }

    Serial.print(F("Done. New slave ID is "));
    Serial.print((unsigned)sensor.getClientSlaveAddr());
    Serial.println(F(" (saved to sensor EEPROM)."));

    if (sensor.readMeasurementWithTimestamp() != 0) {
        Serial.println(F("Warning: read after change failed."));
    } else if (!sensor.lastMeasure.dataValid) {
        Serial.println(F("Verify read: link OK, data not valid yet."));
    } else {
        Serial.print(F("Verify read: "));
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
    }

    Serial.println(F("Loop will keep printing readings on the NEW address."));
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
