/*!
 * @file DFRobot_IntelligentGasSensor.h
 * @brief Host-side Modbus RTU driver for DFRobot intelligent gas sensors (SEN07xx).
 *
 * @details
 * 寄存器与传感器固件 `RtuRegisterMap.h` 一致：总线仅 **GAS_CODE**；类型/单位由本库译码。
 * 时间戳为 **6 个输入寄存器**（年月日时分秒）；`lastMeasure.timestamp` 由库拼接便于打印。
 * 默认 `readMeasurement()` 不读时间戳区；需要时 `readMeasurementWithTimestamp()`。
 * 构造：`DFRobot_IntelligentGasSensor(Stream *s, uint8_t slaveAddr, int dePin = -1)`；
 * `dePin < 0` 为 TTL；RS-485 时传入 DE 引脚号（与 `DFRobot_RTU` 一致）。
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @version     V1.3.2
 * @date        2026-05-20
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

#define DFROBOT_IGS_BOARD_UNKNOWN ((uint16_t)0)
#define DFROBOT_IGS_BOARD_UART    ((uint16_t)1)
#define DFROBOT_IGS_BOARD_RS485   ((uint16_t)2)

#define DFROBOT_IGS_STATUS_VALID_MSK ((uint16_t)(1u << 0))

#define DFROBOT_IGS_SLAVE_ADDR_MIN 1u
#define DFROBOT_IGS_SLAVE_ADDR_MAX 247u

#define DFROBOT_IGS_COMMIT_KEY_SAVE_CONFIG ((uint16_t)0xA501)

#define DFROBOT_IGS_IN_REG_PID          0x0000u
#define DFROBOT_IGS_IN_REG_VID          0x0001u
#define DFROBOT_IGS_IN_REG_MODBUS_ADDR  0x0002u
#define DFROBOT_IGS_IN_REG_VERSION      0x0005u
#define DFROBOT_IGS_IN_REG_BOARD_TYPE   0x0006u
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
/** 与固件 SEN_RTU_HOLD_REG_PROBE_SLEEP：0=运行 1=休眠（CS），立即生效、不落 EEPROM */
#define DFROBOT_IGS_HOLD_REG_RESERVED3  0x0005u
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
/** 与保持寄存器 0x0003 编码一致，供 `writeDeviceBaudCode()` / `setDeviceBaudCode()` 校验 */
#define DFROBOT_IGS_BAUD_CODE_MIN    DFROBOT_IGS_BAUD_CODE_2400
#define DFROBOT_IGS_BAUD_CODE_MAX    DFROBOT_IGS_BAUD_CODE_115200

#define DFROBOT_IGS_PARITY_NONE  ((uint16_t)0u)
#define DFROBOT_IGS_PARITY_ODD   ((uint16_t)1u)
#define DFROBOT_IGS_PARITY_EVEN  ((uint16_t)2u)
#define DFROBOT_IGS_STOPBIT_1    ((uint16_t)0u)
#define DFROBOT_IGS_STOPBIT_2    ((uint16_t)(1u << 2))

typedef struct {
    uint16_t pid;
    uint16_t vid;
    uint8_t  modbusAddr;
    uint16_t registerMapVersion;
    uint16_t boardType;
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
    DFRobot_IntelligentGasSensor(Stream *s, uint8_t slaveAddr, int dePin = -1);

    void setClientSlaveAddr(uint8_t addr);
    uint8_t getClientSlaveAddr(void) const { return _slave; }

    static void fillLastMeasureFromInputRegs(DFRobot_IntelligentGasSensorMeasure_t *out, const uint16_t *t,
                                             uint16_t regCount);

    uint8_t readMeasurement(bool withTimestamp = false);
    uint8_t readMeasurementWithTimestamp(void);
    float getConcentrationFloat(void) const;

    DFRobot_IntelligentGasSensorMeasure_t lastMeasure;

    uint8_t setDeviceAddress(uint8_t newAddr);

    /**
     * @brief 仅写传感器 Modbus 波特率保持寄存器 **0x0003**（FC06），不落 EEPROM、不 COMMIT。
     * @param code `DFROBOT_IGS_BAUD_CODE_*`（1=2400 … 8=115200）；非法值返回 3。
     * @return 0 成功，否则为 `DFRobot_RTU` 错误码。
     * @note 固件在应答后常**立即**切从站线速；下一步应对 Modbus 串口 `begin(与 code 一致的波特率)` 再 `commitConfiguration()`，参见 `changeDeviceBaudrate`。
     */
    uint8_t writeDeviceBaudCode(uint16_t code);

    /**
     * @brief 连续调用 `writeDeviceBaudCode()` 与 `commitConfiguration()`（中间不切主机串口波特率）。
     * @note SEN07xx 在写 **0x0003** 后即切线速时，主机须在两次 Modbus 之间 `begin` 新波特率；请用例程 `changeDeviceBaudrate` 的三步：`writeDeviceBaudCode` → `begin` → `commitConfiguration`。
     */
    uint8_t setDeviceBaudCode(uint16_t code);

    /** 探头休眠：写保持 0x0005，立即生效、不落 EEPROM（与固件 `sensor sleep`/`sensor wake` 一致） */
    uint8_t setProbeSleep(bool sleep);
    /** 读保持 0x0005：true=SLEEP，false=RUN；返回 0 成功，非 0 为 DFRobot_RTU 错误码 */
    uint8_t readProbeSleepMode(bool *outSleep);
    static void applyGasToMeasure(DFRobot_IntelligentGasSensorMeasure_t *m);
    static void applyTimestampToMeasure(DFRobot_IntelligentGasSensorMeasure_t *m, const uint16_t *t);

    static bool gasCodeToTypeName(uint8_t gasCode, char *buf, size_t bufLen);
    static bool gasCodeToDefaultUnit(uint8_t gasCode, char *buf, size_t bufLen);
    static void gasCodeFormatUnknown(uint8_t gasCode, char *buf, size_t bufLen);

    uint8_t writeHoldingReg(uint16_t reg, uint16_t value);
    uint8_t commitConfiguration(void);

    uint8_t _slave;
};

#endif
