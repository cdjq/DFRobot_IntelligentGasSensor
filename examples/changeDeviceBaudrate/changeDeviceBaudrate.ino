/*!
 * @file changeDeviceBaudrate.ino
 * @brief Change sensor Modbus baud from factory 9600 to 19200, then keep communicating at 19200.
 *
 * SEN07xx 固件在写 0x0003 应答之后会**立即**把从站 Modbus UART 切到新波特率；主机若仍用旧波特率发 COMMIT 会失败。
 * 因此顺序必须是：`writeDeviceBaudCode` → **立刻**把 HOST_SERIAL 改成相同波特率 → `commitConfiguration()`。
 * COMMIT 若返回成功，说明整条链（新线速 + EEPROM）一致。
 *
 * Flow: 9600 → writeDeviceBaudCode(19200) → begin(19200) → COMMIT → 之后 **一直** 用 19200 轮询读数。
 *
 * 传感器 EEPROM 会保持 19200；其它默认 9600 的例程需先把 `HOST_SERIAL.begin` 改为 19200，或用 USB `setbaud` 等改回。
 *
 * Before uploading:
 * - Set kSlaveAddr and ensure the sensor really talks at kInitialBaud (default 9600).
 * - RS-485 vs TTL: same pattern as changeDeviceAddress (comment/uncomment sensor constructor).
 *
 * ESP32: RX=36, TX=17, DE=16.  RP2040 (SEN07xx): Serial1, DE=29.
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kInitialBaud = 9600;

/** 目标线速（须与 DFROBOT_IGS_BAUD_CODE_* 一致） */
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

DFRobot_IntelligentGasSensor sensor(&HOST_SERIAL, kSlaveAddr, kDePin);
// TTL: DFRobot_IntelligentGasSensor sensor(&HOST_SERIAL, kSlaveAddr);

static void hostSerialBegin(unsigned long baud) {
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(baud, SERIAL_8N1, HOST_RX, HOST_TX);
#else
    HOST_SERIAL.begin(baud);
#endif
}

/** 调用 `writeDeviceBaudCode` → `HOST_SERIAL.begin(lineBaud)` → `commitConfiguration()`（与固件“写后立即切线速”一致）。 */
static uint8_t writeBaudCodeReopenUartThenCommit(uint16_t code, unsigned long lineBaud) {
    uint8_t e = sensor.writeDeviceBaudCode(code);
    if (e != 0)
        return e;
    delay(20);
    hostSerialBegin(lineBaud);
    return sensor.commitConfiguration();
}

/** @param tag 前缀；可为 nullptr（ESP32 上勿传 F()：F 为 __FlashStringHelper*，与 const char* 不兼容） */
static void printOneLine(const char *tag) {
    if (sensor.readMeasurementWithTimestamp() != 0) {
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

    if (sensor.readMeasurementWithTimestamp() != 0) {
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
