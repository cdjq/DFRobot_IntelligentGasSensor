/*!
 * @file probeSleepWake.ino
 * @brief 通过Modbus保持寄存器0x0005控制SEN07xx探头运行/休眠（立即生效，不写EEPROM）。
 * @n 串口监视器发送单字符（无需换行）：S=休眠，W=唤醒，P=查询模式，M=读一次浓度（休眠时可能无效）。
 * @n 接线同readGasRS485（默认）或readGasUART，下方注释切换RS-485/TTL构造。
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
static const uint8_t         kSlave = 1;

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
DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlave, /*dePin =*/kDePin);
// TTL：注释上一行，取消下一行注释
// DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlave);

static void printProbeMode(void) {
    bool asleep = false;
    uint8_t e = sensor.readProbeSleepMode(&asleep);
    if (e != 0) {
        Serial.print(F("readProbeSleepMode err "));
        Serial.println(e);
        return;
    }
    Serial.println(asleep ? F("probe: SLEEP") : F("probe: RUN"));
}

static void printMeasurementLine(void) {
    if (sensor.readMeasurementWithTimestamp() != 0) {
        Serial.println(F("readMeasurement error"));
        return;
    }
    if (!sensor.lastMeasure.dataValid) {
        Serial.println(F("measurement: (not valid — probe SLEEP or no frame yet)"));
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
}

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, /*rx =*/HOST_RX, /*tx =*/HOST_TX);
#else
    HOST_SERIAL.begin(kBaud);
#endif
    Serial.println(F("S=sleep  W=wake  P=probe mode  M=read gas"));
    printProbeMode();
}

void loop() {
    if (!Serial.available())
        return;
    const char c = (char)Serial.read();
    while (Serial.available())
        (void)Serial.read();

    switch (c) {
    case 'S':
    case 's':
        if (sensor.setProbeSleep(true) != 0)
            Serial.println(F("setProbeSleep(SLEEP) failed"));
        else
            Serial.println(F("setProbeSleep: SLEEP"));
        break;
    case 'W':
    case 'w':
        if (sensor.setProbeSleep(false) != 0)
            Serial.println(F("setProbeSleep(RUN) failed"));
        else
            Serial.println(F("setProbeSleep: RUN"));
        break;
    case 'P':
    case 'p':
        printProbeMode();
        break;
    case 'M':
    case 'm':
        printMeasurementLine();
        break;
    default:
        Serial.println(F("use S W P M"));
        break;
    }
}
