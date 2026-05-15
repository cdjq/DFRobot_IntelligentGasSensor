/*!
 * @file DFRobot_IntelligentGasSensorI2C.cpp
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 */
#include "DFRobot_IntelligentGasSensorI2C.h"

namespace {

/** 与 SEN0738 `I2cMaster_GasSlaveRead` 一致：首字节偶发错位时重读 */
constexpr uint8_t  kWhoAmIRetryMax     = 4;
constexpr uint16_t kWhoAmIRetryDelayUs = 400;

} // namespace

DFRobot_IntelligentGasSensorI2C::DFRobot_IntelligentGasSensorI2C(uint8_t addr7, TwoWire *wire)
    : _wire(wire != nullptr ? wire : &Wire),
      _addr7(addr7),
#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
      _gapUs(400),
#else
      _gapUs(250),
#endif
      _verifyWho(true) {
    memset(&lastMeasure, 0, sizeof(lastMeasure));
}

void DFRobot_IntelligentGasSensorI2C::setClientI2cAddr(uint8_t addr7) { _addr7 = addr7; }

int DFRobot_IntelligentGasSensorI2C::rawReadBurst(uint8_t regStart, uint8_t *out, uint8_t len) {
    if (_wire == nullptr || out == nullptr || len == 0)
        return -4;
    _wire->beginTransmission(_addr7);
    _wire->write(regStart);
    const int err = _wire->endTransmission(true);
    if (err != 0)
        return err;
    if (_gapUs != 0)
        delayMicroseconds((uint32_t)_gapUs);
    const size_t got = _wire->requestFrom((int)_addr7, (int)len);
    if (got != (size_t)len) {
        while (_wire->available())
            (void)_wire->read();
        return -2;
    }
    for (uint8_t i = 0; i < len; i++) {
        if (!_wire->available())
            return -3;
        out[i] = (uint8_t)_wire->read();
    }
    return 0;
}

uint8_t DFRobot_IntelligentGasSensorI2C::readMeasurement(bool withTimestamp) {
    const uint8_t n = withTimestamp ? (uint8_t)16u : (uint8_t)9u;
    uint8_t       buf[16]{};
    int           st = 0;

    for (uint8_t attempt = 0; attempt < kWhoAmIRetryMax; attempt++) {
        st = rawReadBurst(0, buf, n);
        if (st == -4)
            return DFROBOT_IGS_I2C_ERR_PARAM;
        if (st == -2)
            return DFROBOT_IGS_I2C_ERR_WIRE_RX_COUNT;
        if (st == -3)
            return DFROBOT_IGS_I2C_ERR_WIRE_AVAIL;
        if (st != 0)
            return DFROBOT_IGS_I2C_ERR_WIRE_TX;

        if (!_verifyWho || buf[0] == 0x07u)
            break;
        if (attempt + 1u < kWhoAmIRetryMax)
            delayMicroseconds((uint32_t)kWhoAmIRetryDelayUs);
    }

    if (_verifyWho && buf[0] != 0x07u)
        return DFROBOT_IGS_I2C_ERR_WHOAMI;

    uint16_t table[DFROBOT_IGS_INPUT_REG_COUNT]{};
    table[DFROBOT_IGS_IN_REG_PID]          = 0;
    table[DFROBOT_IGS_IN_REG_VID]          = DFROBOT_IGS_VID_DFRobot;
    table[DFROBOT_IGS_IN_REG_MODBUS_ADDR]  = (uint16_t)_addr7;
    table[DFROBOT_IGS_IN_REG_VERSION]      = DFROBOT_IGS_REGMAP_VERSION;
    table[DFROBOT_IGS_IN_REG_BOARD_TYPE]   = DFROBOT_IGS_BOARD_UNKNOWN;
    table[DFROBOT_IGS_IN_REG_STATUS]       = (uint16_t)buf[2];
    table[DFROBOT_IGS_IN_REG_GAS_CODE]     = (uint16_t)buf[3];
    table[DFROBOT_IGS_IN_REG_CONC_LO]      = (uint16_t)buf[4] | ((uint16_t)buf[5] << 8);
    table[DFROBOT_IGS_IN_REG_CONC_HI]      = (uint16_t)buf[6] | ((uint16_t)buf[7] << 8);
    table[DFROBOT_IGS_IN_REG_DECIMAL_PT]   = (uint16_t)buf[8];

    if (withTimestamp && n >= 16u) {
        table[DFROBOT_IGS_IN_REG_TS_YEAR]   = (uint16_t)buf[9] | ((uint16_t)buf[10] << 8);
        table[DFROBOT_IGS_IN_REG_TS_MONTH]  = (uint16_t)buf[11];
        table[DFROBOT_IGS_IN_REG_TS_DAY]    = (uint16_t)buf[12];
        table[DFROBOT_IGS_IN_REG_TS_HOUR]   = (uint16_t)buf[13];
        table[DFROBOT_IGS_IN_REG_TS_MINUTE] = (uint16_t)buf[14];
        table[DFROBOT_IGS_IN_REG_TS_SECOND] = (uint16_t)buf[15];
    }

    const uint16_t regCount = withTimestamp ? DFROBOT_IGS_INPUT_REG_COUNT : DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP;
    DFRobot_IntelligentGasSensor::fillLastMeasureFromInputRegs(&lastMeasure, table, regCount);
    return DFROBOT_IGS_I2C_OK;
}

uint8_t DFRobot_IntelligentGasSensorI2C::readMeasurementWithTimestamp(void) { return readMeasurement(true); }

float DFRobot_IntelligentGasSensorI2C::getConcentrationFloat(void) const {
    if (lastMeasure.decimalPoint > 12u)
        return NAN;
    double v = (double)lastMeasure.concentrationRaw;
    for (uint8_t i = 0; i < lastMeasure.decimalPoint; i++)
        v *= 0.1;
    return (float)v;
}

uint8_t DFRobot_IntelligentGasSensorI2C::programI2cAddress(uint8_t newAddr7) {
    if (newAddr7 < 0x08u || newAddr7 > 0x77u)
        return DFROBOT_IGS_I2C_ERR_ADDR_RANGE;

    _wire->beginTransmission(_addr7);
    _wire->write((uint8_t)0x30);
    _wire->write((uint8_t)0x5A);
    if (_wire->endTransmission(true) != 0)
        return DFROBOT_IGS_I2C_ERR_WIRE_TX;
    if (_gapUs != 0)
        delayMicroseconds((uint32_t)_gapUs);

    _wire->beginTransmission(_addr7);
    _wire->write((uint8_t)0x31);
    _wire->write(newAddr7);
    if (_wire->endTransmission(true) != 0)
        return DFROBOT_IGS_I2C_ERR_WIRE_TX;
    if (_gapUs != 0)
        delayMicroseconds((uint32_t)_gapUs);

    _wire->beginTransmission(_addr7);
    _wire->write((uint8_t)0x32);
    _wire->write((uint8_t)0xA5);
    if (_wire->endTransmission(true) != 0)
        return DFROBOT_IGS_I2C_ERR_WIRE_TX;

    _addr7 = newAddr7;
    return DFROBOT_IGS_I2C_OK;
}
