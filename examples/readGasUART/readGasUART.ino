/*!
 * @file readGasUART.ino
 * @brief Poll SEN07xx intelligent gas sensor concentration through TTL UART without an RS-485 DE pin.
 * @n Cross-connect the host TX/RX pins with the sensor and share GND; default bus settings are 9600 8N1 and Modbus slave address 1.
 * @n note: The sensor HOST UART is 3.3 V logic. Connect only to a 3.3 V host such as ESP32; do not connect directly to 5 V UNO/Mega boards.
 * @n connected table (ESP32 TTL UART)
 * -----------------------------------------------------------------------
 * sensor pin |    ESP32     |
 *     VCC    |    3.3V      |
 *     GND    |    GND       |
 *     RX     | GPIO17(TX2) |
 *     TX     | GPIO36(RX2) |
 * -----------------------------------------------------------------------
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
static const uint8_t         kSlave = 1;

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX 36
#define HOST_TX 17
#else
#define HOST_SERIAL Serial1
#endif

DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlave);

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, /*rx =*/HOST_RX, /*tx =*/HOST_TX);
#else
    HOST_SERIAL.begin(kBaud);
#endif
}

void loop() {
    const uint8_t err = sensor.readGasMeasurementData(true);
    if (err != 0) {
        Serial.print(F("read error, code="));
        Serial.println(err);
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
