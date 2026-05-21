/*!
 * @file changeDeviceAddress.ino
 * @brief 用setDeviceAddress()修改传感器Modbus从机地址并读数验证。
 * @n 上传前设置kCurrentSlaveAddr与kNewSlaveAddr（1~247）；RS-485与TTL构造切换同readGasMultiSlave。
 * @n 修改成功后传感器仅在新地址应答，直至再次修改。
 * @n note: 传感器对外仅RS-485端子A、B；ESP32的TX/RX/DE接UART转RS485模块，模块A/B再接传感器。
 * @n connected table (ESP32 + UART转RS485模块 + 传感器A/B)
 * ---------------------------------------------------------------------------------------------------------------
 * ESP32 pin  | UART转RS485模块 | 传感器(SEN07xx) |
 *    3.3V    |      VCC        |        —        |
 *    GND     |      GND        |        —        |
 * GPIO17(TX)|       DI        |        —        |
 * GPIO36(RX)|       RO        |        —        |
 * GPIO16    |     DE/RE       |        —        |
 *     —     |       A         |        A        |
 *     —     |       B         |        B        |
 * ---------------------------------------------------------------------------------------------------------------
 *
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @author [wxzed](xiao.wu@dfrobot.com)
 * @version  V1.0.0
 * @date  2026-05-21
 * @https://github.com/DFRobot/DFRobot_IntelligentGasSensor
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

// RS-485（默认）
DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kCurrentSlaveAddr, /*dePin =*/kDePin);
// TTL：注释上一行，取消下一行注释
// DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kCurrentSlaveAddr);

void setup() {
    Serial.begin(115200);

#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, /*rx =*/HOST_RX, /*tx =*/HOST_TX);
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
