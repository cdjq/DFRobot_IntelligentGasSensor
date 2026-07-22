#!/usr/bin/python
# -*- coding: utf-8 -*-

'''!
  @file 02.change_device_baudrate.py
  @brief Change the sensor Modbus baud rate.
  @details The correct order is: write sensor baud code, change host baud, then commit.
  @n Set SLAVE_ADDR and INITIAL_BAUD to the sensor's current settings before running.
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


## Baud rate currently used by the sensor.
INITIAL_BAUD = 9600
## New baud rate. The matching sensor baud code is selected automatically.
TARGET_BAUD = 19200
## Modbus slave address of the sensor.
SLAVE_ADDR = 1

## Convert the human-readable baud rate to the value written to register 0x0003.
BAUD_CODE_BY_RATE = {
  2400: DFRobot_IntelligentGasSensor.BAUD_CODE_2400,
  4800: DFRobot_IntelligentGasSensor.BAUD_CODE_4800,
  9600: DFRobot_IntelligentGasSensor.BAUD_CODE_9600,
  14400: DFRobot_IntelligentGasSensor.BAUD_CODE_14400,
  19200: DFRobot_IntelligentGasSensor.BAUD_CODE_19200,
  38400: DFRobot_IntelligentGasSensor.BAUD_CODE_38400,
  57600: DFRobot_IntelligentGasSensor.BAUD_CODE_57600,
  115200: DFRobot_IntelligentGasSensor.BAUD_CODE_115200,
}

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
  target_baud_code = BAUD_CODE_BY_RATE.get(TARGET_BAUD)
  if target_baud_code is None:
    print(
      "Unsupported TARGET_BAUD {}. Choose one of: {}".format(
        TARGET_BAUD,
        ", ".join(str(baud) for baud in sorted(BAUD_CODE_BY_RATE)),
      )
    )
    return

  sensor = DFRobot_IntelligentGasSensor(baud=INITIAL_BAUD, slave_addr=SLAVE_ADDR)

  # Confirm the initial communication settings before changing them.
  print("Checking Modbus link at {} baud...".format(INITIAL_BAUD))
  error = sensor.read_gas_measurement_data(with_timestamp=True)
  if error != sensor.ERR_OK:
    print("Cannot read. Check slave address, wiring and initial baud rate.")
    return
  print_measurement(sensor, "Before change: ")

  print("Changing sensor and host to {} baud...".format(TARGET_BAUD))
  # Do not commit yet: the sensor changes its active baud immediately after this write.
  error = sensor.write_device_baud_code(target_baud_code)
  if error != sensor.ERR_OK:
    print("write_device_baud_code failed, code={}".format(error))
    return

  # The sensor switches baud immediately after acknowledging the register write.
  time.sleep(0.02)
  if not sensor.set_host_baudrate(TARGET_BAUD):
    print("Cannot change the Raspberry Pi UART baud rate.")
    return

  # The host and sensor now use the same baud, so the commit request can be received.
  error = sensor.commit_configuration()
  if error != sensor.ERR_OK:
    print("commit_configuration failed, code={}".format(error))
    return

  print("Baud rate changed and saved. Press Ctrl+C to stop.")
  print_measurement(sensor, "After change: ")
  try:
    while True:
      print_measurement(sensor)
      time.sleep(1)
  except KeyboardInterrupt:
    print("Stopped.")


if __name__ == "__main__":
  main()
