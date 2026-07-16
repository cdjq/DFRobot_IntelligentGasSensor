/*!
 * @file changeDeviceBaudrate.ino
 * @brief Change the sensor Modbus baud rate from the factory 9600 setting to 19200.
 * @n SEN07xx firmware switches the slave UART to the new baud rate immediately after replying to the holding-register 0x0003 write.
 * @n Correct order: writeDeviceBaudCode -> immediately call HOST_SERIAL.begin(new baud) -> commitConfiguration().
 * @n A successful COMMIT means the active line speed and EEPROM setting now match. The loop then keeps polling at 19200.
 * @n Before uploading, confirm that kSlaveAddr and the sensor's current baud rate match kInitialBaud.
 * @n note: The sensor exposes only RS-485 A/B terminals. Connect ESP32 TX/RX/DE to a UART-to-RS485 module, then connect the module A/B pins to the sensor.
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

static const unsigned long kInitialBaud = 9600;

// Target line speed. Keep this in sync with DFROBOT_IGS_BAUD_CODE_*.
static const unsigned long kTargetUartBaud = 19200;
static const uint16_t     kTargetBaudCode  = DFROBOT_IGS_BAUD_CODE_19200;

static const uint8_t kSlaveAddr = 1;

#if defined(ARDUINO_ARCH_ESP32)
#define HOST_SERIAL Serial2
#define HOST_RX 36
#define HOST_TX 17
static const int kDePin = 16;
#else
#define HOST_SERIAL Serial1
static const int kDePin = 29;
#endif

DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlaveAddr, /*dePin =*/kDePin);
// TTL: DFRobot_IntelligentGasSensor sensor(/*s =*/&HOST_SERIAL, /*slaveAddr =*/kSlaveAddr);

static void hostSerialBegin(unsigned long baud) {
#if defined(ARDUINO_ARCH_ESP32)
    HOST_SERIAL.end();
    HOST_SERIAL.begin(baud, SERIAL_8N1, /*rx =*/HOST_RX, /*tx =*/HOST_TX);
#else
    HOST_SERIAL.end();
    HOST_SERIAL.begin(baud);
#endif
}

// writeDeviceBaudCode -> HOST_SERIAL.begin(lineBaud) -> commitConfiguration()
static uint8_t writeBaudCodeReopenUartThenCommit(uint16_t code, unsigned long lineBaud) {
    uint8_t e = sensor.writeDeviceBaudCode(code);
    if (e != 0)
        return e;
    delay(20);
    hostSerialBegin(lineBaud);
    return sensor.commitConfiguration();
}

static void printOneLine(const char *tag) {
    if (sensor.readGasMeasurementData(true) != 0) {
        if (tag)
            Serial.print(tag);
        Serial.println(F("read error"));
        return;
    }
    if (!sensor.lastMeasure.dataValid) {
        if (tag)
            Serial.print(tag);
        Serial.println(F("waiting for valid data..."));
        return;
    }
    if (tag)
        Serial.print(tag);
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
    hostSerialBegin(kInitialBaud);

    Serial.print(F("Modbus link at "));
    Serial.print(kInitialBaud);
    Serial.println(F(" - checking..."));

    if (sensor.readGasMeasurementData(true) != 0) {
        Serial.println(F("Cannot read. Fix kSlaveAddr / wiring / kInitialBaud."));
        for (;;)
            delay(1000);
    }
    if (!sensor.lastMeasure.dataValid) {
        Serial.println(F("Link OK but measurement not valid yet; wait and retry."));
        for (;;)
            delay(1000);
    }
    printOneLine("Before change: ");

    Serial.println(F("Step: writeDeviceBaudCode -> HOST_SERIAL.begin(target) -> COMMIT..."));

    uint8_t err = writeBaudCodeReopenUartThenCommit(kTargetBaudCode, kTargetUartBaud);
    if (err != 0) {
        Serial.print(F("baud+commit failed, code "));
        Serial.println(err);
        for (;;)
            delay(1000);
    }

    Serial.print(F("COMMIT OK. Sensor + HOST_SERIAL now "));
    Serial.print(kTargetUartBaud);
    Serial.println(F(" - loop will keep this baud."));

    printOneLine("After change: ");
}

void loop() {
    printOneLine(nullptr);
    delay(1000);
}
