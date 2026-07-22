#!/usr/bin/python
# -*- coding: utf-8 -*-

'''!
  @file 03.probe_sleep_wake.py
  @brief Put the gas probe to sleep, wake it, query its mode or read one measurement.
  @details Commands: S=sleep, W=wake, P=query mode, M=read measurement, Q=quit.
  @n Probe sleep takes effect immediately and is not saved to EEPROM.
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

# Add the parent directory so this example can import the driver directly.
sys.path.append(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
from DFRobot_IntelligentGasSensor import DFRobot_IntelligentGasSensor


## Current UART baud rate and Modbus address.
BAUD = 9600
SLAVE_ADDR = 1


def read_command():
  '''Read one command from the terminal.'''
  return input("Command [S/W/P/M/Q]: ")


def print_probe_mode(sensor):
  error, is_sleep = sensor.get_probe_work_mode()
  if error != sensor.ERR_OK:
    print("get_probe_work_mode failed, code={}".format(error))
    return
  print("Probe mode: {}".format("SLEEP" if is_sleep else "RUN"))


def print_measurement(sensor):
  error = sensor.read_gas_measurement_data(with_timestamp=True)
  if error != sensor.ERR_OK:
    print("read_gas_measurement_data failed, code={}".format(error))
    return

  measure = sensor.last_measure
  if not measure["data_valid"]:
    print("Measurement is not valid. The probe may be asleep.")
    return

  timestamp = "[{}] ".format(measure["timestamp"]) if measure["timestamp"] else ""
  print(
    "{}{} {:.2f} {}".format(
      timestamp,
      measure["gas_type"],
      sensor.get_concentration_float(),
      measure["unit"],
    )
  )


def main():
  sensor = DFRobot_IntelligentGasSensor(baud=BAUD, slave_addr=SLAVE_ADDR)
  print("S=sleep  W=wake  P=probe mode  M=read gas  Q=quit")
  print_probe_mode(sensor)

  # Each command performs one operation, which makes the mode change easy to observe.
  while True:
    command = read_command().strip().lower()
    if command == "s":
      error = sensor.set_probe_sleep(True)
      print("Probe set to SLEEP." if error == sensor.ERR_OK else "Sleep command failed.")
    elif command == "w":
      error = sensor.set_probe_sleep(False)
      print("Probe set to RUN." if error == sensor.ERR_OK else "Wake command failed.")
    elif command == "p":
      print_probe_mode(sensor)
    elif command == "m":
      print_measurement(sensor)
    elif command == "q":
      print("Stopped.")
      break
    else:
      print("Use S, W, P, M or Q.")


if __name__ == "__main__":
  main()
