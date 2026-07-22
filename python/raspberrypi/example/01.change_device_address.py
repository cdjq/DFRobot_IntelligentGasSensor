#!/usr/bin/python
# -*- coding: utf-8 -*-

'''!
  @file 01.change_device_address.py
  @brief Change the sensor Modbus address and verify communication at the new address.
  @details Set CURRENT_SLAVE_ADDR and NEW_SLAVE_ADDR before running this example.
  @n The new address is saved to sensor EEPROM.
  @n Wiring can use the automatic-direction RS-485 connection in example 06 or TTL UART in example 07.
  @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
  @license The MIT License (MIT)
  @author [PLELES](li.jia@dfrobot.com)
  @version V1.0.0
  @date 2026-07-20
  @url https://github.com/DFRobot/DFRobot_IntelligentGasSensor
'''

import os
import sys
import time

# Add the parent directory so this example can import the driver directly.
sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
from DFRobot_IntelligentGasSensor import DFRobot_IntelligentGasSensor


## Current UART baud rate of the sensor.
BAUD = 9600
## Address that the sensor currently responds to.
CURRENT_SLAVE_ADDR = 5
## New address to write and save; valid range is 1 to 247.
NEW_SLAVE_ADDR = 1


def print_measurement(sensor, prefix=""):
  '''Print one measurement line.'''
  error = sensor.read_gas_measurement_data(with_timestamp=True)
  if error != sensor.ERR_OK:
    print("{}read error, code={}".format(prefix, error))
    return

  measure = sensor.last_measure
  if not measure["data_valid"]:
    print("{}waiting for valid measurement...".format(prefix))
    return

  timestamp = "[{}] ".format(measure["timestamp"]) if measure["timestamp"] else ""
  print(
    "{}{}{} {:.2f} {}".format(
      prefix,
      timestamp,
      measure["gas_type"],
      sensor.get_concentration_float(),
      measure["unit"],
    )
  )


def main():
  sensor = DFRobot_IntelligentGasSensor(baud=BAUD, slave_addr=CURRENT_SLAVE_ADDR)

  # Confirm the old address before changing a persistent setting.
  print("Talking to slave {} - checking link...".format(CURRENT_SLAVE_ADDR))
  error = sensor.read_gas_measurement_data(with_timestamp=True)
  if error != sensor.ERR_OK:
    print("Cannot read at the current address. Check address, wiring and baud rate.")
    return
  if not sensor.last_measure["data_valid"]:
    print("Link is OK, but the measurement is not valid yet. Wait and retry.")
    return

  print("Read OK. Changing address...")
  # This writes the address, switches the host-side address and commits to EEPROM.
  error = sensor.set_device_address(NEW_SLAVE_ADDR)
  if error != sensor.ERR_OK:
    print("set_device_address failed, code={}".format(error))
    return

  print("Done. New slave address is {} and has been saved.".format(sensor.get_client_slave_addr()))
  # All later requests automatically use NEW_SLAVE_ADDR.
  print_measurement(sensor, "Verify read: ")
  print("Press Ctrl+C to stop.")

  try:
    while True:
      print_measurement(sensor)
      time.sleep(1)
  except KeyboardInterrupt:
    print("Stopped.")


if __name__ == "__main__":
  main()
