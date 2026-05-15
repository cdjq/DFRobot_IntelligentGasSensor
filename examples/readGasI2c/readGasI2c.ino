/*!
 * @file readGasI2c.ino
 * @brief I2C 主机：`GasI2cSlave` 带墙钟寄存器，固定读时间戳并打印（气体名、浓度两位小数、单位、墙钟字符串）。
 *
 * 接线：主机 SDA/SCL 与从机 I2C 引脚相连并共地；从机默认 7 位地址 0x38。
 * 注意：arduino-pico 从机须「写子地址 + STOP + 短延时 + 读」，本库已处理。
 */
#include <Arduino.h>
#include <Wire.h>
#include <DFRobot_IntelligentGasSensorI2C.h>

static const uint8_t kSlaveAddr7 = 0x38;

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
static const uint8_t kMasterSdaPin = 8;
static const uint8_t kMasterSclPin = 9;
#endif

DFRobot_IntelligentGasSensorI2C gasI2c(kSlaveAddr7, &Wire);

void setup() {
    Serial.begin(115200);
    delay(500);

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
    Wire.setSDA(kMasterSdaPin);
    Wire.setSCL(kMasterSclPin);
#endif
    Wire.begin();
#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
    Wire.setClock(100000);
#endif

#if !defined(ARDUINO_ARCH_RP2040) && !defined(ARDUINO_ARCH_RP2350)
    gasI2c.setStopGapMicros(400);
#endif
}

void loop() {
    if (gasI2c.readMeasurementWithTimestamp() != DFROBOT_IGS_I2C_OK) {
        Serial.println(F("read error"));
        delay(1000);
        return;
    }

    Serial.print(gasI2c.lastMeasure.gasType);
    Serial.print(' ');
    Serial.print(gasI2c.getConcentrationFloat(), 2);
    Serial.print(' ');
    Serial.print(gasI2c.lastMeasure.unit);
    if (gasI2c.lastMeasure.timestamp[0] != '\0') {
        Serial.print(' ');
        Serial.print(gasI2c.lastMeasure.timestamp);
    }
    Serial.println();

    delay(1000);
}
