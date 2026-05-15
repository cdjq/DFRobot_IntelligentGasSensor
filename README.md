# DFRobot_IntelligentGasSensor

* [中文版](./README_CN.md)

Arduino host library for **DFRobot intelligent gas sensors** running **Modbus RTU** firmware (SEN07xx family). It builds on **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)** for framing and CRC, and adds register decoding: gas type / unit from SMX100 **GAS_CODE**, raw concentration with decimal scaling, optional timestamp block, SEN0742-style UART **active report** reception, and high-level helpers such as **acquire mode** and **device address** programming.

## Table of contents

* [Summary](#summary)
* [Dependency](#dependency)
* [Installation](#installation)
* [Wiring](#wiring)
* [Constructor note](#constructor-note)
* [Slave address: host vs sensor](#slave-address-host-vs-sensor)
* [Methods](#methods)
* [Examples](#examples)
* [Advanced: base class `DFRobot_RTU`](#advanced-base-class-dfrobot_rtu)
* [Compatibility](#compatibility)
* [License](#license)

## Summary

* Poll the sensor with `readMeasurement()` (default: 12 input registers, no RTC block) or `readMeasurementWithTimestamp()` for the full 18 registers.
* Read decoded fields from `lastMeasure` and a float concentration via `getConcentrationFloat()`.
* **SEN0742 TTL UART**: switch **passive Modbus poll** vs **active FC 0x04 push** using `getAcquireMode` / `setAcquireMode`; in active mode use `pollUnsolicitedAutoReport()` to assemble frames from RX only.
* Change the sensor’s Modbus slave ID on the bus with `setDeviceAddress()` (writes holding registers + EEPROM commit as required by firmware).

Register layout matches the sensor firmware register map (GAS_CODE on the bus; type/unit decoded on the host).

## Dependency

* **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)** — declared in `library.properties` as `depends=DFRobot_RTU`. Install both libraries.

## Installation

1. **Arduino Library Manager** (if published): search `DFRobot_IntelligentGasSensor` and install; ensure `DFRobot_RTU` is installed.
2. **Manual**: clone or copy this folder under `Arduino/libraries` (or `Arduino15/libraries`), then restart the IDE and open **File → Examples → DFRobot_IntelligentGasSensor**.

## Wiring

| Sensor / RS-485 board | MCU |
|----------------------|-----|
| VCC | 3.3 V or 5 V (per module) |
| GND | GND |
| UART RX | MCU UART **TX** (Modbus line) |
| UART TX | MCU UART **RX** |
| RS-485 **DE / RE** (if used) | GPIO passed as `dePin` in the constructor |

Baud rate must match the sensor (often **9600 8N1** in examples). Use a **hardware UART** (`Serial1`, `Serial2`, …) for reliable timing.

## Constructor note

The second parameter is always **Modbus slave address**; the third is **RS-485 DE** (default **`-1`** = TTL, no DE). Example:

```cpp
DFRobot_IntelligentGasSensor gas(&Serial1, 5);        // TTL, slave 5
DFRobot_IntelligentGasSensor gas485(&Serial2, 5, 4); // RS-485 DE=GPIO4, slave 5
```

So literals like `5` always mean **slave 5**, not a DE pin. Full signatures and return values are under **[Methods](#methods)**.

## Slave address: host vs sensor

| API | Effect |
|-----|--------|
| `setClientSlaveAddr(addr)` | **Host only**: next Modbus requests use this slave ID. Does **not** write the sensor’s EEPROM. |
| `getClientSlaveAddr()` | Returns the slave ID the host object will use on the wire. |
| `setDeviceAddress(newAddr)` | **Sensor**: writes the new Modbus address to the module and commits; the library also calls `setClientSlaveAddr(newAddr)` so the host tracks the new ID immediately. |

## Methods

```C++
/**
 * @brief Construct Modbus host for DFRobot intelligent gas sensor on a UART Stream.
 * @param s Pointer to hardware serial (e.g. Serial1, Serial2). Must be started with matching baud (often 9600 8N1).
 * @param slaveAddr Modbus slave ID in range 1–247; used for all subsequent requests from this object.
 * @param dePin RS-485 driver enable pin (same semantics as DFRobot_RTU). Use default -1 for TTL UART without DE control.
 */
DFRobot_IntelligentGasSensor(Stream *s, uint8_t slaveAddr, int dePin = -1);

/**
 * @brief Update only the host-side slave ID (the next Modbus ADU will use this address).
 * @param addr New slave ID 1–247.
 * @n Does not access sensor holding registers or EEPROM. To change the sensor’s own address on the bus, use setDeviceAddress().
 */
void setClientSlaveAddr(uint8_t addr);

/**
 * @brief Read the Modbus slave ID this object currently uses for requests.
 * @return Slave address 1–247.
 */
uint8_t getClientSlaveAddr(void) const;

/**
 * @brief UART gas-data acquire mode (SEN0742: holding register 0x0001). Other hardware may ignore.
 * @n ACQUIRE_MODE_PASSIVE: host polls with Modbus. ACQUIRE_MODE_ACTIVE: slave may push FC 0x04 measurement frames on UART.
 * @n ACQUIRE_MODE_UNKNOWN: holding register value was not 0 or 1.
 */
enum AcquireMode : uint8_t {
    ACQUIRE_MODE_PASSIVE = 0,
    ACQUIRE_MODE_ACTIVE = 1,
    ACQUIRE_MODE_UNKNOWN = 0xFF
};

/**
 * @brief Read current acquire mode (reads holding register shadow via Modbus).
 * @param mode Output: 0 passive, 1 active, 0xFF unknown; only valid if return value is 0.
 * @return 0 success; non-zero DFRobot_RTU / Modbus receive or bus error — do not read *mode.
 */
uint8_t getAcquireMode(uint8_t *mode);

/**
 * @brief Set passive or active UART acquire mode; optionally COMMIT to sensor EEPROM.
 * @param mode Must be ACQUIRE_MODE_PASSIVE (0) or ACQUIRE_MODE_ACTIVE (1); may use sensor.ACQUIRE_MODE_* in sketches.
 * @param commitToEeprom If true, writes commit key so the mode survives power-off together with other saved serial config per firmware.
 * @return 0 success; 3 if mode is illegal; other non-zero values from RTU write or COMMIT failure.
 */
uint8_t setAcquireMode(uint8_t mode, bool commitToEeprom = true);

/**
 * @brief Read input registers (FC 0x04) from register 0 and decode into lastMeasure.
 * @param withTimestamp false (default): read 12 input registers (no RTC timestamp block). true: read 18 registers including six timestamp registers.
 * @return 0 success; non-zero Modbus / RTU error — treat measurement as invalid for this call.
 */
uint8_t readMeasurement(bool withTimestamp = false);

/**
 * @brief Equivalent to readMeasurement(true): full input table including timestamp when firmware provides RTC.
 * @return 0 success; non-zero RTU / Modbus error.
 */
uint8_t readMeasurementWithTimestamp(void);

/**
 * @brief Floating-point concentration from last successful decode.
 * @n Computes concentrationRaw × 10^(−decimalPoint). Returns NAN if decimalPoint is out of supported range.
 */
float getConcentrationFloat(void) const;

/**
 * @brief Last decoded measurement snapshot (meaningful after readMeasurement() returns 0, or after pollUnsolicitedAutoReport() returns 0).
 * @n Contains pid, vid, modbusAddr, gasType, unit, gasCode, concentrationRaw, decimalPoint, optional timestamp string, dataValid, etc.
 */
DFRobot_IntelligentGasSensorMeasure_t lastMeasure;

/**
 * @brief Active-report receive path: drain constructor Stream, detect frame gap, parse one FC 0x04–shaped response into lastMeasure. Sends no Modbus request.
 * @return 0 parsed a new valid frame; 1 no complete frame in this call; 9 Modbus Stream was null at construction.
 */
uint8_t pollUnsolicitedAutoReport(void);

/**
 * @brief Program the sensor’s Modbus slave address (holding 0x0006) and EEPROM commit per firmware; then sync host addressing.
 * @param newAddr Target slave ID 1–247; must differ from current sensor address to perform a write.
 * @return 0 success; 3 if newAddr is out of range; other non-zero RTU write / commit errors.
 */
uint8_t setDeviceAddress(uint8_t newAddr);
```

Additional inherited members from **`DFRobot_RTU`** (e.g. `setTimeoutTimeMs`, raw `readInputRegister` / `writeHoldingRegister`, …) are available on the same object for advanced use; see the [DFRobot_RTU README](https://github.com/DFRobot/DFRobot_RTU).

## Examples

| Sketch | Purpose |
|--------|---------|
| `readGasModbus` | Minimal poll: gas name, value, unit. TTL / RS-485 selectable by macro. |
| `readGasMultiSlave` | Multiple `DFRobot_IntelligentGasSensor` instances on one UART, different slave IDs. |
| `changeMode` | Flip UART acquire mode (passive ↔ active) with `getAcquireMode` / `setAcquireMode`. |
| `uartAutoReportListen` | Enable active report, then `pollUnsolicitedAutoReport()` in a tight loop. |
| `changeDeviceAddress` | Run once to move the sensor to a new Modbus address (`setDeviceAddress`). |

## Advanced: base class `DFRobot_RTU`

`DFRobot_IntelligentGasSensor` extends `DFRobot_RTU`. You may call inherited methods (e.g. other FC 0x03/0x04 access) if your application needs raw register access. Timeout defaults are tuned in the gas sensor constructor (`setTimeoutTimeMs(200)`); adjust if your bus is slow or very long.

## Compatibility

* **Architectures**: `*` (Arduino-AVR, ESP32, RP2040, SAMD, … as supported by `DFRobot_RTU` and your board’s `Stream` / UART).
* **Sensor firmware**: SEN07xx Modbus register map; SEN0742 UART active report supported where firmware provides it.

## License

MIT License — see header in `src/DFRobot_IntelligentGasSensor.h`.
