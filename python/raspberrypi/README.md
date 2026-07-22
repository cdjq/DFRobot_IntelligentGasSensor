# DFRobot_IntelligentGasSensor

- [中文版](./README_CN.md)

DFRobot SEN07xx intelligent gas sensors report gas type, concentration, measurement status, and an
optional timestamp through Modbus RTU. This Raspberry Pi Python library reads CH4, H2S, CO, and O2
measurements and supports changing the slave address, changing the communication baud rate,
controlling probe sleep, and polling multiple sensors on one RS-485 bus.<br>
The driver is based on `DFRobot_RTU.py` and supports direct TTL UART or an automatic-direction
UART-to-RS-485 converter.

## Product Link ([https://www.dfrobot.com](https://www.dfrobot.com))

    SKU: SEN0738 / SEN0739 / SEN0740 / SEN0741

## Table of Contents

  * [Summary](#summary)
  * [Installation](#installation)
  * [Methods](#methods)
  * [Compatibility](#compatibility)
  * [History](#history)
  * [Credits](#credits)

## Summary

This library communicates with SEN07xx as a Modbus RTU slave. The default communication settings
are **9600 8N1**, the default slave address is **1**, and valid slave addresses range from 1 to 247.
The underlying serial device is `/dev/ttyAMA0`.

Main features:

* Read gas type, concentration, status, and an optional timestamp;
* Convert the raw 32-bit concentration to a scaled floating-point value;
* Change and save the Modbus slave address;
* Change the sensor baud rate and switch the Raspberry Pi UART accordingly;
* Put the probe into RUN or SLEEP mode;
* Poll multiple Modbus slaves with multiple objects or one object.

After a successful read, data is stored in the `sensor.last_measure` dictionary. Common fields are
`gas_type`, `concentration_raw`, `decimal_point`, `unit`, `timestamp`, and `data_valid`. Call
`get_concentration_float()` to obtain the scaled concentration directly.

Examples:

| File | Function |
| ---- | -------- |
| `01.change_device_address.py` | Change and save the sensor slave address |
| `02.change_device_baudrate.py` | Change and save the sensor baud rate |
| `03.probe_sleep_wake.py` | Sleep, wake, and query the probe state |
| `04.read_gas_multi_slave.py` | Read multiple slaves with objects sharing one UART |
| `05.read_gas_multi_slave_one_instance.py` | Read multiple slaves by changing one object's target address |
| `06.read_gas_uart.py` | Continuously read gas concentration through UART |

For the complete register description, see
[REGISTERMAP_MODBUS.md](../../docs/REGISTERMAP_MODBUS.md).

## Installation

Download the library to the Raspberry Pi and install `pyserial`. The required `DFRobot_RTU.py` is
included in this directory.

```bash
sudo apt update
sudo apt install -y python3-serial
git clone https://github.com/DFRobot/DFRobot_IntelligentGasSensor.git
```

Before using the Raspberry Pi UART, run `sudo raspi-config`, disable the serial login shell, enable
the serial hardware, and reboot. If access to the serial port is denied, add the current user to the
`dialout` group:

```bash
sudo usermod -aG dialout $USER
```

Open the example directory and run an example. To run the UART measurement example:

```bash
cd DFRobot_IntelligentGasSensor/python/raspberrypi
python3 example/06.read_gas_uart.py
```

Before running an example, set the correct baud rate and slave address as described at the top of the file. RS-485 wiring requires an automatic-direction
converter; the current Python RTU library does not control a separate `DE/RE` pin.

## Methods

```python
  '''!
    @brief Initialize the UART and create an intelligent gas sensor object
    @param baud UART baud rate, default 9600
    @param slave_addr Modbus slave address from 1 to 247, default 1
    @param bits UART data bits, default 8
    @param parity UART parity, default "N"
    @param stopbit UART stop bits, default 1
    @param shared_rtu Another DFRobot_RTU object when sharing one physical UART
  '''
  def __init__(self, baud=9600, slave_addr=1, bits=8, parity="N", stopbit=1, shared_rtu=None):

  '''!
    @brief Change the host-side slave address used by later requests
    @param addr Modbus slave address from 1 to 247
    @return True when accepted, otherwise False
    @note This method does not change the sensor address or write EEPROM
  '''
  def set_client_slave_addr(self, addr):

  '''!
    @brief Get the current host-side slave address
    @return Current Modbus slave address
  '''
  def get_client_slave_addr(self):

  '''!
    @brief Set extra attempts after a transient read failure
    @param count Non-negative number of extra attempts
    @return True when accepted, otherwise False
  '''
  def set_read_retry_count(self, count):

  '''!
    @brief Get the configured number of extra read attempts
    @return Number of extra read attempts
  '''
  def get_read_retry_count(self):

  '''!
    @brief Change the Raspberry Pi UART baud rate
    @param baud New host UART baud rate
    @return True on success, otherwise False
    @note This method does not change or save the sensor baud rate
  '''
  def set_host_baudrate(self, baud):

  '''!
    @brief Convert a gas code to its display name
    @param gas_code Gas code
    @return Gas name for a known code, otherwise an empty string
  '''
  def gas_code_to_type_name(cls, gas_code):

  '''!
    @brief Get the default unit for a gas code
    @param gas_code Gas code
    @return Unit for a known code, otherwise an empty string
  '''
  def gas_code_to_default_unit(cls, gas_code):

  '''!
    @brief Format an unknown gas code as hexadecimal
    @param gas_code Gas code
    @return A string such as "0x1A"
  '''
  def gas_code_format_unknown(gas_code):

  '''!
    @brief Parse input registers starting at address 0 and update last_measure
    @param registers List of 16-bit input register values
    @return Parsed measurement dictionary
  '''
  def fill_last_measure_from_input_regs(self, registers):

  '''!
    @brief Read gas measurement input registers
    @param with_timestamp True also reads the timestamp, False reads basic measurement data only
    @return Modbus error code; 0 means success
    @note Parsed data is stored in last_measure
  '''
  def read_gas_measurement_data(self, with_timestamp=False):

  '''!
    @brief Calculate the scaled concentration from the raw value and decimal places
    @return Floating-point concentration, or NaN when decimal_point is greater than 12
  '''
  def get_concentration_float(self):

  '''!
    @brief Write one holding register on the current slave
    @param reg Holding register address
    @param value 16-bit value to write
    @return Modbus error code; 0 means success
  '''
  def write_holding_reg(self, reg, value):

  '''!
    @brief Save the current configuration to sensor EEPROM
    @return Modbus error code; 0 means success
  '''
  def commit_configuration(self):

  '''!
    @brief Change the sensor Modbus address and save it to EEPROM
    @param new_addr New address from 1 to 247
    @return Modbus error code; 0 means success
  '''
  def set_device_address(self, new_addr):

  '''!
    @brief Write the sensor baud-rate code without saving it to EEPROM
    @param code One of BAUD_CODE_2400 through BAUD_CODE_115200
    @return Modbus error code; 0 means success
  '''
  def write_device_baud_code(self, code):

  '''!
    @brief Write and save the sensor baud-rate code
    @param code One of BAUD_CODE_2400 through BAUD_CODE_115200
    @return Modbus error code; 0 means success
    @note This method does not switch the Raspberry Pi UART; see 02.change_device_baudrate.py
  '''
  def set_device_baud_code(self, code):

  '''!
    @brief Put the probe into RUN or SLEEP mode
    @param sleep True enters sleep, False enters run mode
    @return Modbus error code; 0 means success
    @note The mode takes effect immediately and is not stored in EEPROM
  '''
  def set_probe_sleep(self, sleep):

  '''!
    @brief Read the current probe operating mode
    @return Tuple (error_code, is_sleep); error_code 0 means success
  '''
  def get_probe_work_mode(self):
```

## Compatibility

| Board        | Pass | Fail | Not tested | Remarks |
| ------------ | :--: | :--: | :--------: | :-----: |
| RaspberryPi2 |      |      |     √      |         |
| RaspberryPi3 |      |      |     √      |         |
| RaspberryPi4 |  √   |      |            |         |

* Python version

| Python  | Pass | Fail | Not tested | Remarks |
| ------- | :--: | :--: | :--------: | ------- |
| Python2 |     |      |        √    |         |
| Python3 |  √   |      |            |         |

## History

- 2026/07/20 - Version 1.0.0

## Credits

Written by PLELES(li.jia@dfrobot.com), 2026. (Welcome to our [website](https://www.dfrobot.com/))
