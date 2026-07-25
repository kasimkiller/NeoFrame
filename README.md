# NeoFrame - 智能电子画框

基于 **ESP32-S3-WROOM-1** 和 **大连佳显 GDEP133C02** (1200x1600 六色电子墨水屏) 的智能画框控制器。

## 功能特性

- **WiFi 双模式**: 同时支持 AP 热点 (192.168.4.1) 和 STA 连接家中路由器
- **Web 配置页面**: 无需安装 App，通过手机/电脑浏览器即可完成所有操作
- **图片上传**: 通过网页上传图片，支持 JPG/PNG/GIF/WebP 格式
- **多种抖动算法**: Floyd-Steinberg、Floyd-Steinberg Serpentine、Atkinson、Stucki、Jarvis-Judice-Ninke
- **多种颜色模式**: 六色 / 四色 / 三色 / 黑白
- **实时显示模式**: 上传图片后立即显示
- **幻灯片轮播模式**: 自动循环播放已存储的多张图片
- **深色主题网页**: 与原固件一致的视觉风格

## 硬件需求

| 组件 | 规格 | 说明 |
|------|------|------|
| 主控 | ESP32-S3-WROOM-1 | 双核 240MHz, 4MB Flash, OPI PSRAM |
| 屏幕 | 大连佳显 GDEP133C02 | 13.3寸, 1200x1600, 六色 (黑/白/黄/红/蓝/绿) |
| 存储 | 内置 Flash (LittleFS) | 分区方案: Huge APP (3MB No OTA / 1MB SPIFFS) |

### GDEP133C02 屏幕特性

- **分辨率**: 1200 x 1600 像素
- **颜色**: 六色电子墨水 (Black/White/Yellow/Red/Blue/Green)
- **驱动IC**: 双驱动 (左右各一个)
- **接口**: 3线SPI (无DC引脚)
- **数据格式**: 每像素4位, 每字节2个像素
- **颜色编码**: `0x0=黑, 0x1=白, 0x2=黄, 0x3=红, 0x5=蓝, 0x6=绿`
- **BUSY信号**: 低电平有效 (BUSY=0时忙碌, BUSY=1时空闲)

### 引脚连接 (ESP32-S3-WROOM-1 → GDEP133C02)

| 信号 | ESP32-S3 GPIO | 屏幕引脚 | 说明 |
|------|--------------|---------|------|
| CS0 | GPIO 18 | CS0 | 右半屏片选 (x=600~1199) |
| CS1 | GPIO 17 | CS1 | 左半屏片选 (x=0~599) |
| SCK | GPIO 9 | CLK | SPI 时钟 |
| MOSI | GPIO 41 | DIN | SPI 数据输入 |
| BUSY | GPIO 7 | BUSY | 忙信号 (输入, 低电平有效) |
| RST | GPIO 6 | RST | 复位 (输出) |
| GND | GND | GND | 地 |
| 3.3V | 3.3V | VCC | 电源 |

> **注意**: 引脚定义在 `Config.h` 和 `EPD_Driver.h` 中，如需修改请同步更新两个文件。

## 开发环境

### 必需软件

1. **Arduino IDE** (推荐 2.x 版本)
   - 下载: https://www.arduino.cc/en/software

2. **Arduino-ESP32 核心 v3.3.0+**
   - 在 Arduino IDE 的 `文件 -> 首选项` 中添加开发板管理器 URL:
     ```
     https://dl.espressif.com/dl/package_esp32_index.json
     ```
   - 在 `工具 -> 开发板 -> 开发板管理器` 中搜索并安装 **esp32 by Espressif Systems**

### 必需库

| 库名 | 版本 | 安装方式 |
|------|------|---------|
| ArduinoJson | v6.x | Arduino IDE 库管理器 |

> 其他库 (WebServer, LittleFS, SPI, WiFi) 已包含在 Arduino-ESP32 核心中。

## 编译与上传

### 1. 选择开发板

```
工具 -> 开发板 -> esp32 -> ESP32S3 Dev Module
```

### 2. 分区方案

```
工具 -> Partition Scheme -> Huge APP (3MB No OTA / 1MB SPIFFS)
```

### 3. 其他设置

| 设置项 | 推荐值 |
|--------|-------|
| CPU Frequency | 240MHz |
| Flash Mode | QIO |
| Flash Size | 4MB |
| PSRAM | OPI PSRAM |
| Upload Speed | 921600 |

### 4. 上传固件

1. 用 USB 线连接 ESP32-S3 到电脑
2. 选择正确的 COM 端口
3. 点击 `上传` 按钮
4. 如果自动下载失败，按住 **BOOT** 键同时按 **RESET** 键进入下载模式

## 使用方法

### 首次配置

1. 设备启动后会创建一个名为 `NeoFrame-Setup` 的 WiFi 热点
2. 用手机/电脑连接该热点 (密码: `neoframe123`)
3. 打开浏览器访问 `http://192.168.4.1`
4. 在页面上输入家中 WiFi 的 SSID 和密码，点击"连接 WiFi"
5. 设备会自动重启并尝试连接路由器

### 上传图片

1. 确保设备已连接到家中 WiFi (或使用 AP 模式直连)
2. 打开浏览器访问设备的 IP 地址
3. 点击"选择图片"上传一张图片
4. 选择颜色模式和抖动算法
5. 点击"发送到画框"

### 模式切换

- **实时模式**: 上传图片后立即显示，适合即时更新
- **幻灯片模式**: 自动轮播已存储的所有图片，间隔可配置

## 文件结构

```
NeoFrame/
├── NeoFrame.ino      # 主程序 (WiFi/Web服务器/文件系统/幻灯片逻辑)
├── Config.h          # 配置文件 (引脚、WiFi、屏幕参数、驱动命令)
├── EPD_Driver.h      # GDEP133C02 驱动 (完整实现, 基于佳显官方示例)
├── WebPage.h         # Web 配置页面 (HTML/CSS/JS + 5种抖动算法)
└── README.md         # 本文件
```

## 数据格式说明

### 六色模式数据格式

网页端处理后的图片数据格式如下:

- **总大小**: 960,000 字节 (1200 x 1600 像素 x 4bpp / 8)
- **组织方式**: 行优先 (Row-major)
- **扫描方向**: X轴从右到左
- **每行字节数**: 600 字节

像素在缓冲区中的位置:
```
byteIndex = (y * 600) + ((1199 - x) / 2)
```

每个字节包含2个像素:
- **高4位**: 第一个像素 (对应较大的X值, 即屏幕右侧)
- **低4位**: 第二个像素 (对应较小的X值, 即屏幕左侧)

### 颜色映射

| JS端索引 | 颜色 | 驱动IC编码 |
|---------|------|-----------|
| 0 | 白 | 0x1 |
| 1 | 黑 | 0x0 |
| 2 | 红 | 0x3 |
| 3 | 黄 | 0x2 |
| 4 | 蓝 | 0x5 |
| 5 | 绿 | 0x6 |

### 双CS分屏发送

屏幕由两个驱动IC控制，数据发送顺序:
1. **CS0** (GPIO 18): 发送每行的前300字节 (对应屏幕右半部 x=600~1199)
2. **CS1** (GPIO 17): 发送每行的后300字节 (对应屏幕左半部 x=0~599)

## 故障排除

### 无法连接 WiFi
- 检查 SSID 和密码是否正确
- 确认路由器支持 2.4GHz (ESP32-S3 不支持 5GHz)
- 检查路由器是否开启了 MAC 地址过滤

### 图片上传失败
- 确认设备 IP 地址正确
- 检查文件系统空间是否充足
- 确保图片大小适中 (建议不超过 2MB)

### 屏幕不显示
- 检查 SPI 引脚连接是否正确 (特别注意 CS0/CS1 不要接反)
- 检查 BUSY 引脚是否正确连接 (GPIO 7)
- 检查 RST 引脚是否正确连接 (GPIO 6)
- 确认电源电压为 3.3V
- 通过串口查看调试信息 (波特率 115200)

### 显示颜色不正确
- 确认 `WebPage.h` 中的 `DRIVER_MAP` 数组正确映射了颜色编码
- 检查 `EPD_Driver.h` 中的初始化命令序列是否与屏幕匹配

## 参考资源

- 大连佳显 GDEP133C02 官方示例程序 (基于 ESP-IDF)
- [Espressif Arduino-ESP32 文档](https://docs.espressif.com/projects/arduino-esp32/)

## 许可证

本项目为开源项目，仅供学习和个人使用。

## 致谢

- 原固件逆向分析提供了本项目的设计参考
- 大连佳显官方示例程序提供了驱动IC的初始化序列和通信协议
- Espressif 提供的 Arduino-ESP32 核心
