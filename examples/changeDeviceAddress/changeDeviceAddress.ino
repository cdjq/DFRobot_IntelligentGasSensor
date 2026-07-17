/*!
 * @file changeDeviceAddress.ino
 * @brief Change the sensor Modbus slave address with setDeviceAddress() and verify it by reading data.
 * @n Set kCurrentSlaveAddr and kNewSlaveAddr before uploading. Valid slave addresses are 1 to 247.
 * @n After a successful change, the sensor responds only at the new address until it is changed again.
 * @n note: The sensor supports RS-485 through an adapter or direct TTL UART wiring. Use 5 V for sensor VCC when wiring directly.
 * @n connected table (ESP32 + UART-to-RS485 module + sensor A/B)
 * ---------------------------------------------------------------------------------------------------------------
 * ESP32 pin | UART-to-RS485 module | Sensor (SEN07xx) |
 *    3.3V   |          VCC         |        --        |
 *    GND    |          GND         |        --        |
 * GPIO17(TX)|          DI          |        --        |
 * GPIO36(RX)|          RO          |        --        |
 * GPIO16    |         DE/RE        |        --        |
 *     --    |           A          |        A         |
 *     --    |           B          |        B         |
 * ---------------------------------------------------------------------------------------------------------------
 * @n connected table (ESP32 direct TTL UART + sensor)
 * ---------------------------------------------------
 * ESP32 pin | Sensor (SEN07xx) |
 *    5V     |       VCC        |
 *    GND    |       GND        |
 * GPIO17(TX)|        RX        |
 * GPIO36(RX)|        TX        |
 * ---------------------------------------------------
 *
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @author [wxzed](xiao.wu@dfrobot.com)
 * @version  V1.0.0
 * @date  2026-05-21
 * @url https://github.com/DFRobot/DFRobot_IntelligentGasSensor
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kBaud = 9600;

static const uint8_t kCurrentSlaveAddr = 5;
static const uint8_t kNewSlaveAddr     = 1;

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX 36
#define HOST_TX 17
static const int kDePin = 16;
#else
#define HOST_SERIAL Serial1
static const int kDePin = 29;
#endif

// RS-485 by default.
DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kCurrentSlaveAddr, /*dePin =*/kDePin);
// TTL: comment the line above and uncomment the line below.
// DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kCurrentSlaveAddr);

void setup() {
    
    Serial.begin(115200);
    delay(2000);
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, /*rx =*/HOST_RX, /*tx =*/HOST_TX);
#else
    HOST_SERIAL.begin(kBaud);
#endif

    Serial.print(F("Talking to slave "));
    Serial.print((unsigned)kCurrentSlaveAddr);
    Serial.println(F(" - checking link..."));

    if (sensor.readGasMeasurementData(true) != 0) {
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

    if (sensor.readGasMeasurementData(true) != 0) {
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
    if (sensor.readGasMeasurementData(true) != 0) {
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
