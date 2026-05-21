# SEN07xx 智能气体：Modbus RTU 寄存器表

本文档按「类型 | 地址 | 名称 | 读写 | 数据/范围 | 典型值 | 说明」整理。  
**规范来源**：Arduino 库 **`DFRobot_IntelligentGasSensor` V1.0.0**（`src/DFRobot_IntelligentGasSensor.h`），与 SEN07xx 传感器 Modbus RTU 从机行为一致。

> 部分通用 Modbus 表里「输入 0x0000=VID、0x0001=PID」。**本产品线为 0x0000=PID、0x0001=VID**。

**默认链路**：9600 8N1，从站地址 **1**（范围 1～247）。传感器对外为 **RS-485 端子 A/B**；主机经 **UART 转 RS485 模块** 连接（ESP32 等 3.3V 主控）。

---

## 一、功能码

| 类型 | 功能码 | 说明 |
|------|--------|------|
| 输入寄存器 | **0x04** | 只读；每寄存器 16 bit |
| 保持寄存器 | **0x03** 读 / **0x06** 单写 / **0x10** 多写 | 配置与 COMMIT |

**32 位浓度**：**0x0009（低字）+ 0x000A（高字）**，组合为 32 位无符号整数（低字在前）。

**主机库读长度**（`readMeasurement`）：

| API | FC 0x04 起始地址 | 寄存器个数 | 说明 |
|-----|------------------|------------|------|
| `readMeasurement(false)` | 0x0000 | **12**（0x0000～0x000B） | 默认，不含时间戳 |
| `readMeasurementWithTimestamp()` | 0x0000 | **18**（0x0000～0x0011） | 含年月日时分秒 |

---

## 二、输入寄存器（FC 0x04）

共 **18** 个（`0x0000`～`0x0011`）。库内宏前缀：`DFROBOT_IGS_IN_REG_*`。

| 类型 | 地址 | 名称 | 读写 | 典型值 | 说明 |
|------|------|------|------|--------|------|
| 输入 | 0x0000 | PID | R | 0xC746 | RS-485 板（SEN0746）；TTL 板 0xC742（SEN0742） |
| 输入 | 0x0001 | VID | R | 0x3343 | DFRobot |
| 输入 | 0x0002 | Modbus 从站地址 | R | 1～247 | 当前从站号 |
| 输入 | 0x0003～0x0004 | 保留 | R | 0 | 未使用 |
| 输入 | 0x0005 | 寄存器图版本 | R | 0x0100 | `DFROBOT_IGS_REGMAP_VERSION` |
| 输入 | 0x0006 | 板型 | R | 2 | **0**=未知，**1**=UART/TTL，**2**=RS-485 |
| 输入 | 0x0007 | 状态 | R | — | **bit0**=测量有效（`DFROBOT_IGS_STATUS_VALID_MSK`） |
| 输入 | 0x0008 | GAS_CODE | R | — | 气体码；类型/单位由**主机库**译码（见第五节） |
| 输入 | 0x0009 | 浓度低字 | R | — | 32 位浓度低 16 位 |
| 输入 | 0x000A | 浓度高字 | R | — | 32 位浓度高 16 位 |
| 输入 | 0x000B | 小数位 | R | 0～12 | 浓度 = 原码 × 10^−dp；库 `getConcentrationFloat()` |
| 输入 | 0x000C | 年 | R | 2000～2099 | 无 RTC 时可 0 |
| 输入 | 0x000D | 月 | R | 1～12 | |
| 输入 | 0x000E | 日 | R | 1～31 | |
| 输入 | 0x000F | 时 | R | 0～23 | |
| 输入 | 0x0010 | 分 | R | 0～59 | |
| 输入 | 0x0011 | 秒 | R | 0～59 | |

---

## 三、保持寄存器（FC 0x03 / 0x06 / 0x10）

共 **8** 个（`0x0000`～`0x0007`）。库内宏前缀：`DFROBOT_IGS_HOLD_REG_*`。

改**波特率 / 校验 / 从站号**后，向 **0x0007** 写 **0xA501**（`DFROBOT_IGS_COMMIT_KEY_SAVE_CONFIG`）**COMMIT** 方写入 EEPROM。  
**0x0005 探头休眠不入 EEPROM**，写后**立即生效**。

| 类型 | 地址 | 名称 | 读写 | 典型值 | 说明 |
|------|------|------|------|--------|------|
| 保持 | 0x0000 | 保留 | R/W | 0 | 读恒 0 |
| 保持 | 0x0001 | 保留 | R/W | 0 | 读恒 0 |
| 保持 | 0x0002 | 保留 | R/W | 0 | 读恒 0 |
| 保持 | 0x0003 | 波特率编码 | R/W | 3 | 见下表；写后 SEN07xx 常在**应答后立刻**切从站线速，主机须在 COMMIT 前 `Serial.begin` 新波特率（例程 `changeDeviceBaudrate`） |
| 保持 | 0x0004 | 校验/停止位 | R/W | 0 | 见下表；与 0x0003 一同 COMMIT 落盘 |
| 保持 | 0x0005 | 探头休眠 | R/W | 0 | **0**=RUN，**1**=SLEEP；立即生效，**不落盘**；库 `setProbeSleep()` / `readProbeSleepMode()` |
| 保持 | 0x0006 | 从站地址 | R/W | 1 | 1～247；写后**立即**按新地址应答；持久化须 COMMIT；库 `setDeviceAddress()` |
| 保持 | 0x0007 | COMMIT | W | 0xA501 | 将影子配置写入 EEPROM；库 `commitConfiguration()` |

### 3.1 波特率编码（0x0003）

| 编码 | 波特率 | 库宏 |
|------|--------|------|
| 1 | 2400 | `DFROBOT_IGS_BAUD_CODE_2400` |
| 2 | 4800 | `DFROBOT_IGS_BAUD_CODE_4800` |
| 3 | 9600 | `DFROBOT_IGS_BAUD_CODE_9600` |
| 4 | 14400 | `DFROBOT_IGS_BAUD_CODE_14400` |
| 5 | 19200 | `DFROBOT_IGS_BAUD_CODE_19200` |
| 6 | 38400 | `DFROBOT_IGS_BAUD_CODE_38400` |
| 7 | 57600 | `DFROBOT_IGS_BAUD_CODE_57600` |
| 8 | 115200 | `DFROBOT_IGS_BAUD_CODE_115200` |

### 3.2 校验/停止位（0x0004）

低 2 位为校验，bit2 为停止位（与库宏一致）：

| 字段 | 值 | 库宏 | 含义 |
|------|-----|------|------|
| 校验 | 0 | `DFROBOT_IGS_PARITY_NONE` | 无校验 N |
| 校验 | 1 | `DFROBOT_IGS_PARITY_ODD` | 奇校验 O |
| 校验 | 2 | `DFROBOT_IGS_PARITY_EVEN` | 偶校验 E |
| 停止位 | 0 | `DFROBOT_IGS_STOPBIT_1` | 1 停止位 |
| 停止位 | 4 (bit2) | `DFROBOT_IGS_STOPBIT_2` | 2 停止位 |

出厂默认一般为 **0**（8N1）。

---

## 四、主机库 API 与寄存器对应

| 库 API | 保持/输入 | 说明 |
|--------|-----------|------|
| `readMeasurement()` | 输入 0x0000～0x000B | FC 0x04，12 寄存器 |
| `readMeasurementWithTimestamp()` | 输入 0x0000～0x0011 | FC 0x04，18 寄存器 |
| `writeDeviceBaudCode(code)` | 保持 0x0003 | 仅 FC 0x06，不 COMMIT |
| `commitConfiguration()` | 保持 0x0007 = 0xA501 | FC 0x06 |
| `setDeviceAddress(addr)` | 保持 0x0006 + COMMIT | |
| `setProbeSleep(sleep)` | 保持 0x0005 | 0=RUN，1=SLEEP |
| `setClientSlaveAddr(addr)` | — | 仅改主机请求地址，不写寄存器 |

---

## 五、GAS_CODE 主机译码表（SMX100）

总线只上报 **GAS_CODE**；`gasType` / `unit` 由库内表生成（未知码显示为 `0xNN`）。

| GAS_CODE | 气体类型 | 默认单位 |
|----------|----------|----------|
| 0x01 | CH4 | %LEL |
| 0x03 | H2S | ppm |
| 0x04 | CO | ppm |
| 0x05 | O2 | %VOL |

---

## 六、相关文件

| 内容 | 路径 |
|------|------|
| 主机库头文件/实现 | `src/DFRobot_IntelligentGasSensor.{h,cpp}` |
| 库说明 | `README_CN.md` / `README.md` |
| 手动联调帧 | `docs/MANUAL_TEST_RS485_MODBUS_CN.md` |
| 例程 | `examples/readGasRS485`、`changeDeviceBaudrate` 等 |

*文档版本：V1.0.0，2026-05-21，与 `DFRobot_IntelligentGasSensor` 库对齐。*
