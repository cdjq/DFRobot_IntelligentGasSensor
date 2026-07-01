/*!
 * @file changeDeviceBaudrate.ino
 * @brief 将传感器Modbus波特率从出厂9600改为19200，之后主机与从站均保持19200通信。
 * @n SEN07xx固件在写保持寄存器0x0003应答后会立即把从站UART切到新波特率；主机若仍用旧波特率发COMMIT会失败。
 * @n 正确顺序：writeDeviceBaudCode → 立刻HOST_SERIAL.begin(新波特率) → commitConfiguration()。
 * @n COMMIT成功表示新线速与EEPROM均已一致；之后loop一直用19200轮询。
 * @n 上传前确认kSlaveAddr及传感器当前为kInitialBaud（默认9600）；RS-485/TTL构造切换同changeDeviceAddress。
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

static const unsigned long kInitialBaud = 9600;

// 目标线速（须与DFROBOT_IGS_BAUD_CODE_*一致）
static const unsigned long kTargetUartBaud = 19200;
static const uint16_t     kTargetBaudCode  = DFROBOT_IGS_BAUD_CODE_19200;

static const uint8_t kSlaveAddr = 1;

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX 36
#define HOST_TX 17
static const int kDePin = 16;
#else
#define HOST_SERIAL Serial1
static const int kDePin = 29;
#endif

DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlaveAddr, /*dePin =*/kDePin);
// TTL: DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlaveAddr);

static void hostSerialBegin(unsigned long baud) {
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.end();
    HOST_SERIAL.begin(baud, SERIAL_8N1, /*rx =*/HOST_RX, /*tx =*/HOST_TX);
#else
    HOST_SERIAL.end();
    HOST_SERIAL.begin(baud);
#endif
}

// writeDeviceBaudCode → HOST_SERIAL.begin(lineBaud) → commitConfiguration()
static uint8_t writeBaudCodeReopenUartThenCommit(uint16_t code, unsigned long lineBaud) {
    uint8_t e = sensor.writeDeviceBaudCode(code);
    if (e != 0)
        return e;
    delay(20);
    hostSerialBegin(lineBaud);
    return sensor.commitConfiguration();
}

static void printOneLine(const char *tag) {
    if (sensor.readGasMeasurementData(true) != 0) {
        if (tag)
            Serial.print(tag);
        Serial.println(F("read error"));
        return;
    }
    if (!sensor.lastMeasure.dataValid) {
        if (tag)
            Serial.print(tag);
        Serial.println(F("waiting for valid data..."));
        return;
    }
    if (tag)
        Serial.print(tag);
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
    hostSerialBegin(kInitialBaud);

    Serial.print(F("Modbus link at "));
    Serial.print(kInitialBaud);
    Serial.println(F(" — checking..."));

    if (sensor.readGasMeasurementData(true) != 0) {
        Serial.println(F("Cannot read. Fix kSlaveAddr / wiring / kInitialBaud."));
        for (;;)
            delay(1000);
    }
    if (!sensor.lastMeasure.dataValid) {
        Serial.println(F("Link OK but measurement not valid yet; wait and retry."));
        for (;;)
            delay(1000);
    }
    printOneLine("Before change: ");

    Serial.println(F("Step: writeDeviceBaudCode -> HOST_SERIAL.begin(target) -> COMMIT..."));

    uint8_t err = writeBaudCodeReopenUartThenCommit(kTargetBaudCode, kTargetUartBaud);
    if (err != 0) {
        Serial.print(F("baud+commit failed, code "));
        Serial.println(err);
        for (;;)
            delay(1000);
    }

    Serial.print(F("COMMIT OK. Sensor + HOST_SERIAL now "));
    Serial.print(kTargetUartBaud);
    Serial.println(F(" — loop will keep this baud."));

    printOneLine("After change: ");
}

void loop() {
    printOneLine(nullptr);
    delay(1000);
}
