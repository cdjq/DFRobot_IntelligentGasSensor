/*!
 * @file uartAutoReportListen.ino
 * @brief SEN0742（TTL UART）：通过 Modbus 打开固件「主动上报」，再在主机口监听自发 FC 0x04 应答帧并打印气体与浓度。
 *
 * @details
 * 需 **SEN0742** 板 + 已支持保持寄存器 0x0001 主动上报的固件（SEN0738 库 GasModbusRtu）。
 * 接线与 `readGasModbus.ino` 相同：MCU 的 Modbus 串口 TX/RX 与传感器主机口交叉相连、共地。
 *
 * 流程：先 `readIdentification` 确认 PID → `setUartAutoReport(true, true)` → 循环从串口组帧并
 * `parseUnsolicitedInputReadResponse`（与 `readMeasurement(false)` 所读 12 个输入寄存器语义一致）。
 */
#include <DFRobot_IntelligentGasSensor.h>

static const unsigned long kModbusBaud = 9600;
/** 9600 8N1 下约 3.5 字符时间 < 4 ms；略加大以便整帧到齐后再解析 */
static const unsigned long kFrameIdleMs = 8;

/** Modbus 从站号，须与传感器保持寄存器 0x0006 一致。
 *  若串口助手只有「05 …」这类帧有回、而「01 …」无回，说明从站是 5，这里改成 5。 */
static const uint8_t kModbusSlave = 5;

#if defined(ARDUINO_ARCH_ESP32)
#define MODBUS_SERIAL Serial2
#define MODBUS_RX 16
#define MODBUS_TX 17
DFRobot_IntelligentGasSensor sensor(&MODBUS_SERIAL, kModbusSlave);
#else
#define MODBUS_SERIAL Serial1
DFRobot_IntelligentGasSensor sensor(&MODBUS_SERIAL, kModbusSlave);
#endif

/**
 * 排空 Modbus 串口接收；**带总时长上限**，避免误用 DFRobot_RTU 内部 `clearRecvBuffer()` 时
 * 若 `available()` 一直为真（线噪声、电平异常、或从机持续推流）导致死循环。
 */
static void drainModbusRxForMs(unsigned long ms) {
    const unsigned long tEnd = millis() + ms;
    while ((long)(tEnd - millis()) > 0) {
        unsigned n = 0;
        while (MODBUS_SERIAL.available() != 0 && n < 256) {
            (void)MODBUS_SERIAL.read();
            ++n;
        }
        if (n == 0)
            delay(1);
    }
}

static void flushAndDelay(void) {
    drainModbusRxForMs(50);
    delay(50);
}

void setup() {
    Serial.begin(115200);
#if defined(USBCON) || defined(ARDUINO_USB_CDC_ON_BOOT) || defined(ARDUINO_ARCH_RP2040)
    /* 原生 USB CDC 板：等 USB 枚举完成再打印，避免用户以为“无输出” */
    delay(1500);
#else
    delay(500);
#endif
    /* 有限等待 USB 串口（无 USB 连接时最多等约 3 s，避免永久卡死） */
    {
        const unsigned long tUsb = millis() + 3000;
        while (!Serial && (long)(tUsb - millis()) > 0)
            delay(10);
    }

    Serial.println(F("[setup 0] UART auto-report demo — debug"));

#if defined(ARDUINO_ARCH_ESP32)
    Serial.println(F("[setup 1] ModbusSerial.begin..."));
    MODBUS_SERIAL.begin(kModbusBaud, SERIAL_8N1, MODBUS_RX, MODBUS_TX);
#else
    Serial.println(F("[setup 1] ModbusSerial.begin..."));
    MODBUS_SERIAL.begin(kModbusBaud);
#endif

    /* 主动上报若已开启，会持续往主机口推 FC0x04；先排空一段时间再发 Modbus 请求，减轻 RTU 解析错位/久等 */
    Serial.println(F("[setup 2] drain Modbus RX (~800 ms, drop auto-report burst)..."));
    drainModbusRxForMs(800);

    sensor.setTimeoutTimeMs(500);

    Serial.println(F("[setup 3] readIdentification..."));
    flushAndDelay();

    uint16_t pid = 0, vid = 0, ver = 0;
    if (sensor.readIdentification(&pid, &vid, &ver) != 0) {
        Serial.println(F("readIdentification failed (wiring / baud / slave addr?)"));
        for (;;)
            delay(5000);
    }
    Serial.print(F("PID=0x"));
    Serial.print(pid, HEX);
    Serial.print(F(" VID=0x"));
    Serial.print(vid, HEX);
    Serial.print(F(" mapVer=0x"));
    Serial.println(ver, HEX);

    if (pid != DFROBOT_IGS_PID_SEN0742) {
        Serial.println(F("This example is for SEN0742 (TTL UART) only. Abort."));
        for (;;)
            delay(5000);
    }

    Serial.println(F("[setup 4] setUartAutoReport(true)+commit..."));
    if (sensor.setUartAutoReport(true, true) != 0) {
        Serial.println(F("setUartAutoReport+commit failed (firmware may not support reg 0x0001)."));
        for (;;)
            delay(5000);
    }

    Serial.println(F("[setup 5] done. Auto-report ON. Listening ~1 Hz FC 0x04 ..."));
    flushAndDelay();
}

void loop() {
    static uint8_t  rxBuf[96];
    static size_t   rxLen     = 0;
    static unsigned long lastRxMs = 0;

    while (MODBUS_SERIAL.available() != 0) {
        const int c = MODBUS_SERIAL.read();
        if (c < 0)
            break;
        lastRxMs = millis();
        if (rxLen < sizeof(rxBuf))
            rxBuf[rxLen++] = (uint8_t)c;
        else {
            memmove(rxBuf, rxBuf + 1, sizeof(rxBuf) - 1);
            rxBuf[sizeof(rxBuf) - 1] = (uint8_t)c;
        }
    }

    constexpr size_t kAdu = (size_t)(1 + 1 + 1 + (size_t)DFROBOT_IGS_INPUT_REG_COUNT_NO_TIMESTAMP * 2 + 2);

    if (rxLen == 0) {
        delay(10);
        return;
    }

    /* 必须在读完串口后再取 millis()，否则 now < lastRxMs 时无符号下溢会误判“已静默”→ 忙等卡死 */
    const unsigned long now = millis();
    if ((unsigned long)(now - lastRxMs) < kFrameIdleMs) {
        delay(1);
        return;
    }

    if (rxLen < kAdu) {
        /* 已静默但凑不满 29 字节：噪声/断帧，清空避免死循环 */
        rxLen = 0;
        delay(2);
        return;
    }

    bool parsed = false;
    for (size_t off = 0; off + kAdu <= rxLen; ++off) {
        if (sensor.parseUnsolicitedInputReadResponse(rxBuf + off, kAdu) == 0) {
            Serial.print(sensor.lastMeasure.gasType);
            Serial.print(' ');
            Serial.print(sensor.getConcentrationFloat(), 2);
            Serial.print(' ');
            Serial.println(sensor.lastMeasure.unit);

            const size_t tail = rxLen - (off + kAdu);
            memmove(rxBuf, rxBuf + off + kAdu, tail);
            rxLen = tail;
            parsed = true;
            break;
        }
    }

    if (!parsed) {
        if (rxLen > 80) {
            rxLen = 0;
        } else {
            /* 长度够但 CRC/从站号不对：丢 1 字节重新对齐 */
            memmove(rxBuf, rxBuf + 1, --rxLen);
        }
    }

    delay(1);
}
