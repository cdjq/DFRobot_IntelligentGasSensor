/*!
 * @file changeDeviceAddress.ino
 * @brief Change the sensor Modbus slave ID using setDeviceAddress(), then read once to verify.
 *
 * Before uploading:
 * - Set `kCurrentSlaveAddr` to the address your sensor uses now (factory default is often 1).
 * - Set `kNewSlaveAddr` to the desired ID (1–247, must not clash with another device on the same bus).
 * - Wire the Modbus UART / RS-485 like in readGasModbus (same MODBUS_SERIAL pins).
 *
 * After a successful change, the sensor answers only on the new ID until you change it again.
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kModbusBaud = 9600;

/** Address the sensor is using *before* you run this sketch (change to match your module). */
static const uint8_t kCurrentSlaveAddr = 1;

/** Address you want after the change (1–247). */
static const uint8_t kNewSlaveAddr = 5;

#if defined(ARDUINO_ARCH_ESP32)
#define MODBUS_SERIAL Serial2
#define MODBUS_RX 16
#define MODBUS_TX 17
#define RS485_DE_PIN 4
#define USE_RS485 0
#if USE_RS485
DFRobot_IntelligentGasSensor sensor(&MODBUS_SERIAL, kCurrentSlaveAddr, RS485_DE_PIN);
#else
DFRobot_IntelligentGasSensor sensor(&MODBUS_SERIAL, kCurrentSlaveAddr);
#endif
#else
#define MODBUS_SERIAL Serial1
DFRobot_IntelligentGasSensor sensor(&MODBUS_SERIAL, kCurrentSlaveAddr);
#endif

void setup() {
    Serial.begin(115200);
    delay(500);

#if defined(ARDUINO_ARCH_ESP32)
    MODBUS_SERIAL.begin(kModbusBaud, SERIAL_8N1, MODBUS_RX, MODBUS_TX);
#else
    MODBUS_SERIAL.begin(kModbusBaud);
#endif

    Serial.print(F("Talking to slave "));
    Serial.print((unsigned)kCurrentSlaveAddr);
    Serial.println(F(" — checking link..."));

    if (sensor.readMeasurement() != 0) {
        Serial.println(F("Cannot read at current address. Fix kCurrentSlaveAddr / wiring / baud."));
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

    if (sensor.readMeasurement() != 0) {
        Serial.println(F("Warning: read after change failed."));
    } else {
        Serial.print(F("Verify read: "));
        Serial.print(sensor.lastMeasure.gasType);
        Serial.print(' ');
        Serial.print(sensor.getConcentrationFloat(), 2);
        Serial.print(' ');
        Serial.println(sensor.lastMeasure.unit);
    }

    Serial.println(F("Loop will keep printing readings on the NEW address."));
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
