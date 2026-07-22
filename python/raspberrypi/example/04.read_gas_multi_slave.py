#!/usr/bin/python
# -*- coding: utf-8 -*-

'''!
  @file 04.read_gas_multi_slave.py
  @brief Poll three Modbus gas sensors with separate objects sharing one UART.
  @details Change SLAVE_IDS to match the devices connected to the RS-485 bus.
  @n Use an automatic-direction UART-to-RS485 converter with the Python RTU library.
  @copyright Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
  @license The MIT License (MIT)
  @author [wxzed](xiao.wu@dfrobot.com)
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


## All slaves on one RS-485 bus must use the same UART settings.
BAUD = 9600
## Change these addresses to match the sensors connected to the bus.
SLAVE_IDS = [1, 2, 3]


def print_one(sensor):
  slave_id = sensor.get_client_slave_addr()
  error = sensor.read_gas_measurement_data(with_timestamp=True)
  if error != sensor.ERR_OK:
    print("  Addr {:3d}  |  read failed, code={}".format(slave_id, error))
    return

  measure = sensor.last_measure
  if not measure["data_valid"]:
    print("  Addr {:3d}  |  no valid data yet".format(slave_id))
    return

  timestamp = "[{}] ".format(measure["timestamp"]) if measure["timestamp"] else ""
  print(
    "  Addr {:3d}  |  {}{} {:.2f} {}".format(
      slave_id,
      timestamp,
      measure["gas_type"],
      sensor.get_concentration_float(),
      measure["unit"],
    )
  )


def main():
  # The first object opens the UART. Other objects reuse the same physical port.
  first_sensor = DFRobot_IntelligentGasSensor(baud=BAUD, slave_addr=SLAVE_IDS[0])
  sensors = [first_sensor]
  for slave_id in SLAVE_IDS[1:]:
    # shared_rtu prevents the same /dev/ttyAMA0 port from being opened repeatedly.
    sensors.append(
      DFRobot_IntelligentGasSensor(
        baud=BAUD,
        slave_addr=slave_id,
        shared_rtu=first_sensor,
      )
    )

  print("Polling slave addresses {}. Press Ctrl+C to stop.".format(SLAVE_IDS))
  try:
    while True:
      print("----------")
      # Modbus is request-response, so poll one slave at a time.
      for sensor in sensors:
        print_one(sensor)
      print("")
      time.sleep(0.5)
  except KeyboardInterrupt:
    print("Stopped.")


if __name__ == "__main__":
  main()
