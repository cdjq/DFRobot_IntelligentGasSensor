/*!
 * @file probeSleepWake.ino
 * @brief Control SEN07xx SMX100 probe RUN/SLEEP over Modbus holding reg 0x0005 (no EEPROM).
 *
 * Serial Monitor: send a single character (no line ending needed):
 *   S — sleep probe (no new gas frames until wake)
 *   W — wake probe
 *   P — query probe mode (read holding 0x0005)
 *   M — read one measurement (may be invalid while SLEEP)
 *
 * Wiring: same as readGasRS485 (default) or readGasUART — toggle constructor block below.
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

// RS-485 (default)
DFRobot_IntelligentGasSensor sensor(&HOST_SERIAL, kSlave, kDePin);
// TTL: comment line above, uncomment below
// DFRobot_IntelligentGasSensor sensor(&HOST_SERIAL, kSlave);

static void printProbeMode(void) {
    bool asleep = false;
    uint8_t e = sensor.readProbeSleepMode(&asleep);
    if (e != 0) {
        Serial.print(F("readProbeSleepMode err "));
        Serial.println(e);
        return;
    }
    Serial.println(asleep ? F("probe: SLEEP") : F("probe: RUN"));
}

static void printMeasurementLine(void) {
    if (sensor.readMeasurementWithTimestamp() != 0) {
        Serial.println(F("readMeasurement error"));
        return;
    }
    if (!sensor.lastMeasure.dataValid) {
        Serial.println(F("measurement: (not valid — probe SLEEP or no frame yet)"));
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
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, HOST_RX, HOST_TX);
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
