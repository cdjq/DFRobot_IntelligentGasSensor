/*!
 * @file readGasMultiSlaveOneInstance.ino
 * @brief Poll multiple Modbus slaves on one bus with one library object by switching the target slave address.
 * @n Unlike readGasMultiSlave, this example uses one object and calls setClientSlaveAddr() each cycle.
 * @n Switch between TTL and RS-485 the same way as readGasMultiSlave. Default slave list is 1, 5, and 3.
 * @n note: The sensor supports RS-485 through an adapter or direct TTL UART wiring. Use 5 V for sensor VCC when wiring directly.
 * @n connected table (ESP32 + UART-to-RS485 module + sensor A/B)
 * ---------------------------------------------------------------------------------------------------------------
 * ESP32 pin | UART-to-RS485 module | Sensor (SEN07xx) |
 *    3.3V   |          VCC         |        --        |
 *    GND    |          GND         |        --        |
 * GPIO17(TX)|          DI          |        --        |
 * GPIO36(RX)|          RO          |        --        |
 * GPIO16    |         DE/RE        |        --        |
 *     --    |           A          |        A         |
 *     --    |           B          |        B         |
 * ---------------------------------------------------------------------------------------------------------------
 * @n connected table (ESP32 direct TTL UART + sensor)
 * ---------------------------------------------------
 * ESP32 pin | Sensor (SEN07xx) |
 *    5V     |       VCC        |
 *    GND    |       GND        |
 * GPIO17(TX)|        RX        |
 * GPIO36(RX)|        TX        |
 * ---------------------------------------------------
 *
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @author [wxzed](xiao.wu@dfrobot.com)
 * @version  V1.0.0
 * @date  2026-05-21
 * @url https://github.com/DFRobot/DFRobot_IntelligentGasSensor
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kBaud = 9600;

// Slave addresses on the bus. Adjust them to match the actual modules.
static const uint8_t kSlaveIds[] = { 1, 5, 3 };
static const size_t  kSlaveCount = sizeof(kSlaveIds) / sizeof(kSlaveIds[0]);

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX     36
#define HOST_TX     17
static const int kDePin = 16;
#else
#define HOST_SERIAL Serial1
static const int kDePin = 29;
#endif

// RS-485: initial address should be kSlaveIds[0].
DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlaveIds[0], /*dePin =*/kDePin);
// TTL: comment the line above and uncomment the line below.
// DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlaveIds[0]);

static void readAndPrint(void)
{
  uint8_t id = sensor.getClientSlaveAddr();
  Serial.print(F("  ID "));
  if (id < 10)
    Serial.print(' ');
  Serial.print(id);
  Serial.print(F("  |  "));
  if (sensor.readGasMeasurementData(true) != 0) {
    Serial.println(F("read fail"));
    return;
  }
  if (!sensor.lastMeasure.dataValid) {
    Serial.println(F("  (no valid data yet)"));
    return;
  }
  if (sensor.lastMeasure.timestamp[0]) {
    Serial.print('[');
    Serial.print(sensor.lastMeasure.timestamp);
    Serial.print(F("]  "));
  }
  Serial.print(sensor.lastMeasure.gasType);
  Serial.print(F("  "));
  Serial.print(sensor.getConcentrationFloat(), 2);
  Serial.print(F("  "));
  Serial.println(sensor.lastMeasure.unit);
}

void setup()
{
  Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32)
  HOST_SERIAL.begin(kBaud, SERIAL_8N1, /*rx =*/HOST_RX, /*tx =*/HOST_TX);
#else
  HOST_SERIAL.begin(kBaud);
#endif
}

void loop()
{
  Serial.println(F("----------"));
  for (size_t i = 0; i < kSlaveCount; i++) {
    sensor.setClientSlaveAddr(kSlaveIds[i]);
    readAndPrint();
  }
  Serial.println();
  delay(500);
}
