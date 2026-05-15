/*!
 * @file uartAutoReportListen.ino
 * @brief SEN0742：打开 UART 主动上报后，用 @ref DFRobot_IntelligentGasSensor::pollUnsolicitedAutoReport 只收解析。
 *
 * @details
 * `setup()` 刻意保持最短：只开串口、开主动上报。正式产品可再加链路探测、加长上电延时、
 * 排空 RX 等（见 `readGasModbus.ino`）。**从站号**须与传感器一致（`kModbusSlave`）。
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kModbusBaud = 9600;
static const uint8_t kModbusSlave = 5;

#if defined(ARDUINO_ARCH_ESP32)
#define SENSOR_SERIAL Serial2
#define MODBUS_RX 16
#define MODBUS_TX 17
DFRobot_IntelligentGasSensor sensor(&SENSOR_SERIAL, kModbusSlave);
#else
#define SENSOR_SERIAL Serial1
DFRobot_IntelligentGasSensor sensor(&SENSOR_SERIAL, kModbusSlave);
#endif

void setup() {
    Serial.begin(115200);
    delay(1000);

#if defined(ARDUINO_ARCH_ESP32)
    SENSOR_SERIAL.begin(kModbusBaud, SERIAL_8N1, MODBUS_RX, MODBUS_TX);
#else
    SENSOR_SERIAL.begin(kModbusBaud);
#endif

    while (SENSOR_SERIAL.available())
        (void)SENSOR_SERIAL.read();

    (void)sensor.setAcquireMode(sensor.ACQUIRE_MODE_ACTIVE, true);
    Serial.println(F("listening"));
}

void loop() {
    if (sensor.pollUnsolicitedAutoReport() == 0) {
        Serial.print(sensor.lastMeasure.gasType);
        Serial.print(' ');
        Serial.print(sensor.getConcentrationFloat(), 2);
        Serial.print(' ');
        Serial.println(sensor.lastMeasure.unit);
    }
    delay(2);
}
