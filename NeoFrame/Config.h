/**
 * NeoFrame - 智能电子画框配置文件
 * 
 * 硬件平台: ESP32-S3-WROOM-1
 * 屏幕: 大连佳显 GDEP133C02 (1200x1600 六色电子墨水屏)
 * 
 * GDEP133C02 屏幕特性:
 * - 分辨率: 1200 x 1600 像素
 * - 颜色: 六色 (黑/白/黄/红/蓝/绿)
 * - 驱动IC: 双驱动 (左右各一个, CS0=左半, CS1=右半)
 * - 接口: 3线SPI (无DC引脚, 命令和数据通过时序区分)
 * - 数据格式: 每像素4位, 每字节2个像素
 *   颜色编码: 0x0=黑, 0x1=白, 0x2=黄, 0x3=红, 0x5=蓝, 0x6=绿
 * - BUSY信号: 低电平有效 (BUSY=0时忙碌, BUSY=1时空闲)
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ===================== 调试配置 =====================
#define DEBUG_SERIAL      Serial
#define DEBUG_BAUD        115200
#define DEBUG_ENABLE      true

#if DEBUG_ENABLE
  #define DEBUG_PRINT(x)    DEBUG_SERIAL.print(x)
  #define DEBUG_PRINTLN(x)  DEBUG_SERIAL.println(x)
  #define DEBUG_PRINTF(...) DEBUG_SERIAL.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// ===================== WiFi 配置 =====================
#define AP_SSID           "NeoFrame-Setup"
#define AP_PASSWORD       "neoframe123"   // AP模式密码，至少8位
#define AP_CHANNEL        1
#define AP_MAX_CLIENTS    4

// WiFi 连接超时 (毫秒)
#define WIFI_CONNECT_TIMEOUT  20000
// WiFi 断线重连间隔 (毫秒)
#define WIFI_RECONNECT_INTERVAL 30000

// WiFi 凭证存储文件
#define WIFI_CONFIG_FILE  "/wifi_config.json"

// ===================== 屏幕配置 =====================
#define SCREEN_WIDTH      1200
#define SCREEN_HEIGHT     1600

// 屏幕分两半，每半宽度
#define SCREEN_HALF_WIDTH  600

// 每行每半的数据字节数 (600像素 / 2像素每字节 = 300字节)
#define BYTES_PER_HALF_ROW 300

// 颜色模式
enum ColorMode {
  MODE_SIX_COLOR = 0,       // 六色: 白/黑/红/黄/蓝/绿
  MODE_FOUR_COLOR = 1,      // 四色: 白/黑/红/黄
  MODE_THREE_COLOR = 2,     // 三色: 白/黑/红
  MODE_BLACK_WHITE = 3      // 黑白
};

// 数据缓冲区大小 (六色模式，每像素4位)
// 1200 * 1600 = 1,920,000 像素
// 1,920,000 / 2 = 960,000 字节
#define IMAGE_DATA_SIZE   960000

// ===================== GDEP133C02 引脚定义 (ESP32-S3-WROOM-1) =====================
// 根据佳显官方示例代码定义
#define EPD_CS0_PIN       18    // 左半屏 CS (SPI_CS0)
#define EPD_CS1_PIN       17    // 右半屏 CS (SPI_CS1)
#define EPD_SCK_PIN       9     // SPI 时钟
#define EPD_MOSI_PIN      41    // SPI 数据输出 (Data0)
#define EPD_MISO_PIN      40    // SPI 数据输入 (Data1) - 保留但屏幕主要用MOSI
#define EPD_BUSY_PIN      7     // 忙信号 (输入, 低电平有效)
#define EPD_RST_PIN       6     // 复位 (输出)
#define EPD_LOAD_SW_PIN   45    // 负载开关 (输出) - 示例中配置但未使用

// ===================== 文件系统配置 =====================
#define FILESYSTEM        LittleFS

// 图片存储目录
#define IMAGE_DIR         "/images"
// 图片文件前缀
#define IMAGE_PREFIX      "/images/img_"
// 最大存储图片数量
#define MAX_IMAGES        10
// 图片元数据文件
#define IMAGE_INDEX_FILE  "/image_index.json"

// ===================== 幻灯片配置 =====================
// 幻灯片切换间隔 (毫秒)
#define SLIDE_SHOW_INTERVAL  30000   // 默认30秒
#define SLIDE_SHOW_MIN       5000    // 最小5秒
#define SLIDE_SHOW_MAX       3600000 // 最大1小时

// ===================== Web服务器配置 =====================
#define WEB_SERVER_PORT   80
#define UPLOAD_MAX_SIZE   (IMAGE_DATA_SIZE + 1024)  // 最大上传大小

// ===================== 模式定义 =====================
enum DisplayMode {
  MODE_REALTIME = 0,    // 实时显示模式
  MODE_SLIDESHOW = 1    // 幻灯片轮播模式
};

// ===================== WiFi 状态定义 =====================
enum WiFiState {
  MODE_AP_ONLY = 0,     // 仅 AP 模式（未连接或连接失败）
  MODE_STA_ONLY = 1     // 仅 STA 模式（已连接，AP 已关闭）
};

// ===================== 省电级别定义 =====================
enum PowerLevel {
  PWR_NORMAL = 0,       // 全速运行（上传/刷新中）
  PWR_MODEM_SLEEP = 1   // Modem Sleep（STA 默认）
};

// ===================== 省电配置 =====================
// STA 模式下无操作多久后启用 Modem Sleep（毫秒）
#define PWR_IDLE_TO_MODEM_MS     (30 * 1000)      // 30秒
// WiFi 连接状态检查间隔（毫秒）
#define WIFI_CHECK_INTERVAL      (10 * 1000)      // 10秒
// WiFi 连接尝试次数
#define WIFI_CONNECT_MAX_RETRY   2
// 每次连接尝试超时（毫秒）
#define WIFI_CONNECT_TIMEOUT_MS  15000            // 15秒
// 两次连接尝试之间的等待（毫秒）
#define WIFI_CONNECT_RETRY_DELAY_MS 3000          // 3秒

// ===================== GDEP133C02 驱动命令 =====================
#define GDEP_PSR             0x00
#define GDEP_PWR             0x01
#define GDEP_POF             0x02
#define GDEP_PON             0x04
#define GDEP_BTST_N          0x05
#define GDEP_BTST_P          0x06
#define GDEP_DTM             0x10
#define GDEP_DRF             0x12
#define GDEP_CDI             0x50
#define GDEP_TCON            0x60
#define GDEP_TRES            0x61
#define GDEP_PTLW            0x83
#define GDEP_AN_TM           0x74
#define GDEP_AGID            0x86
#define GDEP_BUCK_BOOST_VDDN 0xB0
#define GDEP_TFT_VCOM_POWER  0xB1
#define GDEP_EN_BUF          0xB6
#define GDEP_BOOST_VDDP_EN   0xB7
#define GDEP_CCSET           0xE0
#define GDEP_PWS             0xE3
#define GDEP_CMD66           0xF0

#endif // CONFIG_H
