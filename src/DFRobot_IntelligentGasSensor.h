/*!
 * @file DFRobot_IntelligentGasSensor.h
 * @brief Host-side Modbus RTU driver for DFRobot intelligent gas sensors (SEN07xx Modbus 固件).
 *
 * @details
 * 寄存器与传感器固件 `RtuRegisterMap.h` 一致：总线仅 **GAS_CODE**；类型/单位由本库译码。
 * 时间戳为 **6 个输入寄存器**（年月日时分秒）；`lastMeasure.timestamp` 由库拼接便于打印。
 * **SEN0742**：UART 上气体数据的获取方式由保持寄存器 `0x0001` 决定；应用层请用 @ref getAcquireMode / @ref setAcquireMode。底层写寄存器可用 @ref setUartAutoReport。主动上报收端请用 @ref pollUnsolicitedAutoReport / @ref parseUnsolicitedInputReadResponse（**不经过** `DFRobot_RTU` 的收发 API）。
 * 默认 `readMeasurement()` 不读时间戳区；需要时 `readMeasurementWithTimestamp()`。
 * 构造：`DFRobot_IntelligentGasSensor(Stream *s, uint8_t slaveAddr, int dePin = -1)`；`dePin < 0`（默认 -1）为 TTL 无 DE；RS-485 时传入 DE 引脚号（与 `DFRobot_RTU` 一致）。
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @version     V1.0
 * @date        2026-05-14
 */
#ifndef __DFRobot_IntelligentGasSensor_H__
#define __DFRobot_IntelligentGasSensor_H__

#include "Arduino.h"
#include "DFRobot_RTU.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

/** DFRobot VID（输入寄存器 0x0001） */
#define DFROBOT_IGS_VID_DFRobot ((uint16_t)0x3343)
/** TTL UART 产品 PID（输入寄存器 0x0000） */
#define DFROBOT_IGS_PID_SEN0742 ((uint16_t)0xC742)
/** RS-485 产品 PID */
#define DFROBOT_IGS_PID_SEN0746 ((uint16_t)0xC746)

#define DFROBOT_IGS_REGMAP_VERSION ((uint16_t)0x0100)

#define DFROBOT_IGS_BOARD_UNKNOWN ((uint16_t)0)
#define DFROBOT_IGS_BOARD_UART    ((uint16_t)1)
#define DFROBOT_IGS_BOARD_RS485   ((uint16_t)2)

#define DFROBOT_IGS_STATUS_VALID_MSK ((uint16_t)(1u << 0))

#define DFROBOT_IGS_SLAVE_ADDR_MIN 1u
#define DFROBOT_IGS_SLAVE_ADDR_MAX 247u

#define DFROBOT_IGS_COMMIT_KEY_SAVE_CONFIG ((uint16_t)0xA501)

/* 输入寄存器地址（0 基，FC 0x04） */
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

/* 保持寄存器（FC 0x03 / 0x06 / 0x10） */
#define DFROBOT_IGS_HOLD_REG_UART_AUTO_REPORT 0x0001u
#define DFROBOT_IGS_HOLD_REG_BAUD_CODE    0x0003u
#define DFROBOT_IGS_HOLD_REG_PARITY_STOP  0x0004u
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

#define DFROBOT_IGS_PARITY_NONE  ((uint16_t)0u)
#define DFROBOT_IGS_PARITY_ODD   ((uint16_t)1u)
#define DFROBOT_IGS_PARITY_EVEN  ((uint16_t)2u)
#define DFROBOT_IGS_STOPBIT_1    ((uint16_t)0u)
#define DFROBOT_IGS_STOPBIT_2    ((uint16_t)(1u << 2))

/** `pollUnsolicitedAutoReport` 内建：无新字节达到该毫秒数后认为一帧结束（约覆盖 9600 8N1 下 t3.5；更高波特可改小后重编库） */
#define DFROBOT_IGS_AUTO_REPORT_IDLE_MS  ((uint16_t)8u)

/**
 * @brief 一次输入寄存器表解析后的测量与标识（在 @ref readMeasurement 成功后有效）
 */
typedef struct {
    uint16_t pid;
    uint16_t vid;
    uint8_t  modbusAddr;
    uint16_t registerMapVersion;
    uint16_t boardType;
    uint16_t status;
    uint8_t  gasCode; /**< Raw SMX100 gas code from input reg @ref DFROBOT_IGS_IN_REG_GAS_CODE (low 8 bits) */
    uint32_t concentrationRaw;
    uint8_t  decimalPoint;
    char     gasType[13]; /**< **Decoded locally** from @ref gasCode (not read from Modbus as text); unknown → `0xNN` */
    char     unit[9]; /**< **Decoded locally** from @ref gasCode; unknown code → `"?"` */
    uint16_t tsYear; /**< 仅读满含时间戳寄存器后有效；无时间时为 0 */
    uint8_t  tsMonth, tsDay, tsHour, tsMinute, tsSecond;
    char     timestamp[21]; /**< 由年月日时分秒拼接；无有效时间时为 "" */
    bool     dataValid;
} DFRobot_IntelligentGasSensorMeasure_t;

class DFRobot_IntelligentGasSensor : public DFRobot_RTU {
public:
    /**
     * @brief 构造：先 Modbus 从站地址，再可选 RS-485 DE 引脚。
     * @param s  Modbus 串口（`Serial1` / `Serial2` 等）
     * @param slaveAddr 从站地址 1～247
     * @param dePin RS-485 收发方向控制（高发送等，与 `DFRobot_RTU` 一致）；默认 **-1** 表示 TTL、无 DE
     */
    DFRobot_IntelligentGasSensor(Stream *s, uint8_t slaveAddr, int dePin = -1);

    void setSlaveAddr(uint8_t addr);
    uint8_t getSlaveAddr(void) const { return _slave; }

    /**
     * @brief 气体数据在 UART 上的获取方式（SEN0742 TTL 对应保持寄存器 `0x0001`；RS-485 等板型可能无主动上报）
     */
    enum AcquireMode : uint8_t {
        ACQUIRE_MODE_PASSIVE = 0, /**< 主机轮询 Modbus 读测量 */
        ACQUIRE_MODE_ACTIVE  = 1, /**< 从机 UART 主动推送（FC 0x04 测量帧） */
        ACQUIRE_MODE_UNKNOWN = 0xFF /**< 寄存器值非 0/1，固件异常或未定义 */
    };

    /**
     * @brief 读取当前获取方式（内部读保持寄存器影子）
     * @param mode 输出；成功时不会是未使用的未定义状态，失败时勿读
     * @return 0 成功，非 0 为 DFRobot_RTU 异常码
     */
    uint8_t getAcquireMode(AcquireMode *mode);

    /**
     * @brief 同 @ref getAcquireMode(AcquireMode*)，输出为 `uint8_t`：`0`/`1`/`0xFF`，与 @ref AcquireMode 取值一致（便于 sketch）
     */
    uint8_t getAcquireMode(uint8_t *mode);

    /**
     * @brief 设置获取方式并可选写入 EEPROM（内部写 `0x0001` 与 COMMIT）
     * @param mode 仅允许 @ref ACQUIRE_MODE_PASSIVE 或 @ref ACQUIRE_MODE_ACTIVE
     * @param commitToEeprom true 时落盘（与波特率等配置一并保存）
     * @return 0 成功；`mode` 非法返回 3；其它为 RTU 写失败码
     */
    uint8_t setAcquireMode(AcquireMode mode, bool commitToEeprom = true);

    /**
     * @brief 同 @ref setAcquireMode(AcquireMode,bool)，`mode` 仅允许 `0`（被动）或 `1`（主动）
     */
    uint8_t setAcquireMode(uint8_t mode, bool commitToEeprom = true);

    /**
     * @brief 读取输入寄存器并填充 @ref lastMeasure（默认不读时间戳区，与常见无 RTC 传感器一致）
     * @param withTimestamp false（默认）：读到小数位（12 个寄存器）；true：再读 6 个时间寄存器（共 18 个）
     * @return 0 成功，非 0 为 DFRobot_RTU 异常码
     */
    uint8_t readMeasurement(bool withTimestamp = false);

    /**
     * @brief 读满输入寄存器表并解析时间戳（带 RTC 的固件可选用）
     */
    uint8_t readMeasurementWithTimestamp(void);

    /**
     * @brief 仅读前 6 个输入寄存器（PID/VID/地址/版本），用于快速探测
     */
    uint8_t readIdentification(uint16_t *pid, uint16_t *vid, uint16_t *version);

    /**
     * @brief 读 VID 是否为 DFRobot 规范值
     */
    bool checkVid(void);

    /**
     * @brief 浓度浮点值 = concentrationRaw × 10^(−decimalPoint)
     */
    float getConcentrationFloat(void) const;

    /**
     * @brief SMX100 手册表 3：气体码 → 类型缩写（与 SEN07xx 固件 `SensorDriver` 映射表一致）
     * @n 已知码：0x01 CH4、0x03 H2S、0x04 CO、0x05 O2（表随手册扩充时在此与固件同步维护）
     * @return true 已写入 buf（含 '\\0'）；false 未知码且 buf[0] 置为 '\\0'
     */
    static bool gasCodeToTypeName(uint8_t gasCode, char *buf, size_t bufLen);

    /**
     * @brief 气体码 → 默认单位（与固件一致，如 O2 为 "%VOL"）
     * @return true 已知；false 未知且 buf[0]='\\0'
     */
    static bool gasCodeToDefaultUnit(uint8_t gasCode, char *buf, size_t bufLen);

    /**
     * @brief 未知气体码时的占位类型名，与固件相同格式：`0xNN`
     */
    static void gasCodeFormatUnknown(uint8_t gasCode, char *buf, size_t bufLen);

    DFRobot_IntelligentGasSensorMeasure_t lastMeasure;

    /**
     * @brief 读取保持寄存器影子（长度 @ref DFROBOT_IGS_HOLDING_REG_COUNT）
     */
    uint8_t readHoldingShadow(uint16_t *regs);

    /**
     * @brief 写单个保持寄存器（如改地址、波特率编码等）
     */
    uint8_t writeHoldingReg(uint16_t reg, uint16_t value);

    /**
     * @brief Write commit magic to holding reg 0x0007 to save baud / parity / slave address to sensor EEPROM
     */
    uint8_t commitConfiguration(void);

    /**
     * @brief 底层：写保持寄存器 `0x0001` 开关 UART 主动上报。常用请用 @ref setAcquireMode。
     * @param commitToEeprom true 时再写 `0x0007`=COMMIT 键落盘（与波特率等一并保存）
     */
    uint8_t setUartAutoReport(bool enable, bool commitToEeprom = false);

    /**
     * @brief 解析 SEN0742 **主动上报**的一帧 RTU（格式同 FC 0x04 读 12 个输入寄存器的应答）
     * @param adu 含 CRC；总长应为 **29**（1+1+1+24+2）
     * @param len 字节数，须为 29
     * @return 0 成功并填充 @ref lastMeasure；8 CRC 错；11 从站号不符；3 功能码/字节数等非法
     */
    uint8_t parseUnsolicitedInputReadResponse(const uint8_t *adu, size_t len);

    /**
     * @brief **仅接收**：从构造时传入的 Modbus `Stream` 读字节，按 @ref DFROBOT_IGS_AUTO_REPORT_IDLE_MS 静默判定组帧后调用 @ref parseUnsolicitedInputReadResponse（**不发送**任何请求）。
     * @return 0 已更新 @ref lastMeasure；1 本周期尚无完整合法帧；9 构造时串口指针为空
     */
    uint8_t pollUnsolicitedAutoReport(void);

    /**
     * @brief 将传感器 Modbus 从站地址改为 `newAddr`（1～247），并写入 EEPROM 保持
     * @n 顺序：写保持寄存器 0x0006 → 本机 `setSlaveAddr`（固件会立即用新地址应答）→ `commitConfiguration` 落盘
     * @return 0 成功；非 0 为 DFRobot_RTU 写失败码；`newAddr` 非法时返回 3（非法数据）
     */
    uint8_t setDeviceAddress(uint8_t newAddr);

private:
    uint8_t   _slave;
    Stream   *_modbusStream = nullptr;
    uint8_t   _autoRepBuf[64]{};
    uint16_t  _autoRepLen = 0;
    uint32_t  _autoRepLastMs = 0;
    void parseInputTable(const uint16_t *table, uint16_t regCount);
    void applyGasStringsFromCode(void);
    void applyTimestampFromRegs(const uint16_t *t);
};

#endif
