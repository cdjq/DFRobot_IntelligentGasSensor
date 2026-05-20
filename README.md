# DFRobot_IntelligentGasSensor

* [中文](./README_CN.md)

Arduino host library for **DFRobot SEN07xx intelligent gas sensors** over **Modbus RTU** (TTL UART or RS-485 with DE pin). Built on **[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)**; decodes **GAS_CODE** into gas type and unit on the host.

## Features

* Poll measurements via FC **0x04** (`readMeasurement` / `readMeasurementWithTimestamp`).
* `getConcentrationFloat()` from raw value and decimal point.
* `setDeviceAddress()` — write holding reg `0x0006` + COMMIT to EEPROM.
* `writeDeviceBaudCode()` — FC06 to holding `0x0003` only (no COMMIT); pair with `Serial*.begin()` + `commitConfiguration()` per `changeDeviceBaudrate`.
* `setDeviceBaudCode()` — `writeDeviceBaudCode()` + `commitConfiguration()` without changing host baud between frames; if firmware switches line speed immediately, use the three-step example.
* `setClientSlaveAddr()` — host-side slave ID only (no sensor EEPROM).
* `setProbeSleep()` / `readProbeSleepMode()` — SMX100 probe RUN/SLEEP via holding `0x0005` (not stored in EEPROM).

## Quick start

```cpp
#include <DFRobot_IntelligentGasSensor.h>

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
    Serial.println(gas.getConcentrationFloat(), 2);
  }
  delay(1000);
}
```

RS-485: pass DE pin as third argument, e.g. `DFRobot_IntelligentGasSensor gas(&Serial1, 1, 29);`

## Examples

| Sketch | Description |
|--------|-------------|
| `readGasUART` | Basic polling over TTL UART |
| `readGasRS485` | Basic polling over RS-485 (DE pin) |
| `readGasMultiSlave` | Multiple slaves on one RS-485 bus (omit DE arg for TTL) |
| `readGasMultiSlaveOneInstance` | Multiple slaves: one object + `setClientSlaveAddr` (no EEPROM write) |
| `probeSleepWake` | Serial keys S/W/P/M: probe sleep, wake, read mode, read gas |
| `changeDeviceAddress` | Change Modbus slave address |
| `changeDeviceBaudrate` | 9600→19200：写 0x0003 → `begin` 同速 → COMMIT，之后一直 19200 读数 |

## Docs

* [REGISTERMAP_MODBUS_CN.md](./docs/REGISTERMAP_MODBUS_CN.md) (Chinese register table)
* [MANUAL_TEST_RS485_MODBUS_CN.md](./docs/MANUAL_TEST_RS485_MODBUS_CN.md)

## Dependency

[DFRobot_RTU](https://github.com/DFRobot/DFRobot_RTU)

## License

MIT — see [LICENCE](./LICENCE).
