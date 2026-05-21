/*!
 * @file readGasMultiSlave.ino
 * @brief 一条RS-485总线上用三个库对象（共享UART与DE）轮询多个Modbus从机。
 * @n gas1/gas2/gas3共用HOST_SERIAL与kDePin，从机地址不同；默认1、5、3，请按实际模块修改。
 * @n TTL与RS-485：注释/取消注释下方对应的三行构造即可切换。
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
DFRobot_IntelligentGasSensor gas1(/*s =*/&HOST_SERIAL, /*slaveAddr =*/1, /*dePin =*/kDePin);
DFRobot_IntelligentGasSensor gas2(/*s =*/&HOST_SERIAL, /*slaveAddr =*/5, /*dePin =*/kDePin);
DFRobot_IntelligentGasSensor gas3(/*s =*/&HOST_SERIAL, /*slaveAddr =*/3, /*dePin =*/kDePin);

// TTL：注释上面三行，取消下面三行注释
// DFRobot_IntelligentGasSensor gas1(/*s =*/&HOST_SERIAL, /*slaveAddr =*/1);
// DFRobot_IntelligentGasSensor gas2(/*s =*/&HOST_SERIAL, /*slaveAddr =*/5);
// DFRobot_IntelligentGasSensor gas3(/*s =*/&HOST_SERIAL, /*slaveAddr =*/3);

static void printOne(DFRobot_IntelligentGasSensor &g) {
    Serial.print(F("  ID "));
    if (g.getClientSlaveAddr() < 10)
        Serial.print(' ');
    Serial.print(g.getClientSlaveAddr());
    Serial.print(F("  |  "));
    if (g.readMeasurementWithTimestamp() != 0) {
        Serial.println(F("read fail"));
        return;
    }
    if (!g.lastMeasure.dataValid) {
        Serial.println(F("  (no valid data yet)"));
        return;
    }
    if (g.lastMeasure.timestamp[0]) {
        Serial.print('[');
        Serial.print(g.lastMeasure.timestamp);
        Serial.print(F("]  "));
    }
    Serial.print(g.lastMeasure.gasType);
    Serial.print(F("  "));
    Serial.print(g.getConcentrationFloat(), 2);
    Serial.print(F("  "));
    Serial.println(g.lastMeasure.unit);
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
    printOne(gas1);
    printOne(gas2);
    printOne(gas3);
    Serial.println();
    delay(500);
}
