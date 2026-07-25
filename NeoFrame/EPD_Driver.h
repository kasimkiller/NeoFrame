#ifndef EPD_DRIVER_H
#define EPD_DRIVER_H

#include <Arduino.h>
#include <SPI.h>

// 引脚和屏幕参数由 Config.h 提供，此处不再重复定义
// Config.h 中已定义: EPD_SCK_PIN, EPD_MOSI_PIN, EPD_CS0_PIN, EPD_CS1_PIN,
//                     EPD_BUSY_PIN, EPD_RST_PIN, SCREEN_WIDTH, SCREEN_HEIGHT,
//                     SCREEN_HALF_WIDTH, BYTES_PER_HALF_ROW, IMAGE_DATA_SIZE

// ===================== 驱动命令 (复刻佳显定义) =====================
#define CMD_PSR             0x00
#define CMD_PWR             0x01
#define CMD_POF             0x02
#define CMD_PON             0x04
#define CMD_BTST_N          0x05
#define CMD_BTST_P          0x06
#define CMD_DTM             0x10
#define CMD_DRF             0x12
#define CMD_CDI             0x50
#define CMD_TCON            0x60
#define CMD_TRES            0x61
#define CMD_PTLW            0x83
#define CMD_AN_TM           0x74
#define CMD_AGID            0x86
#define CMD_BUCK_BOOST_VDDN 0xB0
#define CMD_TFT_VCOM_POWER  0xB1
#define CMD_EN_BUF          0xB6
#define CMD_BOOST_VDDP_EN   0xB7
#define CMD_CCSET           0xE0
#define CMD_PWS             0xE3
#define CMD_CMD66           0xF0

// ===================== 初始化参数 (严格复刻佳显 GDEP133C02.c) =====================
static const uint8_t PSR_V[2]     = {0xDF, 0x69};
static const uint8_t PWR_V[6]     = {0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38};
static const uint8_t POF_V[1]     = {0x00};
static const uint8_t DRF_V[1]     = {0x01};
static const uint8_t CDI_V[1]     = {0xF7};
static const uint8_t TCON_V[2]    = {0x03, 0x03};
static const uint8_t TRES_V[4]    = {0x04, 0xB0, 0x03, 0x20};
static const uint8_t CMD66_V[6]   = {0x49, 0x55, 0x13, 0x5D, 0x05, 0x10};
static const uint8_t EN_BUF_V[1]  = {0x07};
static const uint8_t CCSET_V[1]   = {0x01};
static const uint8_t PWS_V[1]     = {0x22};
static const uint8_t AN_TM_V[9]   = {0xC0, 0x1E, 0x1E, 0xCE, 0xCE, 0xCE, 0x15, 0x15, 0x55};
static const uint8_t AGID_V[1]    = {0x10};
static const uint8_t BTST_P_V[2]  = {0xE8, 0x28};
static const uint8_t BOOST_VDDP_EN_V[1] = {0x01};
static const uint8_t BTST_N_V[2]  = {0xE8, 0x28};
static const uint8_t BUCK_BOOST_VDDN_V[1] = {0x01};
static const uint8_t TFT_VCOM_POWER_V[1]  = {0x02};

// ===================== 颜色定义 (4bpp, 复刻佳显) =====================
#define COLOR_BLACK  0x00  // 0x0 = 黑
#define COLOR_WHITE  0x11  // 0x1 = 白
#define COLOR_YELLOW 0x22  // 0x2 = 黄
#define COLOR_RED    0x33  // 0x3 = 红
#define COLOR_BLUE   0x55  // 0x5 = 蓝
#define COLOR_GREEN  0x66  // 0x6 = 绿

class EPD_Driver {
public:
    EPD_Driver() {}

    // ===================== GPIO 操作 (复刻佳显) =====================
    void setPinCsAll(uint8_t level) {
        digitalWrite(EPD_CS0_PIN, level);
        digitalWrite(EPD_CS1_PIN, level);
    }

    void setPinCs(uint8_t csNumber, uint8_t level) {
        if (csNumber == 0) {
            digitalWrite(EPD_CS0_PIN, level);
        } else {
            digitalWrite(EPD_CS1_PIN, level);
        }
    }

    void resetPin(uint8_t level) {
        digitalWrite(EPD_RST_PIN, level);
    }

    // BUSY = 0 时忙碌, BUSY = 1 时空闲
    void checkBusyHigh(void) {
        while (digitalRead(EPD_BUSY_PIN) == LOW) {
            delay(1);
        }
    }

    void checkBusyLow(void) {
        while (digitalRead(EPD_BUSY_PIN) == HIGH) {
            delay(1);
        }
    }

    // ===================== SPI 通信 (复刻佳显 writeEpd / writeEpdCommand / writeEpdData) =====================
    // 发送命令+数据 (CS 在外部控制)
    void writeEpd(uint8_t cmd, const uint8_t *data, uint16_t len) {
        SPI.transfer(cmd);
        if (len > 0 && data != nullptr) {
            SPI.writeBytes((uint8_t*)data, len);
        }
    }

    // 仅发送命令 (CS 在外部控制)
    void writeEpdCommand(uint8_t cmd) {
        SPI.transfer(cmd);
    }

    // 仅发送数据 (CS 在外部控制, 通常在 writeEpdCommand 之后连续调用)
    void writeEpdData(const uint8_t *data, uint32_t len) {
        SPI.writeBytes((uint8_t*)data, len);
    }

    // ===================== 硬件复位 (严格复刻佳显 20ms 时序) =====================
    void epdHardwareReset(void) {
        resetPin(LOW);
        delay(20);
        resetPin(HIGH);
        delay(20);
    }

    // ===================== 初始化序列 (严格复刻佳显 initEPD) =====================
    void init() {
        // GPIO 配置
        pinMode(EPD_RST_PIN, OUTPUT);
        pinMode(EPD_BUSY_PIN, INPUT);
        pinMode(EPD_CS0_PIN, OUTPUT);
        pinMode(EPD_CS1_PIN, OUTPUT);

        // 初始状态: 所有 CS 拉高, RST 拉高
        setPinCsAll(HIGH);
        resetPin(HIGH);

        // 初始化 SPI (3线, 无DC引脚, 10MHz, Mode0)
        SPI.begin(EPD_SCK_PIN, -1, EPD_MOSI_PIN, -1);
        SPI.setFrequency(10000000);  // 10 MHz
        SPI.setDataMode(SPI_MODE0);
        SPI.setBitOrder(MSBFIRST);

        // 硬件复位
        epdHardwareReset();

        // 等待面板就绪 (BUSY 变高)
        checkBusyHigh();

        // ========== 初始化命令序列 (严格复刻佳显 GDEP133C02.c initEPD) ==========
        // 注意: 某些命令只发 CS0, 某些命令发所有 CS

        // 1. AN_TM → 仅 CS0
        setPinCs(0, LOW);
        writeEpd(CMD_AN_TM, AN_TM_V, sizeof(AN_TM_V));
        setPinCsAll(HIGH);

        // 2. CMD66 → 所有 CS
        setPinCsAll(LOW);
        writeEpd(CMD_CMD66, CMD66_V, sizeof(CMD66_V));
        setPinCsAll(HIGH);

        // 3. PSR → 所有 CS
        setPinCsAll(LOW);
        writeEpd(CMD_PSR, PSR_V, sizeof(PSR_V));
        setPinCsAll(HIGH);

        // 4. CDI → 所有 CS
        setPinCsAll(LOW);
        writeEpd(CMD_CDI, CDI_V, sizeof(CDI_V));
        setPinCsAll(HIGH);

        // 5. TCON → 所有 CS
        setPinCsAll(LOW);
        writeEpd(CMD_TCON, TCON_V, sizeof(TCON_V));
        setPinCsAll(HIGH);

        // 6. AGID → 所有 CS
        setPinCsAll(LOW);
        writeEpd(CMD_AGID, AGID_V, sizeof(AGID_V));
        setPinCsAll(HIGH);

        // 7. PWS → 所有 CS
        setPinCsAll(LOW);
        writeEpd(CMD_PWS, PWS_V, sizeof(PWS_V));
        setPinCsAll(HIGH);

        // 8. CCSET → 所有 CS
        setPinCsAll(LOW);
        writeEpd(CMD_CCSET, CCSET_V, sizeof(CCSET_V));
        setPinCsAll(HIGH);

        // 9. TRES → 所有 CS
        setPinCsAll(LOW);
        writeEpd(CMD_TRES, TRES_V, sizeof(TRES_V));
        setPinCsAll(HIGH);

        // 10. PWR → 仅 CS0
        setPinCs(0, LOW);
        writeEpd(CMD_PWR, PWR_V, sizeof(PWR_V));
        setPinCsAll(HIGH);

        // 11. EN_BUF → 仅 CS0
        setPinCs(0, LOW);
        writeEpd(CMD_EN_BUF, EN_BUF_V, sizeof(EN_BUF_V));
        setPinCsAll(HIGH);

        // 12. BTST_P → 仅 CS0
        setPinCs(0, LOW);
        writeEpd(CMD_BTST_P, BTST_P_V, sizeof(BTST_P_V));
        setPinCsAll(HIGH);

        // 13. BOOST_VDDP_EN → 仅 CS0
        setPinCs(0, LOW);
        writeEpd(CMD_BOOST_VDDP_EN, BOOST_VDDP_EN_V, sizeof(BOOST_VDDP_EN_V));
        setPinCsAll(HIGH);

        // 14. BTST_N → 仅 CS0
        setPinCs(0, LOW);
        writeEpd(CMD_BTST_N, BTST_N_V, sizeof(BTST_N_V));
        setPinCsAll(HIGH);

        // 15. BUCK_BOOST_VDDN → 仅 CS0
        setPinCs(0, LOW);
        writeEpd(CMD_BUCK_BOOST_VDDN, BUCK_BOOST_VDDN_V, sizeof(BUCK_BOOST_VDDN_V));
        setPinCsAll(HIGH);

        // 16. TFT_VCOM_POWER → 仅 CS0
        setPinCs(0, LOW);
        writeEpd(CMD_TFT_VCOM_POWER, TFT_VCOM_POWER_V, sizeof(TFT_VCOM_POWER_V));
        setPinCsAll(HIGH);

        delay(100);
    }

    // ===================== 显示刷新 (严格复刻佳显 epdDisplay) =====================
    void epdDisplay(void) {
        // 1. PON - Power On
        setPinCsAll(LOW);
        writeEpdCommand(CMD_PON);
        setPinCsAll(HIGH);
        checkBusyHigh();

        // 2. DRF - Display Refresh
        setPinCsAll(LOW);
        delay(30);
        writeEpd(CMD_DRF, DRF_V, sizeof(DRF_V));
        setPinCsAll(HIGH);
        checkBusyHigh();

        // 3. POF - Power Off
        setPinCsAll(LOW);
        writeEpd(CMD_POF, POF_V, sizeof(POF_V));
        setPinCsAll(HIGH);
        checkBusyHigh();
    }

    // ===================== 全屏显示 (严格复刻佳显 pic_display_test) =====================
    // data: 960000 字节, 行优先, 每行 600 字节
    //       每字节 = 2 个 4bpp 像素 (高4位=左侧像素, 低4位=右侧像素)
    //       缓冲区: 每行 600 字节
    //       字节 0~299 → CS0 (左半屏 x=0~599)
    //       字节 300~599 → CS1 (右半屏 x=600~1199)
    void display(const uint8_t *data, uint32_t len) {
        if (len != IMAGE_DATA_SIZE) {
            return;
        }

        // 计算每半屏宽度参数 (复刻佳显 pic_display_test)
        uint16_t Width = (SCREEN_WIDTH % 2 == 0) ? (SCREEN_WIDTH / 2) : (SCREEN_WIDTH / 2 + 1); // 600
        uint16_t Width1 = (Width % 2 == 0) ? (Width / 2) : (Width / 2 + 1);                     // 300
        uint16_t Height = SCREEN_HEIGHT;                                                        // 1600

        // --- 发送右半屏数据 (CS0) ---
        // 复刻: setPinCsAll(HIGH); setPinCs(0, 0); writeEpdCommand(DTM);
        setPinCsAll(HIGH);
        setPinCs(0, LOW);
        writeEpdCommand(CMD_DTM);

        // 发送每行前半部分数据 (字节 0~299 对应每行)
        // 复刻: for (...) { writeEpdData(num + i * Width, Width1); }
        for (uint16_t i = 0; i < Height; i++) {
            writeEpdData(data + i * Width, Width1);
            delay(1);  // 复刻佳显 vTaskDelay(pdMS_TO_TICKS(1))
        }
        setPinCsAll(HIGH);

        // --- 发送左半屏数据 (CS1) ---
        // 复刻: setPinCs(1, 0); writeEpdCommand(DTM);
        setPinCs(1, LOW);
        writeEpdCommand(CMD_DTM);

        // 发送每行后半部分数据 (字节 300~599 对应每行)
        // 复刻: for (...) { writeEpdData(num + i * Width + Width1, Width1); }
        for (uint16_t i = 0; i < Height; i++) {
            writeEpdData(data + i * Width + Width1, Width1);
            delay(1);  // 复刻佳显 vTaskDelay(pdMS_TO_TICKS(1))
        }
        setPinCsAll(HIGH);

        // --- 刷新屏幕 ---
        epdDisplay();
        delay(10);
    }
    
// ===================== 清屏 (复刻佳显 epdDisplayColor) =====================
    void clear(uint8_t colorSelect) {
        uint8_t fillBuf[256];
        memset(fillBuf, colorSelect, sizeof(fillBuf));

        // 总共需要发送 480000 字节到每个 CS
        // 使用 256 字节缓冲区分块发送
        uint32_t totalPerCs = 480000;

        setPinCsAll(LOW);
        writeEpdCommand(CMD_DTM);

        for (uint32_t sent = 0; sent < totalPerCs; sent += sizeof(fillBuf)) {
            uint32_t chunk = sizeof(fillBuf);
            if (sent + chunk > totalPerCs) {
                chunk = totalPerCs - sent;
            }
            writeEpdData(fillBuf, chunk);
        }
        setPinCsAll(HIGH);

        epdDisplay();
    }

    // 白色清屏快捷方法
    void clearWhite() {
        clear(COLOR_WHITE);
    }

    // ===================== 检查驱动IC状态 (复刻佳显 checkDriverICStatus) =====================
    uint8_t checkDriverICStatus(void) {
        uint8_t status = 0;  // 0 = DONE
        uint8_t dataBuf[3];

        for (uint8_t csx = 0; csx < 2; csx++) {
            memset(dataBuf, 0, sizeof(dataBuf));
            setPinCs(csx, LOW);
            // 读取命令 0xF2 (类似佳显代码)
            SPI.transfer(0xF2);
            SPI.transferBytes(nullptr, dataBuf, sizeof(dataBuf));
            setPinCs(csx, HIGH);

            if ((dataBuf[0] & 0x01) != 0x01) {
                status = 1;  // ERROR
            }
        }
        return status;
    }
};

#endif // EPD_DRIVER_H
