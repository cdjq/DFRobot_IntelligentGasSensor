# DFRobot_IntelligentGasSensor 智能气体传感器库（中文版）

* [English](./README.md)

面向 **SEN07xx Modbus RTU 固件** 的 Arduino 主机库：在 **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)** 之上完成组帧/CRC，并根据 SMX100 **气体码** 译码类型与单位。支持 **TTL UART** 与 **RS-485**（构造时传入 DE 引脚）。

## 目录

* [概述](#概述)
* [依赖与安装](#依赖与安装)
* [接线](#接线)
* [快速开始](#快速开始)
* [API 摘要](#api-摘要)
* [示例](#示例)
* [寄存器文档](#寄存器文档)
* [许可证](#许可证)

## 概述

* `readMeasurement()`：FC 0x04 读 12 个输入寄存器（至小数位，默认不含时间戳）。
* `readMeasurementWithTimestamp()`：读满 18 个寄存器（含 RTC 时间）。
* `getConcentrationFloat()`：`concentrationRaw × 10^(-decimalPoint)`。
* `setDeviceAddress()`：写保持 `0x0006` 并 COMMIT，修改传感器 Modbus 从站号。
* `writeDeviceBaudCode()`：仅写保持 `0x0003`（不切主机波特率、不 COMMIT）；配合 `begin` + `commitConfiguration()` 见例程 `changeDeviceBaudrate`。
* `setDeviceBaudCode()`：连续 `writeDeviceBaudCode` + `commitConfiguration`；若固件写 0x0003 后立即切线速，须用例程三步。
* `setClientSlaveAddr()`：仅改主机侧请求目标地址，不写传感器 EEPROM。
* `setProbeSleep()` / `readProbeSleepMode()`：写/读保持 `0x0005` 控制 SMX100 探头 RUN/SLEEP（**不落 EEPROM**）。

## 依赖与安装

* 依赖 **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)**。
* 将本库放入 `libraries` 后重启 IDE。

## 接线

| 模块 | 主控 |
|------|------|
| VCC / GND | 按模块要求 |
| UART RX | 主控 TX |
| UART TX | 主控 RX |
| RS-485 DE | 构造函数第三参 `dePin` |

波特率须与传感器一致（常见 **9600 8N1**）。使用硬件串口（`Serial1` / `Serial2`）。

## 快速开始

```cpp
#include <DFRobot_IntelligentGasSensor.h>

// TTL:  readGasUART
// RS-485: readGasRS485 (DE=29 on SEN07xx RP2040 module)
DFRobot_IntelligentGasSensor gas(&Serial1, 1);

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600);
}

void loop() {
  if (gas.readMeasurementWithTimestamp() == 0) {
    if (gas.lastMeasure.timestamp[0]) {
      Serial.print('[');
      Serial.print(gas.lastMeasure.timestamp);
      Serial.print("] ");
    }
    Serial.print(gas.lastMeasure.gasType);
    Serial.print(' ');
    Serial.print(gas.getConcentrationFloat(), 2);
    Serial.print(' ');
    Serial.println(gas.lastMeasure.unit);
  }
  delay(1000);
}
```

## API 摘要

| API | 说明 |
|-----|------|
| `DFRobot_IntelligentGasSensor(Stream*, slave, dePin=-1)` | 构造；`dePin=-1` 为 TTL |
| `readMeasurement(withTimestamp=false)` | 读测量，填充 `lastMeasure` |
| `readMeasurementWithTimestamp()` | 含时间戳寄存器 |
| `getConcentrationFloat()` | 浮点浓度 |
| `setClientSlaveAddr` / `getClientSlaveAddr` | 仅主机侧从站号 |
| `setDeviceAddress` | 写传感器从站号 + EEPROM |
| `writeDeviceBaudCode` | 仅写传感器波特率寄存器 0x0003 |
| `setDeviceBaudCode` | 连续写波特率 + COMMIT（见头文件 @note；立即切线速固件请用例程三步） |
| `setProbeSleep` / `readProbeSleepMode` | 探头休眠（保持 0x0005，立即生效） |
| `fillLastMeasureFromInputRegs` | 静态译码（高级） |

返回值 `0` 表示成功；非 `0` 为 DFRobot_RTU 错误码。

## 示例

| 例程 | 说明 |
|------|------|
| `readGasUART` | 基础轮询（TTL UART，无 DE） |
| `readGasRS485` | 基础轮询（RS-485，含 DE 引脚） |
| `readGasMultiSlave` | 同一 RS-485 总线多从站（TTL 可去掉构造第三参 DE） |
| `readGasMultiSlaveOneInstance` | 多从站：单实例 + `setClientSlaveAddr` 切换（不写 EEPROM） |
| `probeSleepWake` | 串口发 S/W/P/M：休眠、唤醒、读模式、读浓度 |
| `changeDeviceAddress` | 修改从站地址并 COMMIT |
| `changeDeviceBaudrate` | 9600→19200：写 0x0003 → `begin` 同速 → COMMIT，之后一直 19200 轮询 |

## 寄存器文档

* [REGISTERMAP_MODBUS_CN.md](./docs/REGISTERMAP_MODBUS_CN.md)
* [MANUAL_TEST_RS485_MODBUS_CN.md](./docs/MANUAL_TEST_RS485_MODBUS_CN.md) — 十六进制帧手工联调

## 许可证

MIT — 见 [LICENCE](./LICENCE)。
