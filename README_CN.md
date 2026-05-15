# DFRobot_IntelligentGasSensor 智能气体传感器库（中文版）

* [English](./README.md)

面向 **DFRobot 智能气体传感器（SEN07xx 系列 Modbus RTU 固件）** 的 Arduino **主机侧**库。在 **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)** 之上实现 Modbus 组帧与 CRC，并完成寄存器语义解析：根据 SMX100 **气体码** 在主机侧得到 **气体类型 / 单位**，配合 **浓度原码与小数位** 计算浮点浓度；可选读取 **时间戳寄存器**；支持 SEN0742 类 **UART 主动上报** 的纯接收解析；提供 **获取模式**、**从站地址烧写** 等高层接口。若从机固件启用 **I2C GasI2cSlave**，主机可用 **`DFRobot_IntelligentGasSensorI2C`**（`Wire`）读取与 Modbus 一致的译码结果。

## 目录

* [概述](#概述)
* [依赖](#依赖)
* [安装](#安装)
* [接线](#接线)
* [构造函数说明](#构造函数说明)
* [从站号：主机侧与传感器侧](#从站号主机侧与传感器侧)
* [Methods（函数说明）](#methods函数说明)
* [I2C GasI2cSlave](#i2c-gasi2cslave)
* [示例程序](#示例程序)
* [进阶：基类 DFRobot_RTU](#进阶基类-dfrobot_rtu)
* [兼容性](#兼容性)
* [许可证](#许可证)

## 概述

* 使用 `readMeasurement()` 轮询测量（默认读 12 个输入寄存器，不含时间戳区）；需要时钟寄存器时用 `readMeasurementWithTimestamp()`。
* 成功后从 `lastMeasure` 读取解析结果，用 `getConcentrationFloat()` 得到浓度浮点数。
* **I2C**：从机为 SEN07xx 固件侧 `GasI2cSlave` 时，包含 `DFRobot_IntelligentGasSensorI2C.h`，`Wire.begin()` 后使用 `DFRobot_IntelligentGasSensorI2C` 的 `readMeasurement()`；详见下文 **[I2C GasI2cSlave](#i2c-gasi2cslave)**。
* **SEN0742 TTL**：用 `getAcquireMode` / `setAcquireMode` 在 **被动轮询** 与 **UART 主动上报** 之间切换；主动模式下用 `pollUnsolicitedAutoReport()` 从串口组帧并解析（不主动发 Modbus 请求）。
* 使用 `setDeviceAddress()` 修改传感器在总线上的 **Modbus 从站地址** 并写入 EEPROM（流程符合固件要求）。

寄存器布局与传感器固件寄存器表一致；总线上传输气体 **编码**，类型与单位由本库本地译码。

## 依赖

* 必须同时安装 **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)**（`library.properties` 中 `depends=DFRobot_RTU`）。

## 安装

1. **库管理器**（若已上架）：搜索 `DFRobot_IntelligentGasSensor` 并安装，确认已安装 `DFRobot_RTU`。
2. **手动安装**：将本库文件夹放入 Arduino 的 `libraries`（或 Arduino IDE 2 的 `Arduino15\libraries`），重启 IDE，在 **文件 → 示例 → DFRobot_IntelligentGasSensor** 中打开例程。

## 接线

| 传感器 / 485 板 | 主控 |
|------------------|------|
| VCC | 按模块要求 3.3 V 或 5 V |
| GND | GND |
| UART RX | 主控 UART **TX**（Modbus 线） |
| UART TX | 主控 UART **RX** |
| RS-485 DE（若使用） | 构造函数的第三参 `dePin` |

波特率须与传感器一致（示例中多为 **9600 8N1**）。建议使用 **硬件串口**（如 `Serial1`、`Serial2`）。

## 构造函数说明

第二个参数固定为 **Modbus 从站地址**；第三个为 **RS-485 DE**（默认 **`-1`** 表示 **TTL**，无 DE）。示例：

```cpp
DFRobot_IntelligentGasSensor gas(&Serial1, 5);        // TTL，从站 5
DFRobot_IntelligentGasSensor gas485(&Serial2, 5, 4); // RS-485 DE=GPIO4，从站 5
```

因此字面量 `5` 始终表示 **从站 5**，不会被当成 DE 脚。完整形参与返回值见下文 **[Methods](#methods函数说明)**。

## 从站号：主机侧与传感器侧

| API | 作用 |
|-----|------|
| `setClientSlaveAddr(addr)` | **仅主机对象**：之后 `readMeasurement` 等请求发往该从站号；**不修改**传感器内部保存的地址。 |
| `getClientSlaveAddr()` | 查询当前主机将使用的从站号。 |
| `setDeviceAddress(newAddr)` | **写传感器**：通过保持寄存器修改从站地址并 COMMIT；库内会再调用 `setClientSlaveAddr`，与固件「写完后立即用新地址应答」一致。 |

## Methods（函数说明）

```C++
/**
 * @brief 在 UART Stream 上构造智能气体传感器的 Modbus 主机对象。
 * @param s 硬件串口指针（如 Serial1、Serial2）；须已按传感器波特率 begin（常见 9600 8N1）。
 * @param slaveAddr Modbus 从站地址 1～247；本对象后续请求均发往该地址。
 * @param dePin RS-485 收发方向脚，语义同 DFRobot_RTU；默认 -1 表示 TTL，不使用 DE。
 */
DFRobot_IntelligentGasSensor(Stream *s, uint8_t slaveAddr, int dePin = -1);

/**
 * @brief 仅更新主机侧从站号（下一帧 Modbus 请求将使用该从站地址）。
 * @param addr 新从站号 1～247。
 * @n 不读写传感器保持寄存器与 EEPROM。若要修改传感器自身在总线上的地址，请用 setDeviceAddress()。
 */
void setClientSlaveAddr(uint8_t addr);

/**
 * @brief 读取当前主机发往总线所使用的 Modbus 从站号。
 * @return 从站地址 1～247。
 */
uint8_t getClientSlaveAddr(void) const;

/**
 * @brief UART 上气体数据的获取方式（SEN0742：保持寄存器 0x0001）；其它硬件可能无此项。
 * @n ACQUIRE_MODE_PASSIVE：主机 Modbus 轮询读测量。ACQUIRE_MODE_ACTIVE：从机可在 UART 上主动推送 FC 0x04 测量帧。
 * @n ACQUIRE_MODE_UNKNOWN：保持寄存器取值非 0 且非 1。
 */
enum AcquireMode : uint8_t {
    ACQUIRE_MODE_PASSIVE = 0,
    ACQUIRE_MODE_ACTIVE = 1,
    ACQUIRE_MODE_UNKNOWN = 0xFF
};

/**
 * @brief 读取当前获取方式（通过 Modbus 读保持寄存器影子）。
 * @param mode 输出：0 被动、1 主动、0xFF 未知；仅当返回值为 0 时有效。
 * @return 0 成功；非 0 为 DFRobot_RTU / Modbus 接收或总线错误——失败时不要读 *mode。
 */
uint8_t getAcquireMode(uint8_t *mode);

/**
 * @brief 设置被动或主动获取方式；可选择是否 COMMIT 写入传感器 EEPROM。
 * @param mode 必须为 ACQUIRE_MODE_PASSIVE(0) 或 ACQUIRE_MODE_ACTIVE(1)；草图中可写 sensor.ACQUIRE_MODE_*。
 * @param commitToEeprom 为 true 时写 COMMIT 键，掉电后仍保持（与波特率等配置一并按固件规则保存）。
 * @return 0 成功；3 表示 mode 非法；其它非 0 为 RTU 写寄存器或 COMMIT 失败码。
 */
uint8_t setAcquireMode(uint8_t mode, bool commitToEeprom = true);

/**
 * @brief 从输入寄存器 0 起执行 FC 0x04 读，并解析到 lastMeasure。
 * @param withTimestamp false（默认）：读 12 个输入寄存器（不含时间戳区）。true：读 18 个寄存器（含 6 个时间寄存器）。
 * @return 0 成功；非 0 为 Modbus / RTU 错误——本次测量视为无效。
 */
uint8_t readMeasurement(bool withTimestamp = false);

/**
 * @brief 等价于 readMeasurement(true)：读满含时间戳的输入表（固件带 RTC 时有效）。
 * @return 0 成功；非 0 为 RTU / Modbus 错误。
 */
uint8_t readMeasurementWithTimestamp(void);

/**
 * @brief 根据最近一次成功解析结果计算浮点浓度。
 * @n 计算式：concentrationRaw × 10^(−decimalPoint)。decimalPoint 超范围时返回 NAN。
 */
float getConcentrationFloat(void) const;

/**
 * @brief 最近一次解析后的测量与标识（在 readMeasurement() 返回 0 后有效；主动上报时在 pollUnsolicitedAutoReport() 返回 0 后有效）。
 * @n 含 pid、vid、modbusAddr、gasType、unit、gasCode、concentrationRaw、decimalPoint、可选时间戳字符串、dataValid 等。
 */
DFRobot_IntelligentGasSensorMeasure_t lastMeasure;

/**
 * @brief 主动上报接收路径：从构造时传入的 Stream 读字节，按静默间隔组帧，解析一帧 FC 0x04 形应答到 lastMeasure；不发送任何 Modbus 请求。
 * @return 0 本周期解析到新合法帧；1 本周期尚无完整帧；9 构造时串口指针为空。
 */
uint8_t pollUnsolicitedAutoReport(void);

/**
 * @brief 将传感器 Modbus 从站地址写入保持寄存器 0x0006 并按固件要求 COMMIT；随后同步主机侧从站号。
 * @param newAddr 目标从站号 1～247；与当前传感器地址相同时不写寄存器。
 * @return 0 成功；3 表示 newAddr 非法；其它非 0 为 RTU 写失败或 COMMIT 失败。
 */
uint8_t setDeviceAddress(uint8_t newAddr);

/**
 * @brief 将 Modbus 顺序输入寄存器表（从 0 起）解码到 `out`（与 FC 0x04 布局一致）。Modbus 与 I2C 路径共用。
 */
static void fillLastMeasureFromInputRegs(DFRobot_IntelligentGasSensorMeasure_t *out, const uint16_t *t, uint16_t regCount);
```

类还 **公有继承 `DFRobot_RTU`**，进阶用法可直接调用基类方法（如 `setTimeoutTimeMs`、原始 `readInputRegister` / `writeHoldingRegister` 等），详见 [DFRobot_RTU README](https://github.com/DFRobot/DFRobot_RTU)。

## I2C GasI2cSlave

从机烧录带 **`GasI2cSlave`** 的固件（如 SEN0738 工程）时，主机通过 **`Wire`** 读气体数据。须 **先** `Wire.begin()`（RP2040 可再 `setSDA` / `setSCL` / `setClock`）。

```cpp
#include <Wire.h>
#include <DFRobot_IntelligentGasSensorI2C.h>

DFRobot_IntelligentGasSensorI2C gas(0x38, &Wire);

void setup() {
  Wire.begin();
  // gas.setStopGapMicros(400);  // AVR 等可适当加大写 STOP 与读段间隔
}

void loop() {
  if (gas.readMeasurementWithTimestamp() != DFROBOT_IGS_I2C_OK) {
    Serial.println(F("read error"));
    return;
  }
  Serial.print(gas.lastMeasure.gasType);
  Serial.print(' ');
  Serial.print(gas.getConcentrationFloat(), 2);
  Serial.print(' ');
  Serial.print(gas.lastMeasure.unit);
  if (gas.lastMeasure.timestamp[0] != '\0') {
    Serial.print(' ');
    Serial.print(gas.lastMeasure.timestamp);
  }
  Serial.println();
}
```

* **与 Modbus 类的区别**：I2C 类 **无** `getAcquireMode` / `setAcquireMode` / `pollUnsolicitedAutoReport` / `setDeviceAddress`（这些仅 UART Modbus）；I2C 改址用 **`programI2cAddress`**（0x08～0x77，写从机 EEPROM）。
* **传输注意**：对 arduino-pico 从机须 **写子地址 + STOP**，短延时再 **`requestFrom`**（本库默认 RP2040 间隔约 400 µs，并对 WHO_AM_I **短时重读** 数次，与 SEN0738 `I2cMaster_GasSlaveRead` 一致）。若仍偶发 `DFROBOT_IGS_I2C_ERR_WHOAMI`，可再略调大 `setStopGapMicros`，或 `setVerifyWhoAmI(false)`（不推荐除非总线可信）。
* **错误码**：`DFROBOT_IGS_I2C_OK`、`DFROBOT_IGS_I2C_ERR_WIRE_TX` 等见 `DFRobot_IntelligentGasSensorI2C.h`。

## 示例程序

| 例程 | 说明 |
|------|------|
| `readGasI2c` | I2C 主机：与 `readGasModbus` 相同浓度行，并固定带墙钟（`readMeasurementWithTimestamp`）在同一行末尾打印时间。 |
| `readGasModbus` | 最简轮询：打印气体、浓度、单位；宏切换 TTL / RS-485。 |
| `readGasMultiSlave` | 同一 UART 上多个从站，多实例轮询。 |
| `changeMode` | 切换 UART 获取模式（被动 ↔ 主动）。 |
| `uartAutoReportListen` | 打开主动上报后仅接收解析。 |
| `changeDeviceAddress` | 一次性修改传感器从站地址。 |

## 进阶：基类 DFRobot_RTU

本类继承 `DFRobot_RTU`，需要时可使用基类提供的其它 Modbus 功能。气体库构造里已将应答超时设为 **200 ms**（`setTimeoutTimeMs`），总线很慢时可自行再调。

## 兼容性

* **架构**：`library.properties` 中 `architectures=*`，具体取决于 `DFRobot_RTU` 与目标板 `Stream` 支持情况。
* **传感器**：SEN07xx Modbus 固件寄存器图；其中 SEN0742 TTL 的 UART 主动上报在固件支持的前提下可用。

## 许可证

MIT License，见 `src/DFRobot_IntelligentGasSensor.h` 文件头版权声明。
