#!/usr/bin/python
# -*- coding: utf-8 -*-

'''!
  @file DFRobot_IntelligentGasSensor.py
  @brief Modbus RTU driver for DFRobot intelligent gas sensors (SEN07xx).
  @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
  @license The MIT License (MIT)
  @author [PLELES](li.jia@dfrobot.com)
  @version V1.0.0
  @date 2026-07-20
  @url https://github.com/DFRobot/DFRobot_IntelligentGasSensor
'''

import time

from DFRobot_RTU import DFRobot_RTU


class DFRobot_IntelligentGasSensor(DFRobot_RTU):
  '''!
    @brief DFRobot intelligent gas sensor driver.
    @details The sensor communicates through UART with the Modbus RTU protocol.
  '''

  ## DFRobot vendor ID.
  VID_DFROBOT = 0x3343
  ## Supported product IDs.
  PID_SEN0742 = 0xC742
  PID_SEN0746 = 0xC746
  ## Register map version used by SEN07xx sensors.
  REGISTER_MAP_VERSION = 0x0100

  ## Modbus return codes from DFRobot_RTU.
  ERR_OK = 0x00
  ERR_ILLEGAL_FUNCTION = 0x01
  ERR_ILLEGAL_DATA_ADDRESS = 0x02
  ERR_ILLEGAL_DATA_VALUE = 0x03
  ERR_SLAVE_FAILURE = 0x04
  ERR_CRC = 0x08
  ERR_RECV = 0x09
  ERR_MEMORY = 0x0A
  ERR_ID = 0x0B

  ## Input register addresses (Modbus function code 0x04).
  _REG_INPUT_PID = 0x0000
  _REG_INPUT_VID = 0x0001
  _REG_INPUT_MODBUS_ADDR = 0x0002
  _REG_INPUT_RESERVED0 = 0x0003
  _REG_INPUT_RESERVED1 = 0x0004
  _REG_INPUT_VERSION = 0x0005
  _REG_INPUT_RESERVED2 = 0x0006
  _REG_INPUT_STATUS = 0x0007
  _REG_INPUT_GAS_CODE = 0x0008
  _REG_INPUT_CONCENTRATION_LOW = 0x0009
  _REG_INPUT_CONCENTRATION_HIGH = 0x000A
  _REG_INPUT_DECIMAL_POINT = 0x000B
  _REG_INPUT_TIMESTAMP_YEAR = 0x000C
  _REG_INPUT_TIMESTAMP_MONTH = 0x000D
  _REG_INPUT_TIMESTAMP_DAY = 0x000E
  _REG_INPUT_TIMESTAMP_HOUR = 0x000F
  _REG_INPUT_TIMESTAMP_MINUTE = 0x0010
  _REG_INPUT_TIMESTAMP_SECOND = 0x0011
  _INPUT_REGISTER_COUNT = 18
  _INPUT_REGISTER_COUNT_NO_TIMESTAMP = 12

  ## Holding register addresses (Modbus function code 0x03/0x06).
  _REG_HOLDING_RESERVED0 = 0x0000
  _REG_HOLDING_RESERVED1 = 0x0001
  _REG_HOLDING_RESERVED2 = 0x0002
  _REG_HOLDING_BAUD_CODE = 0x0003
  _REG_HOLDING_PARITY_STOP = 0x0004
  _REG_HOLDING_RESERVED3 = 0x0005
  _REG_HOLDING_PROBE_SLEEP = _REG_HOLDING_RESERVED3
  _REG_HOLDING_SLAVE_ADDR = 0x0006
  _REG_HOLDING_COMMIT = 0x0007
  _HOLDING_REGISTER_COUNT = 8

  ## Measurement status bit: current gas data is valid.
  _STATUS_VALID_MASK = 1 << 0
  ## Modbus slave address range.
  _SLAVE_ADDR_MIN = 1
  _SLAVE_ADDR_MAX = 247
  ## Value written to the commit register to save configuration.
  _COMMIT_KEY_SAVE_CONFIG = 0xA501

  ## Probe working modes.
  PROBE_RUN = 0
  PROBE_SLEEP = 1

  ## Baud-rate codes written to holding register 0x0003.
  BAUD_CODE_2400 = 1
  BAUD_CODE_4800 = 2
  BAUD_CODE_9600 = 3
  BAUD_CODE_14400 = 4
  BAUD_CODE_19200 = 5
  BAUD_CODE_38400 = 6
  BAUD_CODE_57600 = 7
  BAUD_CODE_115200 = 8
  _BAUD_CODE_MIN = BAUD_CODE_2400
  _BAUD_CODE_MAX = BAUD_CODE_115200

  ## Default communication settings.
  _DEFAULT_TIMEOUT_S = 0.1
  _DEFAULT_READ_RETRIES = 3
  _READ_RETRY_DELAY_S = 0.1

  ## Gas code, display name and default unit table.
  _GAS_TABLE = {
    0x01: ("CH4", "%LEL"),
    0x03: ("H2S", "ppm"),
    0x04: ("CO", "ppm"),
    0x05: ("O2", "%VOL"),
  }

  def __init__(
    self,
    baud=9600,
    slave_addr=1,
    bits=8,
    parity="N",
    stopbit=1,
    shared_rtu=None,
  ):
    '''!
      @brief Initialize UART and create the gas sensor object.
      @param baud UART baud rate, default is 9600.
      @param slave_addr Modbus slave address, range 1 to 247.
      @param bits UART data bits, default is 8.
      @param parity UART parity, default is "N" (none).
      @param stopbit UART stop bits, default is 1.
      @param shared_rtu Another DFRobot_RTU object whose serial port will be shared.
    '''
    if not self._is_valid_slave_addr(slave_addr):
      raise ValueError("slave_addr must be in range 1 to 247")

    if shared_rtu is None:
      DFRobot_RTU.__init__(self, baud, bits, parity, stopbit)
    else:
      # Multiple Modbus slave objects can safely use one physical UART.
      self._ser = shared_rtu._ser
      self._timeout = self._DEFAULT_TIMEOUT_S
    self._slave = slave_addr
    self._read_retries = self._DEFAULT_READ_RETRIES
    self.last_measure = self._new_measure()
    # Sensor flash operations may take close to one second.
    self.set_timout_time_s(self._DEFAULT_TIMEOUT_S)

  @staticmethod
  def _new_measure():
    '''Create an empty measurement dictionary.'''
    return {
      "pid": 0,
      "vid": 0,
      "modbus_addr": 0,
      "register_map_version": 0,
      "status": 0,
      "gas_code": 0,
      "concentration_raw": 0,
      "decimal_point": 0,
      "gas_type": "",
      "unit": "",
      "timestamp_year": 0,
      "timestamp_month": 0,
      "timestamp_day": 0,
      "timestamp_hour": 0,
      "timestamp_minute": 0,
      "timestamp_second": 0,
      "timestamp": "",
      "data_valid": False,
    }

  @classmethod
  def _is_valid_slave_addr(cls, addr):
    # bool is a subclass of int in Python, but True/False are not valid addresses.
    return (
      isinstance(addr, int)
      and not isinstance(addr, bool)
      and cls._SLAVE_ADDR_MIN <= addr <= cls._SLAVE_ADDR_MAX
    )

  def set_client_slave_addr(self, addr):
    '''!
      @brief Change the slave address used by later host requests.
      @param addr Modbus slave address, range 1 to 247.
      @return True when accepted, otherwise False.
      @note This function only changes the host-side address and does not write the sensor.
    '''
    if not self._is_valid_slave_addr(addr):
      return False
    self._slave = addr
    return True

  def get_client_slave_addr(self):
    '''!
      @brief Get the slave address used by this object.
      @return Current Modbus slave address.
    '''
    return self._slave

  def set_read_retry_count(self, count):
    '''!
      @brief Set extra attempts after a transient read error.
      @param count Additional attempts after the first request.
      @return True when accepted, otherwise False.
    '''
    if not isinstance(count, int) or count < 0:
      return False
    self._read_retries = count
    return True

  def get_read_retry_count(self):
    '''!
      @brief Get the number of configured extra read attempts.
      @return Extra read attempt count.
    '''
    return self._read_retries

  def set_host_baudrate(self, baud):
    '''!
      @brief Change the baud rate of the Raspberry Pi UART.
      @param baud New host UART baud rate.
      @return True when changed, otherwise False.
      @note This function does not change or save the sensor baud rate.
    '''
    if not isinstance(baud, int) or not 1 <= baud <= 115200:
      return False
    try:
      self._ser.baudrate = baud
      return True
    except (AttributeError, ValueError):
      return False

  @classmethod
  def gas_code_to_type_name(cls, gas_code):
    '''!
      @brief Convert a gas code to its display name.
      @return Gas name, or an empty string when the code is unknown.
    '''
    row = cls._GAS_TABLE.get(gas_code)
    return row[0] if row else ""

  @classmethod
  def gas_code_to_default_unit(cls, gas_code):
    '''!
      @brief Convert a gas code to its default concentration unit.
      @return Unit string, or an empty string when the code is unknown.
    '''
    row = cls._GAS_TABLE.get(gas_code)
    return row[1] if row else ""

  @staticmethod
  def gas_code_format_unknown(gas_code):
    '''!
      @brief Format an unknown gas code as a hexadecimal string.
      @return String such as "0x1A".
    '''
    return "0x%02X" % (gas_code & 0xFF)

  def _read_registers(self, start_reg, register_count, input_register=True):
    '''Read Modbus bytes and convert every two bytes to one 16-bit register.'''
    if input_register:
      result = self.read_input_registers(self._slave, start_reg, register_count)
    else:
      result = self.read_holding_registers(self._slave, start_reg, register_count)

    if not result:
      return (self.ERR_RECV, [])
    error = result[0]
    if error != self.ERR_OK:
      return (error, [])

    data = result[1:]
    if len(data) < register_count * 2:
      return (self.ERR_RECV, [])

    registers = []
    for index in range(register_count):
      high_byte = data[index * 2]
      low_byte = data[index * 2 + 1]
      registers.append(((high_byte << 8) | low_byte) & 0xFFFF)
    return (self.ERR_OK, registers)

  def fill_last_measure_from_input_regs(self, registers):
    '''!
      @brief Parse the input register table and update last_measure.
      @param registers List of 16-bit input register values starting at address 0.
      @return Parsed measurement dictionary.
    ''' 
    measure = self._new_measure()   
    if registers is None or len(registers) < 6:
      self.last_measure = measure
      return measure

    measure["pid"] = registers[self._REG_INPUT_PID]
    measure["vid"] = registers[self._REG_INPUT_VID]
    measure["modbus_addr"] = registers[self._REG_INPUT_MODBUS_ADDR] & 0xFF
    measure["register_map_version"] = registers[self._REG_INPUT_VERSION]

    if len(registers) >= self._INPUT_REGISTER_COUNT_NO_TIMESTAMP:
      measure["status"] = registers[self._REG_INPUT_STATUS]
      measure["gas_code"] = registers[self._REG_INPUT_GAS_CODE] & 0xFF
      measure["concentration_raw"] = (
        registers[self._REG_INPUT_CONCENTRATION_LOW]
        | (registers[self._REG_INPUT_CONCENTRATION_HIGH] << 16)
      )
      measure["decimal_point"] = registers[self._REG_INPUT_DECIMAL_POINT] & 0xFF
      self._apply_gas_to_measure(measure)

    if len(registers) >= self._INPUT_REGISTER_COUNT:
      self._apply_timestamp_to_measure(measure, registers)

    measure["data_valid"] = bool(measure["status"] & self._STATUS_VALID_MASK)
    self.last_measure = measure
    return measure

  def _apply_gas_to_measure(self, measure):
    gas_code = measure["gas_code"]
    gas_type = self.gas_code_to_type_name(gas_code)
    if gas_type:
      measure["gas_type"] = gas_type
      measure["unit"] = self.gas_code_to_default_unit(gas_code) or "?"
    else:
      measure["gas_type"] = self.gas_code_format_unknown(gas_code)
      measure["unit"] = "?"

  def _apply_timestamp_to_measure(self, measure, registers):
    year = registers[self._REG_INPUT_TIMESTAMP_YEAR]
    month = registers[self._REG_INPUT_TIMESTAMP_MONTH] & 0xFF
    day = registers[self._REG_INPUT_TIMESTAMP_DAY] & 0xFF
    hour = registers[self._REG_INPUT_TIMESTAMP_HOUR] & 0xFF
    minute = registers[self._REG_INPUT_TIMESTAMP_MINUTE] & 0xFF
    second = registers[self._REG_INPUT_TIMESTAMP_SECOND] & 0xFF

    measure["timestamp_year"] = year
    measure["timestamp_month"] = month
    measure["timestamp_day"] = day
    measure["timestamp_hour"] = hour
    measure["timestamp_minute"] = minute
    measure["timestamp_second"] = second

    all_zero = year == 0 and month == 0 and day == 0 and hour == 0 and minute == 0 and second == 0
    valid = (
      2000 <= year <= 2099
      and 1 <= month <= 12
      and 1 <= day <= 31
      and 0 <= hour <= 23
      and 0 <= minute <= 59
      and 0 <= second <= 59
    )
    if not all_zero and valid:
      measure["timestamp"] = "%04d-%02d-%02d %02d:%02d:%02d" % (
        year,
        month,
        day,
        hour,
        minute,
        second,
      )

  def read_gas_measurement_data(self, with_timestamp=False):
    '''!
      @brief Read gas measurement input registers.
      @param with_timestamp True reads timestamp registers, False reads measurement only.
      @return Modbus exception code; zero means success.
      @note Parsed data is stored in last_measure.
    '''
    if not isinstance(with_timestamp, bool):
      return self.ERR_ILLEGAL_DATA_VALUE

    register_count = (
      self._INPUT_REGISTER_COUNT if with_timestamp else self._INPUT_REGISTER_COUNT_NO_TIMESTAMP
    )
    error = self.ERR_RECV
    registers = []

    for attempt in range(self._read_retries + 1):
      error, registers = self._read_registers(0, register_count, input_register=True)
      if error == self.ERR_OK:
        break
      if error not in (self.ERR_CRC, self.ERR_RECV):
        break
      if attempt < self._read_retries:
        time.sleep(self._READ_RETRY_DELAY_S)

    if error == self.ERR_OK:
      self.fill_last_measure_from_input_regs(registers)
    return error

  def get_concentration_float(self):
    '''!
      @brief Convert the raw concentration using its decimal point value.
      @return Concentration as float, or NaN when decimal_point is greater than 12.
    '''
    decimal_point = self.last_measure["decimal_point"]
    if decimal_point > 12:
      return float("nan")
    return float(self.last_measure["concentration_raw"]) / (10 ** decimal_point)

  def write_holding_reg(self, reg, value):
    '''!
      @brief Write one holding register on the current slave.
      @return Modbus exception code; zero means success.
    '''
    return self.write_holding_register(self._slave, reg, value)

  def commit_configuration(self):
    '''!
      @brief Save the current sensor configuration to EEPROM.
      @return Modbus exception code; zero means success.
    '''
    return self.write_holding_reg(self._REG_HOLDING_COMMIT, self._COMMIT_KEY_SAVE_CONFIG)

  def set_device_address(self, new_addr):
    '''!
      @brief Change and save the sensor Modbus slave address.
      @param new_addr New slave address, range 1 to 247.
      @return Modbus exception code; zero means success.
    '''
    if not self._is_valid_slave_addr(new_addr):
      return self.ERR_ILLEGAL_DATA_VALUE
    if new_addr == self._slave:
      return self.ERR_OK

    error = self.write_holding_reg(self._REG_HOLDING_SLAVE_ADDR, new_addr)
    if error != self.ERR_OK:
      return error

    # The sensor uses the new address immediately, so COMMIT must use it too.
    self._slave = new_addr
    return self.commit_configuration()

  def write_device_baud_code(self, code):
    '''!
      @brief Write a baud-rate code without saving it to EEPROM.
      @param code One of BAUD_CODE_*.
      @return Modbus exception code; zero means success.
    '''
    if (
      not isinstance(code, int)
      or isinstance(code, bool)
      or not self._BAUD_CODE_MIN <= code <= self._BAUD_CODE_MAX
    ):
      return self.ERR_ILLEGAL_DATA_VALUE
    return self.write_holding_reg(self._REG_HOLDING_BAUD_CODE, code)

  def set_device_baud_code(self, code):
    '''!
      @brief Write and save a baud-rate code.
      @return Modbus exception code; zero means success.
      @note The host UART baud rate is not changed by this function.
    '''
    error = self.write_device_baud_code(code)
    if error != self.ERR_OK:
      return error
    return self.commit_configuration()

  def set_probe_sleep(self, sleep):
    '''!
      @brief Put the probe into sleep mode or wake it immediately.
      @param sleep True enters sleep mode, False enters run mode.
      @return Modbus exception code; zero means success.
    '''
    if not isinstance(sleep, bool):
      return self.ERR_ILLEGAL_DATA_VALUE
    value = self.PROBE_SLEEP if sleep else self.PROBE_RUN
    return self.write_holding_reg(self._REG_HOLDING_PROBE_SLEEP, value)

  def get_probe_work_mode(self):
    '''!
      @brief Read the probe work mode.
      @return Tuple of (error_code, is_sleep); zero error_code means success.
    '''
    error, registers = self._read_registers(
      self._REG_HOLDING_PROBE_SLEEP,
      1,
      input_register=False,
    )
    if error != self.ERR_OK:
      return (error, False)
    return (self.ERR_OK, registers[0] != self.PROBE_RUN)
