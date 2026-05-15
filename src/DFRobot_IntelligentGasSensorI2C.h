/*!
 * @file DFRobot_IntelligentGasSensorI2C.h
 * @brief I2C 主机：读取烧录 SEN07xx 固件侧 `GasI2cSlave` 的气体测量（与 Modbus 输入表语义对齐）。
 *
 * @details
 * 须先对所用总线调用 `Wire.begin()`（或 `Wire1.begin()` 等），再构造本类并 @ref readMeasurement。
 * 与 arduino-pico 从机配合时：本库采用 **写子地址 + STOP** 后延时再 `requestFrom`，避免 Repeated START 导致读指针错位（见 SEN0738 `GasI2cSlave` 说明）。
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 */
#ifndef __DFRobot_IntelligentGasSensorI2C_H__
#define __DFRobot_IntelligentGasSensorI2C_H__

#include "DFRobot_IntelligentGasSensor.h"
#include <Wire.h>

/** I2C 传输成功 */
#define DFROBOT_IGS_I2C_OK 0u
/** `Wire.endTransmission` 非 0（总线 NACK 等） */
#define DFROBOT_IGS_I2C_ERR_WIRE_TX 1u
/** `requestFrom` 返回字节数与请求不符 */
#define DFROBOT_IGS_I2C_ERR_WIRE_RX_COUNT 2u
/** 读段 `available` 不足 */
#define DFROBOT_IGS_I2C_ERR_WIRE_AVAIL 3u
/** 参数非法（空指针等） */
#define DFROBOT_IGS_I2C_ERR_PARAM 4u
/** 开启 WHO_AM_I 校验时多次读仍非 0x07（多为从机与 Wire 时序偶发错位；可调 @ref setStopGapMicros 或关闭校验） */
#define DFROBOT_IGS_I2C_ERR_WHOAMI 5u
/** @ref programI2cAddress 新地址不在 0x08～0x77 */
#define DFROBOT_IGS_I2C_ERR_ADDR_RANGE 6u

class DFRobot_IntelligentGasSensorI2C {
public:
    /**
     * @param addr7 从机 7 位地址（SEN0738 默认一般为 0x38）
     * @param wire  总线实例，默认 `&Wire`
     */
    explicit DFRobot_IntelligentGasSensorI2C(uint8_t addr7 = 0x38, TwoWire *wire = &Wire);

    /** 仅改主机侧目标 7 位地址（不写从机 EEPROM） */
    void setClientI2cAddr(uint8_t addr7);
    uint8_t getClientI2cAddr(void) const { return _addr7; }

    /** 写 STOP 与读段之间的间隔（微秒）；RP2040 默认约 400，其它约 250。仍偶发 @ref DFROBOT_IGS_I2C_ERR_WHOAMI 时可再略调大 */
    void setStopGapMicros(uint16_t us) { _gapUs = us; }
    uint16_t getStopGapMicros(void) const { return _gapUs; }

    /** 为 true（默认）时要求寄存器 0x00 为 0x07；失败前会短时重读数次（与 SEN0738 I2C 示例一致）。不需要该校验可置 false */
    void setVerifyWhoAmI(bool on) { _verifyWho = on; }

    /**
     * @brief 从 I2C 读测量并填充 @ref lastMeasure（与 Modbus 译码一致）
     * @param withTimestamp false：读到小数位；true：再读墙钟 0x09～0x0F
     * @return @ref DFROBOT_IGS_I2C_OK 或上述错误码
     */
    uint8_t readMeasurement(bool withTimestamp = false);

    uint8_t readMeasurementWithTimestamp(void);

    float getConcentrationFloat(void) const;

    DFRobot_IntelligentGasSensorMeasure_t lastMeasure;

    /**
     * @brief 按 `GasI2cSlave` 序列 0x30/0x31/0x32 将新 7 位地址写入从机 EEPROM；成功后本对象内部目标地址同步为新值。
     * @return 0 成功；@ref DFROBOT_IGS_I2C_ERR_ADDR_RANGE 等
     */
    uint8_t programI2cAddress(uint8_t newAddr7);

private:
    int rawReadBurst(uint8_t regStart, uint8_t *out, uint8_t len);

    TwoWire *_wire;
    uint8_t  _addr7;
    uint16_t _gapUs;
    bool     _verifyWho;
};

#endif
