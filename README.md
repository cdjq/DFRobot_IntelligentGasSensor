# DFRobot_IntelligentGasSensor

* [中文版](./README_CN.md)

Arduino host library for **DFRobot SEN07xx intelligent gas sensors** over **Modbus RTU**. Built on **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)**. Main features:<br>
* FC **0x04**: Read gas measurement input registers (`readGasMeasurementData`);
* Host-side decode **GAS_CODE** to gas type and unit;
* FC **0x06** + COMMIT: Change Modbus slave address (`setDeviceAddress`);
* FC **0x06**: Write baud holding register `0x0003` (`writeDeviceBaudCode` / `setDeviceBaudCode`);
* FC **0x06**: Probe RUN/SLEEP via holding `0x0005` (`setProbeSleep` / `getProbeWorkMode`, not EEPROM);
* `setClientSlaveAddr()`: Switch target slave on host only (no sensor EEPROM).

## Product Link（[https://www.dfrobot.com](https://www.dfrobot.com)）

```text
SKU: SEN0738 / SEN0739 / SEN0740 / SEN0741
```


## Table of Contents

* [Summary](#summary)
* [Connected](#connected)
* [Installation](#installation)
* [CDC Configuration](#cdc-configuration)
* [Methods](#methods)
* [Compatibility](#compatibility)
* [History](#history)
* [Credits](#credits)

## Summary

This library drives SEN07xx gas sensors as Modbus RTU slaves. It extends **DFRobot_RTU** for framing/CRC and parses sensor input registers into `lastMeasure`. Default link: **9600 8N1**, slave address **1**.<br>

See `examples/` for `readGasRS485`, `readGasUART`, `changeDeviceAddress`, `changeDeviceBaudrate`, etc. Register map: [REGISTERMAP_MODBUS.md](./docs/REGISTERMAP_MODBUS.md).

## Connected

Hardware conneted table (ESP32 example; the module supports either **RS-485 A/B** through a UART-to-RS-485 adapter or direct TTL UART wiring to the sensor. Use **5 V** for direct sensor wiring.)<br>

| ESP32 pin | UART-to-RS-485 module |
|-----------|----------------------|
| 3.3V | VCC |
| GND | GND |
| GPIO17 (TX) | DI |
| GPIO36 (RX) | RO |
| GPIO16 | DE/RE (direction; `dePin` in constructor) |

| UART-to-RS-485 module | Sensor (SEN07xx) |
|----------------------|------------------|
| A | A |
| B | B |

 ESP32 pin| Sensor (SEN07xx) |
|------------|-------------------|
| 5.0V | VCC |
| GND | GND |
| GPIO17 (TX) | RX |
| GPIO36 (RX) | TX |

Note: Direct TTL wiring (no DE) is supported, but the sensor VCC must be connected to **5 V**. See `readGasUART`.

## Installation

1. Install dependency **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)** into `Arduino/libraries`.
2. Download this library into `Arduino/libraries`, restart the IDE.
3. Open the `examples` folder and run a demo (e.g. `readGasRS485`).

## CDC Configuration

Besides the **Modbus RTU host port** (TTL / RS-485), SEN07xx modules expose a **USB CDC** virtual COM port for local setup and status (not Modbus frames on the RS-485 bus).

| Item | Note |
|------|------|
| Connection | USB to PC; open the module’s **COM port** in a serial monitor |
| Monitor | **115200 8N1** recommended (CDC / SenLog; independent of Modbus line speed, often 9600) |
| Line ending | Press Enter after each command; type `help` for the full list |

Typical banner after boot:

```text
=== available commands ===
  help                          show this help
  status                        print sensor + RTC + Modbus summary
  settime YYYY-MM-DD HH:MM:SS   set RTC time
  setinterval <1-60>            CSV every N minutes (saved)
  logger on|off                 enable/disable CSV recorder (saved, default off)
  repairfs                      read-write USB disk for PC filesystem repair
                                (power cycle -> read-only default; stops logging)
  sensor sleep                  SMX100 probe SLEEP
  sensor wake                   SMX100 probe RUN
  modbus                        show host UART / slave settings
  setaddr <1-247>               set Modbus slave ID (saved to EEPROM)
  setbaud <2400|4800|9600|...>  set host UART baud (saved)
  setparity <8N1|8E1|8O1|8N2>   set host UART format (saved)
  modbus default                restore slave=1, 9600 8N1 (saved)
```

### Command reference

| Command | Description |
|---------|-------------|
| `help` | Print command list |
| `status` | Gas reading validity, probe RUN/SLEEP, RTC, Modbus slave ID and UART settings |
| `settime YYYY-MM-DD HH:MM:SS` | Set RTC (e.g. `settime 2026-05-21 11:15:00`) |
| `setinterval <1-60>` | Set CSV logging interval (minutes) |
| `logger on\off` | Enable/disable CSV recorder (saved) |
| `repairfs` | Read-write USB disk for PC filesystem repair (power cycle -> read-only default; stops logging) |
| `sensor sleep` | SMX100 probe **SLEEP** (immediate) |
| `sensor wake` | SMX100 probe **RUN** (immediate) |
| `modbus` | Show Modbus slave address, host UART baud rate and frame format |
| `setaddr <1-247>` | Change slave ID and **save to EEPROM** |
| `setbaud <rate>` | Change host UART baud and save: `2400` `4800` `9600` `14400` `19200` `38400` `57600` `115200` |
| `setparity <fmt>` | Change host UART format and save: `8N1` `8E1` `8O1` `8N2` |
| `modbus default` | Factory restore: **slave 1, 9600 8N1**, saved to EEPROM |

### Mapping to Modbus registers / this library

| CDC command | Holding register | Arduino host (this library) |
|-------------|------------------|----------------------------|
| `setaddr` | `0x0006` slave addr + `0x0007` COMMIT | `setDeviceAddress()` |
| `setbaud` | `0x0003` baud code + COMMIT | `writeDeviceBaudCode()` + `Serial.begin` + `commitConfiguration()` (see `changeDeviceBaudrate`) |
| `setparity` | `0x0004` parity/stop + COMMIT | Write holding `0x0004` then COMMIT (no dedicated API yet) |
| `sensor sleep` / `sensor wake` | `0x0005` probe sleep (**not EEPROM**) | `setProbeSleep(true/false)` / `getProbeWorkMode()` |
| `modbus` / `status` | — | Read input regs / `readGasMeasurementData()`, etc. |

Notes:

* **CDC and Modbus are separate paths**: PC uses CDC for configuration; ESP32 etc. use RS-485/TTL Modbus to read gas data.
* After changing baud via CDC, the Arduino host must `HOST_SERIAL.begin(...)` at the **same** rate.
* Probe RUN/SLEEP is **holding register 0x0005 only**; input `0x0006` is reserved (no board-type field).

## Methods

```C++
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
uint8_t getClientSlaveAddr(void) const;

/**
 * @brief Parse input register table into a measurement structure.
 * @param out:  Output measurement structure.
 * @param t:  Input register array (starting at address 0).
 * @param regCount:  Number of registers in t (12 without timestamp, 18 with timestamp).
 */
static void fillLastMeasureFromInputRegs(DFRobot_IntelligentGasSensorMeasure_t *out, const uint16_t *t, uint16_t regCount);

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
uint8_t getProbeWorkMode(bool *outSleep);

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
```

## Compatibility

MCU                | RS-485 (via adapter) | TTL UART (5V) |
------------------ | :------------------: | :-------------------: |
ESP32              |          √           |           √           |
Mega2560 (5 V)     |          √           |           √           |
Leonardo           |          √           |           √           |
micro:bit          |          X           |           X           |

Requires **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)** and a hardware UART (`Serial1` / `Serial2`).

## History

- Date 2026-05-21
- Version V1.0.0

## Credits

Written by wxzed (xiao.wu@dfrobot.com), 2026. (Welcome to our [website](https://www.dfrobot.com/))
