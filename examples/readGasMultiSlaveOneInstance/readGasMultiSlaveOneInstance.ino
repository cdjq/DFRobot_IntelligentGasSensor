/*!
 * @file readGasMultiSlaveOneInstance.ino
 * @brief 一条总线上仅用一个库对象，通过setClientSlaveAddr()切换目标从机地址轮询读数（不改传感器EEPROM）。
 * @n 与readGasMultiSlave对比：该例程用三个对象；本例程用一个对象每轮切换_slave。
 * @n RS-485与TTL切换方式同readGasMultiSlave；默认从机列表1、5、3。
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

// 总线上的从机地址，按实际模块修改
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

// RS-485：初始地址应为kSlaveIds[0]
DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlaveIds[0], /*dePin =*/kDePin);
// TTL：注释上一行，取消下一行注释
// DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlaveIds[0]);

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
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, /*rx =*/HOST_RX, /*tx =*/HOST_TX);
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
