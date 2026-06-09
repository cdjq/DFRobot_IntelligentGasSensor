/*!
 * @file DFRobot_IntelligentGasSensor.h
 * @brief Modbus RTU host driver for DFRobot intelligent gas sensors (SEN07xx).
 *
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @author [wxzed](xiao.wu@dfrobot.com)
 * @version  V1.0.0
 * @date  2026-05-21
 * @https://github.com/DFRobot/DFRobot_IntelligentGasSensor
 */
#ifndef __DFRobot_IntelligentGasSensor_H__
#define __DFRobot_IntelligentGasSensor_H__

#include "Arduino.h"
#include "DFRobot_RTU.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

#define DFROBOT_IGS_VID_DFRobot ((uint16_t)0x3343)
#define DFROBOT_IGS_PID_SEN0742 ((uint16_t)0xC742)
#define DFROBOT_IGS_PID_SEN0746 ((uint16_t)0xC746)

#define DFROBOT_IGS_REGMAP_VERSION ((uint16_t)0x0100)

#define DFROBOT_IGS_STATUS_VALID_MSK ((uint16_t)(1u << 0))

#define DFROBOT_IGS_SLAVE_ADDR_MIN 1u
#define DFROBOT_IGS_SLAVE_ADDR_MAX 247u

#define DFROBOT_IGS_COMMIT_KEY_SAVE_CONFIG ((uint16_t)0xA501)

#define DFROBOT_IGS_IN_REG_PID          0x0000u
#define DFROBOT_IGS_IN_REG_VID          0x0001u
#define DFROBOT_IGS_IN_REG_MODBUS_ADDR  0x0002u
#define DFROBOT_IGS_IN_REG_RESERVED0    0x0003u
#define DFROBOT_IGS_IN_REG_RESERVED1    0x0004u
#define DFROBOT_IGS_IN_REG_VERSION      0x0005u
#define DFROBOT_IGS_IN_REG_RESERVED2    0x0006u
#define DFROBOT_IGS_IN_REG_STATUS       0x0007u
#define DFROBOT_IGS_IN_REG_GAS_CODE     0x0008u
#define DFROBOT_IGS_IN_REG_CONC_LO      0x0009u
#define DFROBOT_IGS_IN_REG_CONC_HI      0x000Au
#define DFROBOT_IGS_IN_REG_DECIMAL_PT    0x000Bu
#define DFROBOT_IGS_IN_REG_TS_YEAR       0x000Cu
#define DFROBOT_IGS_IN_REG_TS_MONTH      0x000Du
#define DFROBOT_IGS_IN_REG_TS_DAY        0x000Eu
#define DFROBOT_IGS_IN_REG_TS_HOUR       0x000Fu
#define DFROBOT_IGS_IN_REG_TS_MINUTE     0x0010u
#define DFROBOT_IGS_IN_REG_TS_SECOND     0x0011u
#define DFROBOT_IGS_IN_REG_LAST          DFROBOT_IGS_IN_REG_TS_SECOND
#define DFROBOT_IGS_INPUT_REG_COUNT      (DFROBOT_IGS_IN_REG_LAST + 1u)
#define DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP DFROBOT_IGS_IN_REG_TS_YEAR

#define DFROBOT_IGS_HOLD_REG_RESERVED0  0x0000u
#define DFROBOT_IGS_HOLD_REG_RESERVED1  0x0001u
#define DFROBOT_IGS_HOLD_REG_RESERVED2  0x0002u
#define DFROBOT_IGS_HOLD_REG_BAUD_CODE    0x0003u
#define DFROBOT_IGS_HOLD_REG_PARITY_STOP  0x0004u
#define DFROBOT_IGS_HOLD_REG_RESERVED3  0x0005u /**<Probe sleep holding register: 0=RUN, 1=SLEEP; immediate, not saved to EEPROM*/
#define DFROBOT_IGS_HOLD_REG_PROBE_SLEEP  DFROBOT_IGS_HOLD_REG_RESERVED3
#define DFROBOT_IGS_PROBE_SLEEP_RUN       0u
#define DFROBOT_IGS_PROBE_SLEEP_SLEEP     1u
#define DFROBOT_IGS_HOLD_REG_SLAVE_ADDR   0x0006u
#define DFROBOT_IGS_HOLD_REG_COMMIT       0x0007u
#define DFROBOT_IGS_HOLDING_REG_COUNT     0x0008u

#define DFROBOT_IGS_BAUD_CODE_2400   ((uint16_t)1)
#define DFROBOT_IGS_BAUD_CODE_4800   ((uint16_t)2)
#define DFROBOT_IGS_BAUD_CODE_9600   ((uint16_t)3)
#define DFROBOT_IGS_BAUD_CODE_14400  ((uint16_t)4)
#define DFROBOT_IGS_BAUD_CODE_19200  ((uint16_t)5)
#define DFROBOT_IGS_BAUD_CODE_38400  ((uint16_t)6)
#define DFROBOT_IGS_BAUD_CODE_57600  ((uint16_t)7)
#define DFROBOT_IGS_BAUD_CODE_115200 ((uint16_t)8)
#define DFROBOT_IGS_BAUD_CODE_MIN    DFROBOT_IGS_BAUD_CODE_2400 /**<Minimum baud code for holding register 0x0003*/
#define DFROBOT_IGS_BAUD_CODE_MAX    DFROBOT_IGS_BAUD_CODE_115200 /**<Maximum baud code for holding register 0x0003*/

#define DFROBOT_IGS_PARITY_NONE  ((uint16_t)0u)
#define DFROBOT_IGS_PARITY_ODD   ((uint16_t)1u)
#define DFROBOT_IGS_PARITY_EVEN  ((uint16_t)2u)
#define DFROBOT_IGS_STOPBIT_1    ((uint16_t)0u)
#define DFROBOT_IGS_STOPBIT_2    ((uint16_t)(1u << 2))

/** Default Modbus response timeout (ms); SEN07xx may block on CSV flash flush for ~1 s */
#define DFROBOT_IGS_DEFAULT_TIMEOUT_MS  ((uint32_t)1000u)
/** Extra read attempts after CRC/recv failure before returning error */
#define DFROBOT_IGS_DEFAULT_READ_RETRIES  ((uint8_t)3u)
/** Delay between read retries (ms) */
#define DFROBOT_IGS_READ_RETRY_DELAY_MS   ((uint16_t)100u)

typedef struct {
    uint16_t pid;
    uint16_t vid;
    uint8_t  modbusAddr;
    uint16_t registerMapVersion;
    uint16_t status;
    uint8_t  gasCode;
    uint32_t concentrationRaw;
    uint8_t  decimalPoint;
    char     gasType[13];
    char     unit[9];
    uint16_t tsYear;
    uint8_t  tsMonth, tsDay, tsHour, tsMinute, tsSecond;
    char     timestamp[21];
    bool     dataValid;
} DFRobot_IntelligentGasSensorMeasure_t;

class DFRobot_IntelligentGasSensor : public DFRobot_RTU {
public:
/**
 * @brief Construct the gas sensor driver.
 * @param s:  Pointer to the UART Stream used for Modbus RTU.
 * @param slaveAddr:  Modbus slave address. Range: 1~247 (0x01~0xF7).
 * @param dePin:  RS-485 DE pin; pull low to receive, pull high to send. Use -1 for TTL UART (no DE).
 */
    DFRobot_IntelligentGasSensor(Stream *s, uint8_t slaveAddr, int dePin = -1);

/**
 * @brief Set the Modbus slave address used by subsequent read/write calls (host-side only, no EEPROM).
 * @param addr:  Modbus slave address. Range: 1~247 (0x01~0xF7).
 */
    void setClientSlaveAddr(uint8_t addr);

/**
 * @brief Get the current Modbus slave address used by this object.
 * @return Modbus slave address (1~247).
 */
    uint8_t getClientSlaveAddr(void) const { return _slave; }

/**
 * @brief Set extra retries after transient CRC/recv errors on readGasMeasurementData().
 * @param count:  Additional attempts after the first failure (0 = single try).
 */
    void setReadRetryCount(uint8_t count);

/**
 * @brief Get extra read retries configured for readGasMeasurementData().
 * @return Additional attempts after the first failure.
 */
    uint8_t getReadRetryCount(void) const { return _readRetries; }

/**
 * @brief Parse input register table into a measurement structure.
 * @param out:  Output measurement structure.
 * @param t:  Input register array (starting at address 0).
 * @param regCount:  Number of registers in t (12 without timestamp, 18 with timestamp).
 */
    static void fillLastMeasureFromInputRegs(DFRobot_IntelligentGasSensorMeasure_t *out, const uint16_t *t,
                                             uint16_t regCount);

/**
 * @brief Read gas measurement from the sensor (FC04 input registers).
 * @param withTimestamp:  false: read registers 0~11 only; true: read 0~17 including wall-clock timestamp.
 * @return Exception code:
 * @n      0 : sucess.
 * @n      1 or eRTU_EXCEPTION_ILLEGAL_FUNCTION : Illegal function.
 * @n      2 or eRTU_EXCEPTION_ILLEGAL_DATA_ADDRESS: Illegal data address.
 * @n      3 or eRTU_EXCEPTION_ILLEGAL_DATA_VALUE:  Illegal data value.
 * @n      4 or eRTU_EXCEPTION_SLAVE_FAILURE:  Slave failure.
 * @n      8 or eRTU_EXCEPTION_CRC_ERROR:  CRC check error.
 * @n      9 or eRTU_RECV_ERROR:  Receive packet error.
 * @n      10 or eRTU_MEMORY_ERROR: Memory error.
 * @n      11 or eRTU_ID_ERROR: Broadcasr address or error ID
 */
    uint8_t readGasMeasurementData(bool withTimestamp = false);

/**
 * @brief Convert lastMeasure.concentrationRaw to float using lastMeasure.decimalPoint.
 * @return Concentration as float; NAN if decimalPoint > 12.
 */
    float getConcentrationFloat(void) const;

    DFRobot_IntelligentGasSensorMeasure_t lastMeasure;

/**
 * @brief Change sensor Modbus slave address (write holding 0x0006 + COMMIT to EEPROM).
 * @param newAddr:  New Modbus slave address. Range: 1~247 (0x01~0xF7).
 * @return Exception code:
 * @n      0 : sucess.
 * @n      3 or eRTU_EXCEPTION_ILLEGAL_DATA_VALUE:  Illegal address (out of range or unchanged).
 * @n      otherwise same as DFRobot_RTU write/COMMIT errors.
 */
    uint8_t setDeviceAddress(uint8_t newAddr);

/**
 * @brief Write sensor baud-rate holding register 0x0003 only (FC06), not saved until COMMIT.
 * @param code:  Baud code: 1=2400, 2=4800, 3=9600, 4=14400, 5=19200, 6=38400, 7=57600, 8=115200.
 * @return Exception code:
 * @n      0 : sucess.
 * @n      3 or eRTU_EXCEPTION_ILLEGAL_DATA_VALUE:  Illegal baud code.
 * @n      otherwise same as DFRobot_RTU write errors.
 * @note Firmware may switch slave UART baud immediately after ACK; host must begin(new baud) before COMMIT (see changeDeviceBaudrate example).
 */
    uint8_t writeDeviceBaudCode(uint16_t code);

/**
 * @brief Write baud code then COMMIT (does not change host UART baud; prefer changeDeviceBaudrate flow for SEN07xx).
 * @param code:  Baud code (DFROBOT_IGS_BAUD_CODE_*).
 * @return Exception code (same as writeDeviceBaudCode() / commitConfiguration()).
 */
    uint8_t setDeviceBaudCode(uint16_t code);

/**
 * @brief Set probe RUN/SLEEP via holding register 0x0005 (immediate, not EEPROM).
 * @param sleep:  true=SLEEP, false=RUN.
 * @return Exception code (same as writeHoldingReg()).
 */
    uint8_t setProbeSleep(bool sleep);

/**
 * @brief Read probe RUN/SLEEP from holding register 0x0005.
 * @param outSleep:  true if probe is in SLEEP mode.
 * @return Exception code:
 * @n      0 : sucess.
 * @n      3 or eRTU_EXCEPTION_ILLEGAL_DATA_VALUE:  outSleep is NULL.
 * @n      otherwise same as DFRobot_RTU read errors.
 */
    uint8_t readProbeSleepMode(bool *outSleep);

/**
 * @brief Decode gasCode in measure to gasType and unit strings (host-side table).
 * @param m:  Measurement structure; gasCode must be set.
 */
    static void applyGasToMeasure(DFRobot_IntelligentGasSensorMeasure_t *m);

/**
 * @brief Fill timestamp fields in measure from input register table.
 * @param m:  Measurement structure.
 * @param t:  Input register array (must include timestamp registers).
 */
    static void applyTimestampToMeasure(DFRobot_IntelligentGasSensorMeasure_t *m, const uint16_t *t);

/**
 * @brief Map gas code to gas type name string.
 * @param gasCode:  Gas code from input register GAS_CODE.
 * @param buf:  Output buffer.
 * @param bufLen:  Buffer size.
 * @return true if known code, false if unknown (buf cleared).
 */
    static bool gasCodeToTypeName(uint8_t gasCode, char *buf, size_t bufLen);

/**
 * @brief Map gas code to default unit string.
 * @param gasCode:  Gas code from input register GAS_CODE.
 * @param buf:  Output buffer.
 * @param bufLen:  Buffer size.
 * @return true if known code, false if unknown (buf cleared).
 */
    static bool gasCodeToDefaultUnit(uint8_t gasCode, char *buf, size_t bufLen);

/**
 * @brief Format unknown gas code as hex string (e.g. "0x1A").
 * @param gasCode:  Gas code.
 * @param buf:  Output buffer.
 * @param bufLen:  Buffer size.
 */
    static void gasCodeFormatUnknown(uint8_t gasCode, char *buf, size_t bufLen);

/**
 * @brief Write one holding register on the current slave.
 * @param reg:  Holding register address.
 * @param value:  Value to write.
 * @return Exception code (same as DFRobot_RTU writeHoldingRegister()).
 */
    uint8_t writeHoldingReg(uint16_t reg, uint16_t value);

/**
 * @brief Commit configuration to sensor EEPROM (write 0xA501 to holding COMMIT).
 * @return Exception code (same as DFRobot_RTU writeHoldingRegister()).
 */
    uint8_t commitConfiguration(void);

    uint8_t  _slave;
    uint8_t  _readRetries;
};

#endif
