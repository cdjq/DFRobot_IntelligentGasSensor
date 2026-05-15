/*!
 * @file readGasMultiSlave.ino
 * @brief 一条主机口 UART 上三个从站：三个对象 `gas1` `gas2` `gas3`，下面循环里各读各打。TTL 写 `(串口, 从站号)`；RS-485 写 `(串口, 从站号, DE)`。
 * RS-485：`DFRobot_IntelligentGasSensor(&SENSOR_SERIAL, 从站地址, DE引脚)`；TTL 省略第三参（默认无 DE）。
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kBaud = 9600;

#if defined(ARDUINO_ARCH_ESP32)
#define SENSOR_SERIAL Serial2
#define SENSOR_RX 16
#define SENSOR_TX 17
#else
#define SENSOR_SERIAL Serial1
#endif

DFRobot_IntelligentGasSensor gas1(&SENSOR_SERIAL, 1);
DFRobot_IntelligentGasSensor gas2(&SENSOR_SERIAL, 5);
DFRobot_IntelligentGasSensor gas3(&SENSOR_SERIAL, 3);

static void printOne(DFRobot_IntelligentGasSensor &g) {
    Serial.print(F("  ID "));
    if (g.getClientSlaveAddr() < 10)
        Serial.print(' ');
    Serial.print(g.getClientSlaveAddr());
    Serial.print(F("  |  "));
    if (g.readMeasurement() != 0) {
        Serial.println(F("read fail"));
        return;
    }
    Serial.print(g.lastMeasure.gasType);
    Serial.print(F("  "));
    Serial.print(g.getConcentrationFloat(), 2);
    Serial.print(F("  "));
    Serial.println(g.lastMeasure.unit);
}

void setup() {
    Serial.begin(115200);
    delay(500);
#if defined(ARDUINO_ARCH_ESP32)
    SENSOR_SERIAL.begin(kBaud, SERIAL_8N1, SENSOR_RX, SENSOR_TX);
#else
    SENSOR_SERIAL.begin(kBaud);
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
