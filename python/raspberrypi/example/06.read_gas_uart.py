#!/usr/bin/python
# -*- coding: utf-8 -*-

'''!
  @file 07.read_gas_uart.py
  @brief Read gas concentration through the Raspberry Pi TTL UART.
  @details Cross-connect Raspberry Pi TX/RX with sensor RX/TX and connect common GND.
  @n The default serial settings are 9600 baud, 8 data bits, no parity and 1 stop bit.
  @n Raspberry Pi BCM14(TX) connects to sensor RX and BCM15(RX) connects to sensor TX.
  @n The sensor UART uses 3.3 V logic. Connect sensor VCC to 5 V and share GND.
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


## Change these values when the sensor does not use its factory settings.
BAUD = 9600
SLAVE_ADDR = 1


def print_measurement(sensor):
  error = sensor.read_gas_measurement_data(with_timestamp=True)
  if error != sensor.ERR_OK:
    print("Read error, code={}".format(error))
    return

  measure = sensor.last_measure
  if not measure["data_valid"]:
    print("Waiting for a valid measurement...")
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
  # DFRobot_RTU opens Raspberry Pi UART /dev/ttyAMA0.
  sensor = DFRobot_IntelligentGasSensor(baud=BAUD, slave_addr=SLAVE_ADDR)
  print("Reading TTL UART sensor. Press Ctrl+C to stop.")
  try:
    while True:
      print_measurement(sensor)
      time.sleep(1)
  except KeyboardInterrupt:
    print("Stopped.")


if __name__ == "__main__":
  main()
