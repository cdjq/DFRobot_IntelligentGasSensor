/*!
 * @file DFRobot_IntelligentGasSensor.cpp
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 */
#include "DFRobot_IntelligentGasSensor.h"
#include <stdio.h>

namespace {

constexpr size_t kUnsolicitedFc04Len =
    (size_t)1 + 1 + 1 + (size_t)DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP * 2 + 2;

uint16_t modbusCrc16(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFFu;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8u; ++b)
            crc = (crc & 1u) ? (uint16_t)((crc >> 1) ^ 0xA001u) : (uint16_t)(crc >> 1);
    }
    return crc;
}

bool modbusCrcOk(const uint8_t *adu, size_t len) {
    if (len < 4)
        return false;
    const uint16_t got  = (uint16_t)((uint16_t)adu[len - 1] << 8) | adu[len - 2];
    const uint16_t calc = modbusCrc16(adu, len - 2);
    return got == calc;
}

struct GasCodeRow {
    uint8_t     code;
    const char *type;
    const char *unit;
};

/* 与 SEN07xx SensorDriver.cpp 中 SMX100 表 3 保持一致 */
const GasCodeRow kSmx100GasTable[] = {
    { 0x01, "CH4", "%LEL" },
    { 0x03, "H2S", "ppm" },
    { 0x04, "CO", "ppm" },
    { 0x05, "O2", "%VOL" },
};

const GasCodeRow *findGasRow(uint8_t code) {
    for (unsigned i = 0; i < sizeof(kSmx100GasTable) / sizeof(kSmx100GasTable[0]); i++) {
        if (kSmx100GasTable[i].code == code)
            return &kSmx100GasTable[i];
    }
    return nullptr;
}

static void copyCStr(const char *src, char *dst, size_t dstLen) {
    if (dstLen == 0)
        return;
    if (src == nullptr) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dstLen - 1);
    dst[dstLen - 1] = '\0';
}

} // namespace

bool DFRobot_IntelligentGasSensor::gasCodeToTypeName(uint8_t gasCode, char *buf, size_t bufLen) {
    if (buf == nullptr || bufLen == 0)
        return false;
    const GasCodeRow *row = findGasRow(gasCode);
    if (row == nullptr) {
        buf[0] = '\0';
        return false;
    }
    copyCStr(row->type, buf, bufLen);
    return true;
}

bool DFRobot_IntelligentGasSensor::gasCodeToDefaultUnit(uint8_t gasCode, char *buf, size_t bufLen) {
    if (buf == nullptr || bufLen == 0)
        return false;
    const GasCodeRow *row = findGasRow(gasCode);
    if (row == nullptr) {
        buf[0] = '\0';
        return false;
    }
    copyCStr(row->unit, buf, bufLen);
    return true;
}

void DFRobot_IntelligentGasSensor::gasCodeFormatUnknown(uint8_t gasCode, char *buf, size_t bufLen) {
    if (buf == nullptr || bufLen == 0)
        return;
    (void)snprintf(buf, bufLen, "0x%02X", (unsigned)gasCode);
}

DFRobot_IntelligentGasSensor::DFRobot_IntelligentGasSensor(Stream *s, uint8_t slaveAddr, int dePin)
    : DFRobot_RTU(s, dePin), _slave(slaveAddr), _modbusStream(s) {
    memset(&lastMeasure, 0, sizeof(lastMeasure));
    setTimeoutTimeMs(200);
}

void DFRobot_IntelligentGasSensor::setClientSlaveAddr(uint8_t addr) {
    if (addr >= DFROBOT_IGS_SLAVE_ADDR_MIN && addr <= DFROBOT_IGS_SLAVE_ADDR_MAX)
        _slave = addr;
}

void DFRobot_IntelligentGasSensor::applyTimestampFromRegs(const uint16_t *t) {
    if (t == NULL)
        return;
    lastMeasure.tsYear   = t[DFROBOT_IGS_IN_REG_TS_YEAR];
    lastMeasure.tsMonth  = (uint8_t)(t[DFROBOT_IGS_IN_REG_TS_MONTH] & 0xFFu);
    lastMeasure.tsDay    = (uint8_t)(t[DFROBOT_IGS_IN_REG_TS_DAY] & 0xFFu);
    lastMeasure.tsHour   = (uint8_t)(t[DFROBOT_IGS_IN_REG_TS_HOUR] & 0xFFu);
    lastMeasure.tsMinute = (uint8_t)(t[DFROBOT_IGS_IN_REG_TS_MINUTE] & 0xFFu);
    lastMeasure.tsSecond = (uint8_t)(t[DFROBOT_IGS_IN_REG_TS_SECOND] & 0xFFu);
    if (lastMeasure.tsYear == 0 && lastMeasure.tsMonth == 0 && lastMeasure.tsDay == 0
        && lastMeasure.tsHour == 0 && lastMeasure.tsMinute == 0 && lastMeasure.tsSecond == 0) {
        lastMeasure.timestamp[0] = '\0';
        return;
    }
    (void)snprintf(lastMeasure.timestamp, sizeof(lastMeasure.timestamp), "%04u-%02u-%02u %02u:%02u:%02u",
                   (unsigned)lastMeasure.tsYear, (unsigned)lastMeasure.tsMonth, (unsigned)lastMeasure.tsDay,
                   (unsigned)lastMeasure.tsHour, (unsigned)lastMeasure.tsMinute, (unsigned)lastMeasure.tsSecond);
}

void DFRobot_IntelligentGasSensor::applyGasStringsFromCode(void) {
    uint8_t c = lastMeasure.gasCode;
    if (gasCodeToTypeName(c, lastMeasure.gasType, sizeof(lastMeasure.gasType))) {
        if (!gasCodeToDefaultUnit(c, lastMeasure.unit, sizeof(lastMeasure.unit))) {
            lastMeasure.unit[0] = '?';
            lastMeasure.unit[1] = '\0';
        }
    } else {
        gasCodeFormatUnknown(c, lastMeasure.gasType, sizeof(lastMeasure.gasType));
        lastMeasure.unit[0] = '?';
        lastMeasure.unit[1] = '\0';
    }
}

void DFRobot_IntelligentGasSensor::parseInputTable(const uint16_t *t, uint16_t regCount) {
    memset(&lastMeasure, 0, sizeof(lastMeasure));
    if (t == NULL || regCount < 6u)
        return;

    lastMeasure.pid                = t[DFROBOT_IGS_IN_REG_PID];
    lastMeasure.vid                = t[DFROBOT_IGS_IN_REG_VID];
    lastMeasure.modbusAddr         = (uint8_t)(t[DFROBOT_IGS_IN_REG_MODBUS_ADDR] & 0xFFu);
    lastMeasure.registerMapVersion = t[DFROBOT_IGS_IN_REG_VERSION];

    if (regCount < 12u)
        goto done_valid;
    lastMeasure.boardType = t[DFROBOT_IGS_IN_REG_BOARD_TYPE];
    lastMeasure.status    = t[DFROBOT_IGS_IN_REG_STATUS];
    lastMeasure.gasCode   = (uint8_t)(t[DFROBOT_IGS_IN_REG_GAS_CODE] & 0xFFu);
    lastMeasure.concentrationRaw = (uint32_t)t[DFROBOT_IGS_IN_REG_CONC_LO]
                                   | ((uint32_t)t[DFROBOT_IGS_IN_REG_CONC_HI] << 16);
    lastMeasure.decimalPoint = (uint8_t)(t[DFROBOT_IGS_IN_REG_DECIMAL_PT] & 0xFFu);

    applyGasStringsFromCode();

    if (regCount >= DFROBOT_IGS_INPUT_REG_COUNT)
        applyTimestampFromRegs(t);

done_valid:
    lastMeasure.dataValid = (lastMeasure.status & DFROBOT_IGS_STATUS_VALID_MSK) != 0;
}

uint8_t DFRobot_IntelligentGasSensor::readMeasurement(bool withTimestamp) {
    if (withTimestamp) {
        uint16_t table[DFROBOT_IGS_INPUT_REG_COUNT];
        uint8_t  err = readInputRegister(_slave, 0, table, DFROBOT_IGS_INPUT_REG_COUNT);
        if (err == 0)
            parseInputTable(table, DFROBOT_IGS_INPUT_REG_COUNT);
        return err;
    }
    uint16_t table[DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP];
    uint8_t  err = readInputRegister(_slave, 0, table, DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP);
    if (err == 0)
        parseInputTable(table, DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP);
    return err;
}

float DFRobot_IntelligentGasSensor::getConcentrationFloat(void) const {
    if (lastMeasure.decimalPoint > 12)
        return NAN;
    double v = (double)lastMeasure.concentrationRaw;
    for (uint8_t i = 0; i < lastMeasure.decimalPoint; i++)
        v *= 0.1;
    return (float)v;
}

uint8_t DFRobot_IntelligentGasSensor::readHoldingShadow(uint16_t *regs) {
    if (!regs)
        return 3;
    return readHoldingRegister(_slave, 0, regs, DFROBOT_IGS_HOLDING_REG_COUNT);
}

uint8_t DFRobot_IntelligentGasSensor::writeHoldingReg(uint16_t reg, uint16_t value) {
    return writeHoldingRegister(_slave, reg, value);
}

uint8_t DFRobot_IntelligentGasSensor::commitConfiguration(void) {
    return writeHoldingRegister(_slave, DFROBOT_IGS_HOLD_REG_COMMIT, DFROBOT_IGS_COMMIT_KEY_SAVE_CONFIG);
}

uint8_t DFRobot_IntelligentGasSensor::setUartAutoReport(bool enable, bool commitToEeprom) {
    uint8_t e = writeHoldingReg(DFROBOT_IGS_HOLD_REG_UART_AUTO_REPORT, enable ? 1u : 0u);
    if (e != 0)
        return e;
    if (!commitToEeprom)
        return 0;
    return commitConfiguration();
}

uint8_t DFRobot_IntelligentGasSensor::getAcquireMode(uint8_t *mode) {
    if (mode == nullptr)
        return 3;
    uint16_t hold[DFROBOT_IGS_HOLDING_REG_COUNT];
    const uint8_t err = readHoldingShadow(hold);
    if (err != 0)
        return err;
    const uint16_t v = hold[DFROBOT_IGS_HOLD_REG_UART_AUTO_REPORT];
    if (v == 0u)
        *mode = (uint8_t)ACQUIRE_MODE_PASSIVE;
    else if (v == 1u)
        *mode = (uint8_t)ACQUIRE_MODE_ACTIVE;
    else
        *mode = (uint8_t)ACQUIRE_MODE_UNKNOWN;
    return 0;
}

uint8_t DFRobot_IntelligentGasSensor::setAcquireMode(uint8_t mode, bool commitToEeprom) {
    if (mode != (uint8_t)ACQUIRE_MODE_PASSIVE && mode != (uint8_t)ACQUIRE_MODE_ACTIVE)
        return 3;
    return setUartAutoReport(mode != 0u, commitToEeprom);
}

uint8_t DFRobot_IntelligentGasSensor::parseUnsolicitedInputReadResponse(const uint8_t *adu, size_t len) {
    if (adu == nullptr || len != kUnsolicitedFc04Len)
        return 3;
    if (adu[0] != _slave)
        return 11;
    if (adu[1] != 0x04)
        return 3;
    if (adu[2] != (uint8_t)(DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP * 2u))
        return 3;
    if (!modbusCrcOk(adu, len))
        return 8;

    uint16_t table[DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP];
    for (unsigned i = 0; i < DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP; i++) {
        table[i] = (uint16_t)((uint16_t)adu[3u + i * 2u] << 8) | adu[3u + i * 2u + 1u];
    }
    parseInputTable(table, DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP);
    return 0;
}

uint8_t DFRobot_IntelligentGasSensor::pollUnsolicitedAutoReport(void) {
    if (_modbusStream == nullptr)
        return 9;

    while (_modbusStream->available()) {
        const int c = _modbusStream->read();
        if (c < 0)
            break;
        _autoRepLastMs = millis();
        if (_autoRepLen < sizeof(_autoRepBuf))
            _autoRepBuf[_autoRepLen++] = (uint8_t)c;
        else {
            memmove(_autoRepBuf, _autoRepBuf + 1, sizeof(_autoRepBuf) - 1);
            _autoRepBuf[sizeof(_autoRepBuf) - 1] = (uint8_t)c;
        }
    }

    const uint32_t now = millis();
    if (_autoRepLen == 0)
        return 1;
    if ((uint32_t)(now - _autoRepLastMs) < (uint32_t)DFROBOT_IGS_AUTO_REPORT_IDLE_MS)
        return 1;

    constexpr size_t kAdu = kUnsolicitedFc04Len;
    if (_autoRepLen < kAdu) {
        _autoRepLen = 0;
        return 1;
    }

    for (size_t off = 0; off + kAdu <= _autoRepLen; ++off) {
        const uint8_t pe = parseUnsolicitedInputReadResponse(_autoRepBuf + off, kAdu);
        if (pe == 0) {
            const uint16_t consumed = (uint16_t)(off + kAdu);
            const size_t   tail     = (size_t)_autoRepLen - (size_t)consumed;
            memmove(_autoRepBuf, _autoRepBuf + off + kAdu, tail);
            _autoRepLen = (uint16_t)tail;
            return 0;
        }
    }

    if (_autoRepLen > 80)
        _autoRepLen = 0;
    else
        memmove(_autoRepBuf, _autoRepBuf + 1, --_autoRepLen);
    return 1;
}

uint8_t DFRobot_IntelligentGasSensor::setDeviceAddress(uint8_t newAddr) {
    if (newAddr < DFROBOT_IGS_SLAVE_ADDR_MIN || newAddr > DFROBOT_IGS_SLAVE_ADDR_MAX)
        return 3;
    if (newAddr == _slave)
        return 0;
    uint8_t e = writeHoldingReg(DFROBOT_IGS_HOLD_REG_SLAVE_ADDR, newAddr);
    if (e != 0)
        return e;
    /* 固件在写 0x0006 后立即用新从站号应答，主机必须先切到新地址再发 COMMIT */
    setClientSlaveAddr(newAddr);
    e = commitConfiguration();
    if (e != 0)
        return e;
    return 0;
}

uint8_t DFRobot_IntelligentGasSensor::readMeasurementWithTimestamp(void) {
    return readMeasurement(true);
}
