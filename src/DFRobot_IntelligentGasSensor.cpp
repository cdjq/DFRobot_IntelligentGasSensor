/*!
 * @file DFRobot_IntelligentGasSensor.cpp
 * @brief Modbus RTU host driver for DFRobot intelligent gas sensors (SEN07xx).
 *
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @author [wxzed](xiao.wu@dfrobot.com)
 * @version  V1.0.0
 * @date  2026-05-21
 * @https://github.com/DFRobot/DFRobot_IntelligentGasSensor
 */
#include "DFRobot_IntelligentGasSensor.h"
#include <stdio.h>

namespace {

struct GasCodeRow {
    uint8_t     code;
    const char *type;
    const char *unit;
};

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
    : DFRobot_RTU(s, dePin), _slave(slaveAddr), _readRetries(DFROBOT_IGS_DEFAULT_READ_RETRIES) {
    memset(&lastMeasure, 0, sizeof(lastMeasure));
    setTimeoutTimeMs(DFROBOT_IGS_DEFAULT_TIMEOUT_MS);
}

void DFRobot_IntelligentGasSensor::setReadRetryCount(uint8_t count) {
    _readRetries = count;
}

void DFRobot_IntelligentGasSensor::setClientSlaveAddr(uint8_t addr) {
    if (addr >= DFROBOT_IGS_SLAVE_ADDR_MIN && addr <= DFROBOT_IGS_SLAVE_ADDR_MAX)
        _slave = addr;
}

void DFRobot_IntelligentGasSensor::applyTimestampToMeasure(DFRobot_IntelligentGasSensorMeasure_t *m, const uint16_t *t) {
    if (m == nullptr || t == NULL)
        return;
    m->tsYear   = t[DFROBOT_IGS_IN_REG_TS_YEAR];
    m->tsMonth  = (uint8_t)(t[DFROBOT_IGS_IN_REG_TS_MONTH] & 0xFFu);
    m->tsDay    = (uint8_t)(t[DFROBOT_IGS_IN_REG_TS_DAY] & 0xFFu);
    m->tsHour   = (uint8_t)(t[DFROBOT_IGS_IN_REG_TS_HOUR] & 0xFFu);
    m->tsMinute = (uint8_t)(t[DFROBOT_IGS_IN_REG_TS_MINUTE] & 0xFFu);
    m->tsSecond = (uint8_t)(t[DFROBOT_IGS_IN_REG_TS_SECOND] & 0xFFu);
    if (m->tsYear == 0 && m->tsMonth == 0 && m->tsDay == 0 && m->tsHour == 0 && m->tsMinute == 0 && m->tsSecond == 0) {
        m->timestamp[0] = '\0';
        return;
    }
    if (m->tsYear < 2000 || m->tsYear > 2099 || m->tsMonth < 1 || m->tsMonth > 12 || m->tsDay < 1 || m->tsDay > 31 ||
        m->tsHour > 23 || m->tsMinute > 59 || m->tsSecond > 59) {
        m->timestamp[0] = '\0';
        return;
    }
    (void)snprintf(m->timestamp, sizeof(m->timestamp), "%04u-%02u-%02u %02u:%02u:%02u",
                   (unsigned)m->tsYear, (unsigned)m->tsMonth, (unsigned)m->tsDay,
                   (unsigned)m->tsHour, (unsigned)m->tsMinute, (unsigned)m->tsSecond);
}

void DFRobot_IntelligentGasSensor::applyGasToMeasure(DFRobot_IntelligentGasSensorMeasure_t *m) {
    if (m == nullptr)
        return;
    uint8_t c = m->gasCode;
    if (gasCodeToTypeName(c, m->gasType, sizeof(m->gasType))) {
        if (!gasCodeToDefaultUnit(c, m->unit, sizeof(m->unit))) {
            m->unit[0] = '?';
            m->unit[1] = '\0';
        }
    } else {
        gasCodeFormatUnknown(c, m->gasType, sizeof(m->gasType));
        m->unit[0] = '?';
        m->unit[1] = '\0';
    }
}

void DFRobot_IntelligentGasSensor::fillLastMeasureFromInputRegs(DFRobot_IntelligentGasSensorMeasure_t *out,
                                                                const uint16_t *t, uint16_t regCount) {
    if (out == nullptr)
        return;
    memset(out, 0, sizeof(*out));
    if (t == NULL || regCount < 6u)
        return;

    out->pid                = t[DFROBOT_IGS_IN_REG_PID];
    out->vid                = t[DFROBOT_IGS_IN_REG_VID];
    out->modbusAddr         = (uint8_t)(t[DFROBOT_IGS_IN_REG_MODBUS_ADDR] & 0xFFu);
    out->registerMapVersion = t[DFROBOT_IGS_IN_REG_VERSION];

    if (regCount < 12u)
        goto done_valid;
    out->status    = t[DFROBOT_IGS_IN_REG_STATUS];
    out->gasCode   = (uint8_t)(t[DFROBOT_IGS_IN_REG_GAS_CODE] & 0xFFu);
    out->concentrationRaw = (uint32_t)t[DFROBOT_IGS_IN_REG_CONC_LO] | ((uint32_t)t[DFROBOT_IGS_IN_REG_CONC_HI] << 16);
    out->decimalPoint     = (uint8_t)(t[DFROBOT_IGS_IN_REG_DECIMAL_PT] & 0xFFu);

    applyGasToMeasure(out);

    if (regCount >= DFROBOT_IGS_INPUT_REG_COUNT)
        applyTimestampToMeasure(out, t);

done_valid:
    out->dataValid = (out->status & DFROBOT_IGS_STATUS_VALID_MSK) != 0;
}

uint8_t DFRobot_IntelligentGasSensor::readGasMeasurementData(bool withTimestamp) {
    const uint16_t regCount = withTimestamp ? DFROBOT_IGS_INPUT_REG_COUNT : DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP;
    uint16_t       table[DFROBOT_IGS_INPUT_REG_COUNT];
    uint8_t        err      = eRTU_RECV_ERROR;

    for (uint8_t attempt = 0;; attempt++) {
        err = readInputRegister(_slave, 0, table, regCount);
        if (err == 0)
            break;
        if (err != eRTU_EXCEPTION_CRC_ERROR && err != eRTU_RECV_ERROR)
            break;
        if (attempt >= _readRetries)
            break;
        delay(DFROBOT_IGS_READ_RETRY_DELAY_MS);
    }

    if (err == 0)
        fillLastMeasureFromInputRegs(&lastMeasure, table, regCount);
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

uint8_t DFRobot_IntelligentGasSensor::writeHoldingReg(uint16_t reg, uint16_t value) {
    return writeHoldingRegister(_slave, reg, value);
}

uint8_t DFRobot_IntelligentGasSensor::commitConfiguration(void) {
    return writeHoldingRegister(_slave, DFROBOT_IGS_HOLD_REG_COMMIT, DFROBOT_IGS_COMMIT_KEY_SAVE_CONFIG);
}

uint8_t DFRobot_IntelligentGasSensor::setDeviceAddress(uint8_t newAddr) {
    if (newAddr < DFROBOT_IGS_SLAVE_ADDR_MIN || newAddr > DFROBOT_IGS_SLAVE_ADDR_MAX)
        return 3;
    if (newAddr == _slave)
        return 0;
    uint8_t e = writeHoldingReg(DFROBOT_IGS_HOLD_REG_SLAVE_ADDR, newAddr);
    if (e != 0)
        return e;
    setClientSlaveAddr(newAddr);
    e = commitConfiguration();
    if (e != 0)
        return e;
    return 0;
}

uint8_t DFRobot_IntelligentGasSensor::writeDeviceBaudCode(uint16_t code) {
    if (code < DFROBOT_IGS_BAUD_CODE_MIN || code > DFROBOT_IGS_BAUD_CODE_MAX)
        return 3;
    return writeHoldingReg(DFROBOT_IGS_HOLD_REG_BAUD_CODE, code);
}

uint8_t DFRobot_IntelligentGasSensor::setDeviceBaudCode(uint16_t code) {
    uint8_t e = writeDeviceBaudCode(code);
    if (e != 0)
        return e;
    return commitConfiguration();
}

uint8_t DFRobot_IntelligentGasSensor::setProbeSleep(bool sleep) {
    const uint16_t v = sleep ? DFROBOT_IGS_PROBE_SLEEP_SLEEP : DFROBOT_IGS_PROBE_SLEEP_RUN;
    return writeHoldingReg(DFROBOT_IGS_HOLD_REG_PROBE_SLEEP, v);
}

uint8_t DFRobot_IntelligentGasSensor::readProbeSleepMode(bool *outSleep) {
    if (outSleep == nullptr)
        return 3;
    uint16_t v = 0;
    uint8_t   e = readHoldingRegister(_slave, DFROBOT_IGS_HOLD_REG_PROBE_SLEEP, &v, 1u);
    if (e != 0)
        return e;
    *outSleep = (v != 0);
    return 0;
}
