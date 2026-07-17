/*!
 * @file readGasMultiSlave.ino
 * @brief Poll multiple Modbus slaves on one RS-485 bus with three library objects sharing UART and DE.
 * @n gas1, gas2, and gas3 share HOST_SERIAL and kDePin but use different slave addresses. Defaults are 1, 5, and 3.
 * @n Switch between TTL and RS-485 by commenting or uncommenting the corresponding constructor block below.
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

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX     36
#define HOST_TX     17
static const int kDePin = 16;
#else
#define HOST_SERIAL Serial1
static const int kDePin = 29;
#endif

// RS-485 by default.
DFRobot_IntelligentGasSensor gas1(/*s =*/&HOST_SERIAL, /*slaveAddr =*/1, /*dePin =*/kDePin);
DFRobot_IntelligentGasSensor gas2(/*s =*/&HOST_SERIAL, /*slaveAddr =*/5, /*dePin =*/kDePin);
DFRobot_IntelligentGasSensor gas3(/*s =*/&HOST_SERIAL, /*slaveAddr =*/3, /*dePin =*/kDePin);

// TTL: comment the three lines above and uncomment the three lines below.
// DFRobot_IntelligentGasSensor gas1(/*s =*/&HOST_SERIAL, /*slaveAddr =*/1);
// DFRobot_IntelligentGasSensor gas2(/*s =*/&HOST_SERIAL, /*slaveAddr =*/5);
// DFRobot_IntelligentGasSensor gas3(/*s =*/&HOST_SERIAL, /*slaveAddr =*/3);

static void printOne(DFRobot_IntelligentGasSensor &g)
{
  Serial.print(F("  ID "));
  if (g.getClientSlaveAddr() < 10)
    Serial.print(' ');
  Serial.print(g.getClientSlaveAddr());
  Serial.print(F("  |  "));
  if (g.readGasMeasurementData(true) != 0) {
    Serial.println(F("read fail"));
    return;
  }
  if (!g.lastMeasure.dataValid) {
    Serial.println(F("  (no valid data yet)"));
    return;
  }
  if (g.lastMeasure.timestamp[0]) {
    Serial.print('[');
    Serial.print(g.lastMeasure.timestamp);
    Serial.print(F("]  "));
  }
  Serial.print(g.lastMeasure.gasType);
  Serial.print(F("  "));
  Serial.print(g.getConcentrationFloat(), 2);
  Serial.print(F("  "));
  Serial.println(g.lastMeasure.unit);
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
  printOne(gas1);
  printOne(gas2);
  printOne(gas3);
  Serial.println();
  delay(500);
}
