# DFRobot_IntelligentGasSensor

* [English Version](./README.md)

面向 **DFRobot SEN07xx 智能气体传感器** 的 Arduino Modbus RTU 主机库，基于 **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)**。主要能力：<br>
* **0x04**：读气体测量输入寄存器（`readGasMeasurementData`）；
* 主机侧根据 **GAS_CODE** 译码气体类型与单位；
* **0x06** + COMMIT：修改 Modbus 从站地址（`setDeviceAddress`）；
* **0x06**：写波特率保持寄存器 `0x0003`（`writeDeviceBaudCode` / `setDeviceBaudCode`）；
* **0x06**：写保持 `0x0005` 控制探头 RUN/SLEEP（`setProbeSleep` / `getProbeWorkMode`，不落 EEPROM）；
* `setClientSlaveAddr()`：仅切换主机侧目标从站号（不写传感器 EEPROM）。

## 产品链接（[https://www.dfrobot.com.cn](https://www.dfrobot.com.cn)）

```text
SKU: SEN0738 / SEN0739 / SEN0740 / SEN0741
```



## 目录

* [概要](#概要)
* [接线](#接线)
* [库安装](#库安装)
* [CDC 配置命令](#cdc-配置命令)
* [方法](#方法)
* [兼容性](#兼容性)
* [历史](#历史)
* [创作者](#创作者)

## 概要

本库将 SEN07xx 作为 Modbus RTU 从机进行读写，在 **DFRobot_RTU** 之上解析输入寄存器并填充 `lastMeasure`。默认链路：**9600 8N1**，从站地址 **1**。<br>

例程见 `examples/`（如 `readGasRS485`、`readGasUART`、`changeDeviceAddress`、`changeDeviceBaudrate`）。寄存器表：[REGISTERMAP_MODBUS_CN.md](./docs/REGISTERMAP_MODBUS_CN.md)。

## 接线

Hardware conneted table（以 ESP32 为例；传感器对外仅 **RS-485 A/B**，TX/RX/DE 接 **UART 转 RS485 模块**，不直连传感器）<br>

| ESP32 引脚 | UART 转 RS485 模块 |
|------------|-------------------|
| 3.3V | VCC |
| GND | GND |
| GPIO17 (TX) | DI |
| GPIO36 (RX) | RO |
| GPIO16 | DE/RE（方向控制，对应构造函数的 `dePin`） |

| UART 转 RS485 模块 | 传感器 (SEN07xx) |
|-------------------|------------------|
| A | A |
| B | B |

说明：板载 **HOST UART 为 3.3V**，**不支持** 5V 主控（UNO/Mega 等）直连 TTL；5V 主控请经 RS-485 模块连接。TTL 直连（无 DE）见 `readGasUART`（仅 3.3V ESP32）。

## 库安装

1. 先将依赖库 **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)** 放入 `Arduino/libraries`。
2. 下载本库到 `Arduino/libraries`，重启 IDE。
3. 打开 `examples` 文件夹，运行对应例程（如 `readGasRS485`）。

## CDC 配置命令

SEN07xx 模块除 **Modbus RTU 主机口**（TTL / RS-485）外，还提供 **USB CDC** 虚拟串口，用于本地配置与状态查看（**不是** RS-485 总线上的 Modbus 帧）。

| 项目 | 说明 |
|------|------|
| 连接 | USB 接 PC，在设备管理器中打开对应 **COM 口** |
| 串口监视器 | 建议 **115200 8N1**（与模块 SenLog/CDC 一致；与 Modbus 线速 9600 无关） |
| 换行 | 发送命令后回车；输入 `help` 可打印完整列表 |

上电后串口输出示例：

```text
=== available commands ===
  help                          show this help
  status                        print sensor + RTC + Modbus summary
  settime YYYY-MM-DD HH:MM:SS   set RTC time
  sensor sleep                  SMX100 probe SLEEP
  sensor wake                   SMX100 probe RUN
  modbus                        show host UART / slave settings
  setaddr <1-247>               set Modbus slave ID (saved to EEPROM)
  setbaud <2400|4800|9600|...>  set host UART baud (saved)
  setparity <8N1|8E1|8O1|8N2>   set host UART format (saved)
  modbus default                restore slave=1, 9600 8N1 (saved)
```

### 命令说明

| 命令 | 作用 |
|------|------|
| `help` | 打印命令列表 |
| `status` | 传感器浓度/有效性、探头 RUN/SLEEP、RTC、Modbus 从站号与线参数摘要 |
| `settime YYYY-MM-DD HH:MM:SS` | 设置 RTC（例：`settime 2026-05-21 11:15:00`） |
| `sensor sleep` | SMX100 探头 **休眠**（CS 拉低），立即生效 |
| `sensor wake` | SMX100 探头 **运行**（CS 拉高），立即生效 |
| `modbus` | 显示当前 Modbus 从站地址、主机 UART 波特率与帧格式 |
| `setaddr <1-247>` | 修改 Modbus 从站号并 **写入 EEPROM** |
| `setbaud <速率>` | 修改主机口波特率并保存；支持 `2400` `4800` `9600` `14400` `19200` `38400` `57600` `115200` |
| `setparity <格式>` | 修改主机口帧格式并保存；`8N1` `8E1` `8O1` `8N2` |
| `modbus default` | 恢复出厂：**从站 1、9600 8N1**，并写入 EEPROM |

### 与 Modbus 寄存器 / 本库 API 的对应关系

| CDC 命令 | 保持寄存器 | 本库（Arduino 主机） |
|----------|------------|----------------------|
| `setaddr` | `0x0006` 从站地址 + `0x0007` COMMIT | `setDeviceAddress()` |
| `setbaud` | `0x0003` 波特率编码 + COMMIT | `writeDeviceBaudCode()` + `Serial.begin` + `commitConfiguration()`（见 `changeDeviceBaudrate`） |
| `setparity` | `0x0004` 校验/停止位 + COMMIT | 写保持 `0x0004` 后 COMMIT（库未封装专用 API） |
| `sensor sleep` / `sensor wake` | `0x0005` 探头休眠（**不落 EEPROM**） | `setProbeSleep(true/false)` / `getProbeWorkMode()` |
| `modbus` / `status` | — | 读输入寄存器 / `readGasMeasurementData()` 等 |

说明：

* **CDC 与 Modbus 是两条通道**：PC 用 CDC 改配置；ESP32 等主控通过 RS-485/TTL 发 Modbus 读浓度。
* 改波特率后，Arduino 侧 `HOST_SERIAL.begin(...)` 必须与模块新线速一致，否则读不到数据。
* 探头 RUN/SLEEP **仅保持寄存器 0x0005**；输入寄存器 `0x0006` 为保留，无板型字段。

## 方法

```C++
/**
 * @brief 构造气体传感器驱动对象。
 * @param s:  用于 Modbus RTU 的串口 Stream 指针。
 * @param slaveAddr:  Modbus 从站地址，范围 1~247 (0x01~0xF7)。
 * @param dePin:  RS485 流控引脚，拉低接收、拉高发送；-1 表示 TTL UART（无 DE）。
 */
DFRobot_IntelligentGasSensor(Stream *s, uint8_t slaveAddr, int dePin = -1);

/**
 * @brief 设置后续读写使用的从站地址（仅主机侧，不写 EEPROM）。
 * @param addr:  Modbus 从站地址，范围 1~247 (0x01~0xF7)。
 */
void setClientSlaveAddr(uint8_t addr);

/**
 * @brief 获取当前对象使用的 Modbus 从站地址。
 * @return 从站地址 (1~247)。
 */
uint8_t getClientSlaveAddr(void) const;

/**
 * @brief 将输入寄存器表解析为测量结构体。
 * @param out:  输出测量结构体。
 * @param t:  输入寄存器数组（从地址 0 起）。
 * @param regCount:  寄存器个数（不含时间戳 12，含时间戳 18）。
 */
static void fillLastMeasureFromInputRegs(DFRobot_IntelligentGasSensorMeasure_t *out, const uint16_t *t, uint16_t regCount);

/**
 * @brief 读取气体测量值（FC04 输入寄存器）。
 * @param withTimestamp:  false：读 0~11；true：读 0~17（含时间戳）。
 * @return Exception code:
 * @n      0 : sucess.
 * @n      1 or eRTU_EXCEPTION_ILLEGAL_FUNCTION : 非法功能.
 * @n      2 or eRTU_EXCEPTION_ILLEGAL_DATA_ADDRESS: 非法数据地址.
 * @n      3 or eRTU_EXCEPTION_ILLEGAL_DATA_VALUE:  非法数据值.
 * @n      4 or eRTU_EXCEPTION_SLAVE_FAILURE:  从机故障.
 * @n      8 or eRTU_EXCEPTION_CRC_ERROR:  CRC校验错误.
 * @n      9 or eRTU_RECV_ERROR:  接收包错误.
 * @n      10 or eRTU_MEMORY_ERROR: 内存错误.
 * @n      11 or eRTU_ID_ERROR:广播地址或错误ID(因为主机无法收到从机广播包的应答)
 */
uint8_t readGasMeasurementData(bool withTimestamp = false);

/**
 * @brief 根据 lastMeasure 中小数位将浓度原始值转为 float。
 * @return 浓度浮点值；decimalPoint>12 时返回 NAN。
 */
float getConcentrationFloat(void) const;

DFRobot_IntelligentGasSensorMeasure_t lastMeasure;

/**
 * @brief 修改传感器 Modbus 从站地址（写保持 0x0006 并 COMMIT 至 EEPROM）。
 * @param newAddr:  新从站地址，范围 1~247 (0x01~0xF7)。
 * @return Exception code:
 * @n      0 : sucess.
 * @n      3 or eRTU_EXCEPTION_ILLEGAL_DATA_VALUE:  地址非法或未变化。
 * @n      其余同 DFRobot_RTU 写/COMMIT 错误码。
 */
uint8_t setDeviceAddress(uint8_t newAddr);

/**
 * @brief 仅写传感器波特率保持寄存器 0x0003（FC06），COMMIT 前不落 EEPROM。
 * @param code:  波特率编码：1=2400 … 8=115200。
 * @return Exception code:
 * @n      0 : sucess.
 * @n      3 or eRTU_EXCEPTION_ILLEGAL_DATA_VALUE:  非法波特率编码。
 * @n      其余同 DFRobot_RTU 写错误码。
 * @note 固件可能在应答后立即切换从站线速；主机须在 COMMIT 前 begin(新波特率)，见 changeDeviceBaudrate 例程。
 */
uint8_t writeDeviceBaudCode(uint16_t code);

/**
 * @brief 连续写波特率并 COMMIT（不改变主机串口波特率；SEN07xx 立即切线速时请用例程三步）。
 * @param code:  波特率编码 (DFROBOT_IGS_BAUD_CODE_*)。
 * @return 异常码（同 writeDeviceBaudCode() / commitConfiguration()）。
 */
uint8_t setDeviceBaudCode(uint16_t code);

/**
 * @brief 写保持 0x0005 设置探头 RUN/SLEEP（立即生效，不写 EEPROM）。
 * @param sleep:  true=休眠，false=运行。
 * @return 异常码（同 writeHoldingReg()）。
 */
uint8_t setProbeSleep(bool sleep);

/**
 * @brief 读保持 0x0005 获取探头 RUN/SLEEP 状态。
 * @param outSleep:  true 表示休眠模式。
 * @return Exception code:
 * @n      0 : sucess.
 * @n      3 or eRTU_EXCEPTION_ILLEGAL_DATA_VALUE:  outSleep 为 NULL。
 * @n      其余同 DFRobot_RTU 读错误码。
 */
uint8_t getProbeWorkMode(bool *outSleep);

/**
 * @brief 根据 measure 中 gasCode 译码 gasType、unit（主机侧表）。
 * @param m:  测量结构体，须已填入 gasCode。
 */
static void applyGasToMeasure(DFRobot_IntelligentGasSensorMeasure_t *m);

/**
 * @brief 从输入寄存器表填充时间戳字段。
 * @param m:  测量结构体。
 * @param t:  输入寄存器数组（须含时间戳寄存器）。
 */
static void applyTimestampToMeasure(DFRobot_IntelligentGasSensorMeasure_t *m, const uint16_t *t);

/**
 * @brief 气体码映射为类型名字符串。
 * @param gasCode:  输入寄存器 GAS_CODE。
 * @param buf:  输出缓冲区。
 * @param bufLen:  缓冲区长度。
 * @return 已知码返回 true，未知返回 false（buf 清空）。
 */
static bool gasCodeToTypeName(uint8_t gasCode, char *buf, size_t bufLen);

/**
 * @brief 气体码映射为默认单位字符串。
 * @param gasCode:  输入寄存器 GAS_CODE。
 * @param buf:  输出缓冲区。
 * @param bufLen:  缓冲区长度。
 * @return 已知码返回 true，未知返回 false（buf 清空）。
 */
static bool gasCodeToDefaultUnit(uint8_t gasCode, char *buf, size_t bufLen);

/**
 * @brief 将未知气体码格式化为十六进制字符串（如 "0x1A"）。
 * @param gasCode:  气体码。
 * @param buf:  输出缓冲区。
 * @param bufLen:  缓冲区长度。
 */
static void gasCodeFormatUnknown(uint8_t gasCode, char *buf, size_t bufLen);

/**
 * @brief 写当前从站的一个保持寄存器。
 * @param reg:  保持寄存器地址。
 * @param value:  写入值。
 * @return 异常码（同 DFRobot_RTU writeHoldingRegister()）。
 */
uint8_t writeHoldingReg(uint16_t reg, uint16_t value);

/**
 * @brief 将配置 COMMIT 到传感器 EEPROM（向保持 COMMIT 写 0xA501）。
 * @return 异常码（同 DFRobot_RTU writeHoldingRegister()）。
 */
uint8_t commitConfiguration(void);
```

## 兼容性

MCU                | RS-485（经转接模块） | TTL UART（仅 3.3V） |
------------------ | :------------------: | :-----------------: |
ESP32              |          √           |          √          |
Arduino Uno (5V)   |          √           |          X          |
Mega2560 (5V)      |          √           |          X          |
Leonardo           |          √           |          X          |
ESP8266            |          √           |          X          |
micro:bit          |          X           |          X          |

须安装 **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)**，并使用硬件串口（`Serial1` / `Serial2`）。

## 历史

- 日期 2026-05-21
- 版本 V1.0.0

## 创作者

Written by wxzed (xiao.wu@dfrobot.com), 2026. (Welcome to our [website](https://www.dfrobot.com/))
