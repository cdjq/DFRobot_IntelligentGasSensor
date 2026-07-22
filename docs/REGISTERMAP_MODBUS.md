# SEN07xx Intelligent Gas Sensor: Modbus RTU Register Map

* [中文版](./REGISTERMAP_MODBUS_CN.md)

This document describes the register map using the columns "type, address, name, access, typical
value, and description."
**Specification source:** Arduino library **`DFRobot_IntelligentGasSensor` V1.0.0**
(`src/DFRobot_IntelligentGasSensor.h`). The definitions match the Modbus RTU slave behavior of the
SEN07xx sensors.

> Some generic Modbus tables define input register `0x0000` as VID and `0x0001` as PID. **For this
> product family, `0x0000` is PID and `0x0001` is VID.**

**Default link settings:** 9600 8N1, slave address **1** (valid range: 1 to 247). The sensor exposes
**RS-485 A/B** terminals. Connect a 3.3 V host such as an ESP32 through a UART-to-RS-485 converter.

---

## 1. Function Codes

| Type | Function code | Description |
| ---- | ------------- | ----------- |
| Input registers | **0x04** | Read-only; each register is 16 bits |
| Holding registers | **0x03** read / **0x06** single write / **0x10** multiple write | Configuration and COMMIT |

**32-bit concentration:** combine **0x0009 (low word) + 0x000A (high word)** into an unsigned
32-bit value, with the low word first.

**Host-library read lengths** (`readGasMeasurementData`):

| API | FC 0x04 start address | Register count | Description |
| --- | --------------------- | -------------- | ----------- |
| `readGasMeasurementData(false)` | 0x0000 | **12** (`0x0000` to `0x000B`) | Default; no timestamp |
| `readGasMeasurementData(true)` | 0x0000 | **18** (`0x0000` to `0x0011`) | Includes year, month, day, hour, minute, and second |

---

## 2. Input Registers (FC 0x04)

There are **18** input registers (`0x0000` to `0x0011`). Their library macro prefix is
`DFROBOT_IGS_IN_REG_*`.

| Type | Address | Name | Access | Typical value | Description |
| ---- | ------- | ---- | ------ | ------------- | ----------- |
| Input | 0x0000 | PID | R | 0xC746 | RS-485 board (SEN0746); TTL board: 0xC742 (SEN0742) |
| Input | 0x0001 | VID | R | 0x3343 | DFRobot |
| Input | 0x0002 | Modbus slave address | R | 1 to 247 | Current slave address |
| Input | 0x0003 | Reserved 0 | R | 0 | Always reads 0 |
| Input | 0x0004 | Reserved 1 | R | 0 | Always reads 0 |
| Input | 0x0005 | Register-map version | R | 0x0100 | `DFROBOT_IGS_REGMAP_VERSION` |
| Input | 0x0006 | Reserved 2 | R | 0 | Always reads 0; **not a board-type or RUN/SLEEP field** (probe RUN/SLEEP is holding register **0x0005** only) |
| Input | 0x0007 | Status | R | — | **bit 0** = measurement valid (`DFROBOT_IGS_STATUS_VALID_MSK`) |
| Input | 0x0008 | GAS_CODE | R | — | Gas code; the **host library** decodes type and unit (see Section 5) |
| Input | 0x0009 | Concentration low word | R | — | Low 16 bits of the 32-bit concentration |
| Input | 0x000A | Concentration high word | R | — | High 16 bits of the 32-bit concentration |
| Input | 0x000B | Decimal places | R | 0 to 12 | Concentration = raw value × 10^−dp; library method `getConcentrationFloat()` |
| Input | 0x000C | Year | R | 2000 to 2099 | May be 0 when no RTC is available |
| Input | 0x000D | Month | R | 1 to 12 | |
| Input | 0x000E | Day | R | 1 to 31 | |
| Input | 0x000F | Hour | R | 0 to 23 | |
| Input | 0x0010 | Minute | R | 0 to 59 | |
| Input | 0x0011 | Second | R | 0 to 59 | |

---

## 3. Holding Registers (FC 0x03 / 0x06 / 0x10)

There are **8** holding registers (`0x0000` to `0x0007`). Their library macro prefix is
`DFROBOT_IGS_HOLD_REG_*`.

After changing the **baud rate, parity, or slave address**, write **0xA501**
(`DFROBOT_IGS_COMMIT_KEY_SAVE_CONFIG`) to **0x0007** to **COMMIT** the shadow configuration to
EEPROM. **Probe sleep at 0x0005 is not stored in EEPROM** and takes effect immediately.

| Type | Address | Name | Access | Typical value | Description |
| ---- | ------- | ---- | ------ | ------------- | ----------- |
| Holding | 0x0000 | Reserved | R/W | 0 | Always reads 0 |
| Holding | 0x0001 | Reserved | R/W | 0 | Always reads 0 |
| Holding | 0x0002 | Reserved | R/W | 0 | Always reads 0 |
| Holding | 0x0003 | Baud-rate code | R/W | 3 | See the table below. After replying, SEN07xx commonly changes its active baud immediately; the host must call `Serial.begin` with the new baud before COMMIT (see `changeDeviceBaudrate`) |
| Holding | 0x0004 | Parity / stop bits | R/W | 0 | See the table below; saved together with 0x0003 by COMMIT |
| Holding | 0x0005 | Probe sleep | R/W | 0 | **0** = RUN, **1** = SLEEP; immediate and **not saved**; library methods `setProbeSleep()` / `getProbeWorkMode()` |
| Holding | 0x0006 | Slave address | R/W | 1 | 1 to 247; replies using the new address immediately; COMMIT is required for persistence; library method `setDeviceAddress()` |
| Holding | 0x0007 | COMMIT | W | 0xA501 | Save the shadow configuration to EEPROM; library method `commitConfiguration()` |

### 3.1 Baud-rate Code (0x0003)

| Code | Baud rate | Library macro |
| ---- | --------- | ------------- |
| 1 | 2400 | `DFROBOT_IGS_BAUD_CODE_2400` |
| 2 | 4800 | `DFROBOT_IGS_BAUD_CODE_4800` |
| 3 | 9600 | `DFROBOT_IGS_BAUD_CODE_9600` |
| 4 | 14400 | `DFROBOT_IGS_BAUD_CODE_14400` |
| 5 | 19200 | `DFROBOT_IGS_BAUD_CODE_19200` |
| 6 | 38400 | `DFROBOT_IGS_BAUD_CODE_38400` |
| 7 | 57600 | `DFROBOT_IGS_BAUD_CODE_57600` |
| 8 | 115200 | `DFROBOT_IGS_BAUD_CODE_115200` |

### 3.2 Parity / Stop Bits (0x0004)

The lower two bits select parity, and bit 2 selects the number of stop bits, matching the library
macros:

| Field | Value | Library macro | Meaning |
| ----- | ----- | ------------- | ------- |
| Parity | 0 | `DFROBOT_IGS_PARITY_NONE` | No parity (N) |
| Parity | 1 | `DFROBOT_IGS_PARITY_ODD` | Odd parity (O) |
| Parity | 2 | `DFROBOT_IGS_PARITY_EVEN` | Even parity (E) |
| Stop bits | 0 | `DFROBOT_IGS_STOPBIT_1` | 1 stop bit |
| Stop bits | 4 (bit 2) | `DFROBOT_IGS_STOPBIT_2` | 2 stop bits |

The factory default is normally **0** (8N1).

---

## 4. Host-library API to Register Mapping

| Library API | Register range | Description |
| ----------- | -------------- | ----------- |
| `readGasMeasurementData(false)` | Input `0x0000` to `0x000B` | FC 0x04, 12 registers |
| `readGasMeasurementData(true)` | Input `0x0000` to `0x0011` | FC 0x04, 18 registers |
| `writeDeviceBaudCode(code)` | Holding `0x0003` | FC 0x06 only; no COMMIT |
| `commitConfiguration()` | Holding `0x0007` = `0xA501` | FC 0x06 |
| `setDeviceAddress(addr)` | Holding `0x0006` + COMMIT | |
| `setProbeSleep(sleep)` | Holding `0x0005` | 0 = RUN, 1 = SLEEP |
| `setClientSlaveAddr(addr)` | — | Changes only the address used by host requests; does not write a register |

---

## 5. Host-side GAS_CODE Table (SMX100)

The bus reports only **GAS_CODE**. The library generates `gasType` and `unit` from its internal
table. An unknown code is displayed as `0xNN`.

| GAS_CODE | Gas type | Default unit |
| -------- | -------- | ------------ |
| 0x01 | CH4 | %LEL |
| 0x03 | H2S | ppm |
| 0x04 | CO | ppm |
| 0x05 | O2 | %VOL |

---

## 6. Related Files

| Content | Path |
| ------- | ---- |
| Host-library header and implementation | `src/DFRobot_IntelligentGasSensor.{h,cpp}` |
| Library documentation | `README.md` / `README_CN.md` |
| Manual Modbus test frames (Chinese) | `docs/MANUAL_TEST_RS485_MODBUS_CN.md` |
| Examples | `examples/readGasRS485`, `examples/changeDeviceBaudrate`, and others |

*Document version: V1.0.0, 2026-05-21. Aligned with the `DFRobot_IntelligentGasSensor` library.*
