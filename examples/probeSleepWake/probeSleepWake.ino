/*!
 * @file probeSleepWake.ino
 * @brief Control the SEN07xx probe run/sleep state through Modbus holding register 0x0005.
 * @n Send one character in Serial Monitor without a newline: S=sleep, W=wake, P=query mode, M=read one measurement.
 * @n Wiring follows readGasRS485 by default or readGasUART for TTL. Switch the constructor below to select RS-485 or TTL.
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
static const uint8_t         kSlave = 1;

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX 36
#define HOST_TX 17
static const int kDePin = 16;
#else
#define HOST_SERIAL Serial1
static const int kDePin = 29;
#endif

// RS-485 by default.
DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlave, /*dePin =*/kDePin);
// TTL: comment the line above and uncomment the line below.
// DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlave);

static void printProbeMode(void) {
    bool asleep = false;
    uint8_t e = sensor.getProbeWorkMode(&asleep);
    if (e != 0) {
        Serial.print(F("getProbeWorkMode err "));
        Serial.println(e);
        return;
    }
    Serial.println(asleep ? F("probe: SLEEP") : F("probe: RUN"));
}

static void printMeasurementLine(void) {
    if (sensor.readGasMeasurementData(true) != 0) {
        Serial.println(F("readGasMeasurementData error"));
        return;
    }
    if (!sensor.lastMeasure.dataValid) {
        Serial.println(F("measurement: (not valid - probe SLEEP or no frame yet)"));
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
}

void setup() {
    Serial.begin(115200);
    delay(2000);
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, /*rx =*/HOST_RX, /*tx =*/HOST_TX);
#else
    HOST_SERIAL.begin(kBaud);
#endif
    Serial.println(F("S=sleep  W=wake  P=probe mode  M=read gas"));
    printProbeMode();
}

void loop() {
    if (!Serial.available())
        return;
    const char c = (char)Serial.read();
    while (Serial.available())
        (void)Serial.read();

    switch (c) {
    case 'S':
    case 's':
        if (sensor.setProbeSleep(true) != 0)
            Serial.println(F("setProbeSleep(SLEEP) failed"));
        else
            Serial.println(F("setProbeSleep: SLEEP"));
        break;
    case 'W':
    case 'w':
        if (sensor.setProbeSleep(false) != 0)
            Serial.println(F("setProbeSleep(RUN) failed"));
        else
            Serial.println(F("setProbeSleep: RUN"));
        break;
    case 'P':
    case 'p':
        printProbeMode();
        break;
    case 'M':
    case 'm':
        printMeasurementLine();
        break;
    default:
        Serial.println(F("use S W P M"));
        break;
    }
}
