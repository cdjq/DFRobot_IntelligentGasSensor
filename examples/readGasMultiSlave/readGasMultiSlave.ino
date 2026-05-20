/*!
 * @file readGasMultiSlave.ino
 * @brief Poll three Modbus slaves on one RS-485 bus (shared UART + DE).
 *
 * Objects gas1 / gas2 / gas3 share HOST_SERIAL (and kDePin on RS-485), different slave IDs.
 * Switch TTL / RS-485: comment one constructor block, uncomment the other (see below).
 *
 * Default slaves: 1, 5, 3 — change to match your modules.
 * ESP32: RX=36, TX=17, DE=16.  RP2040 (SEN07xx): Serial1, DE=29.
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kBaud = 9600;

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
DFRobot_IntelligentGasSensor gas1(&HOST_SERIAL, 1, kDePin);
DFRobot_IntelligentGasSensor gas2(&HOST_SERIAL, 5, kDePin);
DFRobot_IntelligentGasSensor gas3(&HOST_SERIAL, 3, kDePin);

// TTL UART: comment the three lines above, uncomment the three below
// DFRobot_IntelligentGasSensor gas1(&HOST_SERIAL, 1);
// DFRobot_IntelligentGasSensor gas2(&HOST_SERIAL, 5);
// DFRobot_IntelligentGasSensor gas3(&HOST_SERIAL, 3);

static void printOne(DFRobot_IntelligentGasSensor &g) {
    Serial.print(F("  ID "));
    if (g.getClientSlaveAddr() < 10)
        Serial.print(' ');
    Serial.print(g.getClientSlaveAddr());
    Serial.print(F("  |  "));
    if (g.readMeasurementWithTimestamp() != 0) {
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

void setup() {
    Serial.begin(115200);
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.begin(kBaud, SERIAL_8N1, HOST_RX, HOST_TX);
#else
    HOST_SERIAL.begin(kBaud);
#endif
}

void loop() {
    Serial.println(F("----------"));
    printOne(gas1);
    printOne(gas2);
    printOne(gas3);
    Serial.println();
    delay(500);
}
