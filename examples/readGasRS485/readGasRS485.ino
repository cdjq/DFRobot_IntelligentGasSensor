/*!
 * @file readGasRS485.ino
 * @brief Poll SEN07xx intelligent gas sensor concentration through RS-485 with a DE control pin.
 * @n ESP32 connects to the sensor through a UART-to-RS485 module. The sensor exposes only A/B terminals; default bus settings are 9600 8N1 and slave address 1.
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
 *
 * @copyright   Copyright (c) 2026 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @author [wxzed](xiao.wu@dfrobot.com)
 * @version  V1.0.0
 * @date  2026-05-21
 * @url https://github.com/DFRobot/DFRobot_IntelligentGasSensor
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kBaud  = 9600;
static const uint8_t       kSlave = 1;

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX     36
#define HOST_TX     17
static const int kDePin = 16;
#else
#define HOST_SERIAL Serial1
static const int kDePin = 29;
#endif

DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlave, /*dePin =*/kDePin);

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
  const uint8_t err = sensor.readGasMeasurementData(true);
  if (err != 0) {
    Serial.print(F("read error, code="));
    Serial.println(err);
    delay(1000);
    return;
  }
  if (!sensor.lastMeasure.dataValid) {
    Serial.println(F("waiting for valid measurement..."));
    delay(1000);
    return;
  }
  if (sensor.lastMeasure.timestamp[0]) {
    Serial.print('[');
    Serial.print(sensor.lastMeasure.timestamp);
    Serial.print(F("] "));
  }
  Serial.print(sensor.lastMeasure.gasType);
  Serial.print(' ');
  Serial.print(sensor.getConcentrationFloat(), 2);
  Serial.print(' ');
  Serial.println(sensor.lastMeasure.unit);
  delay(1000);
}
