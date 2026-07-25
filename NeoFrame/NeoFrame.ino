/**
 * NeoFrame - 智能电子画框
 * 
 * 基于 ESP32-S3 的电子墨水屏画框控制器
 * 
 * 功能:
 * 1. WiFi AP/STA 双模式
 * 2. 内置 Web 服务器提供图片上传和配置页面
 * 3. 支持幻灯片轮播模式
 * 4. 支持实时显示模式
 * 5. 使用 LittleFS 存储图片和配置
 * 
 * 硬件需求:
 * - ESP32-S3 开发板
 * - 1200x1600 六色电子墨水屏
 * - SPI 接口连接
 * 
 * 编译环境:
 * - Arduino IDE
 * - Arduino-ESP32 核心 v3.3.0+
 * - 分区方案: 自定义 partitions.csv (16MB Flash: 3MB APP / 约13MB SPIFFS)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <SPI.h>

#include "Config.h"
#include "EPD_Driver.h"
#include "WebPage.h"

// ===================== 全局对象 =====================
WebServer server(WEB_SERVER_PORT);
EPD_Driver epd;

// WiFi 状态
bool wifiConnected = false;
unsigned long lastWifiCheck = 0;
WiFiState wifiState = MODE_AP_ONLY;     // 当前 WiFi 运行模式

// 省电管理
PowerLevel powerLevel = PWR_NORMAL;     // 当前省电级别
unsigned long lastHttpRequest = 0;      // 上次 HTTP 请求时间
unsigned long bootTime = 0;             // 开机时间 (millis 偏移)
bool uploadInProgress = false;          // 是否正在上传
bool displayRefreshing = false;         // 是否正在刷新屏幕

// 上传响应状态（handleUpload 在 UPLOAD_FILE_END 记录，/upload 完成处理器统一发送一次）
int uploadRespCode = 0;                 // 0 = 尚未设置（完成处理器用 500 兜底）
String uploadRespMsg = "";
bool uploadRefreshPending = false;      // 上传成功后待刷新屏幕（响应发送后执行）
size_t uploadSavedLen = 0;              // 待刷新图像的数据长度

// 显示模式
volatile int currentDisplayMode = MODE_REALTIME;
volatile int currentColorMode = MODE_SIX_COLOR;

// 幻灯片轮播
volatile bool slideShowActive = false;
volatile int currentSlideIndex = 0;
volatile int totalSlides = 0;
unsigned long lastSlideChange = 0;
unsigned long slideInterval = SLIDE_SHOW_INTERVAL;

// 图像数据缓冲区 (使用 PSRAM)
uint8_t* imageBuffer = nullptr;

// ===================== 函数声明 =====================
void setupWiFi();
void setupWebServer();
void setupFileSystem();
void handleWiFiConfig();
void handleUpload();
void handleSwitchRealTime();
void handleSwitchSlideShow();
void handleGetStatus();
void handleWiFiScan();
void handleDisconnectWiFi();
void loadWiFiConfig(String& ssid, String& password);
void saveWiFiConfig(const String& ssid, const String& password);
void startAccessPoint();
void connectToWiFi(const String& ssid, const String& password);
bool tryConnectWiFi(const String& ssid, const String& password, unsigned long timeoutMs);
void checkWiFiConnection();
void handleSlideShow();
void displayImageFromFile(const String& path);
void displayImageFromBuffer(uint8_t* data, uint32_t len);
bool saveImageData(const uint8_t* data, size_t len, int index);
String getImagePath(int index);
bool ensureSpaceForImage(size_t len);
void evictOldestImage();
void updateSlideIndex();
int getImageCount();
void enterNormalMode();
void enterModemSleep();
void handlePowerManagement();

// ===================== 初始化 =====================
void setup() {
    DEBUG_SERIAL.begin(DEBUG_BAUD);
    delay(1000);
    
    DEBUG_PRINTLN(F("\n========================================"));
    DEBUG_PRINTLN(F("  NeoFrame - 智能电子画框"));
    DEBUG_PRINTLN(F("========================================"));
    DEBUG_PRINTLN();
    
    // 打印系统信息
    DEBUG_PRINTF("芯片型号: %s\n", ESP.getChipModel());
    DEBUG_PRINTF("CPU 频率: %d MHz\n", ESP.getCpuFreqMHz());
    DEBUG_PRINTF("Flash 大小: %d MB\n", ESP.getFlashChipSize() / 1024 / 1024);
    DEBUG_PRINTF("PSRAM 大小: %d MB\n", ESP.getPsramSize() / 1024 / 1024);
    DEBUG_PRINTF("Free Heap: %d bytes\n", ESP.getFreeHeap());
    DEBUG_PRINTF("Free PSRAM: %d bytes\n", ESP.getFreePsram());
    DEBUG_PRINTLN();
    
    // 初始化文件系统
    setupFileSystem();
    
    // 分配图像缓冲区 (使用 PSRAM)
    if (ESP.getPsramSize() > 0) {
        imageBuffer = (uint8_t*)ps_malloc(IMAGE_DATA_SIZE);
        DEBUG_PRINTLN(F("[内存] 使用 PSRAM 分配图像缓冲区"));
    }
    if (!imageBuffer) {
        imageBuffer = (uint8_t*)malloc(IMAGE_DATA_SIZE);
        DEBUG_PRINTLN(F("[内存] 使用 Heap 分配图像缓冲区"));
    }
    if (!imageBuffer) {
        DEBUG_PRINTLN(F("[错误] 无法分配图像缓冲区!"));
    }
    
    // 初始化 WiFi
    setupWiFi();
    
    // 初始化 Web 服务器
    setupWebServer();
    
    // 初始化电子墨水屏
    DEBUG_PRINTLN(F("[初始化] 电子墨水屏..."));
    epd.init();
    
    // 不执行清屏，保持屏幕原有内容
    DEBUG_PRINTLN(F("[显示] 保持屏幕原有内容 (不清屏)"));
    
    // 记录开机时间
    bootTime = millis();
    
    DEBUG_PRINTLN(F("\n[系统] 初始化完成!"));
    if (wifiState == MODE_STA_ONLY) {
        DEBUG_PRINTF("STA 地址: %s\n", WiFi.localIP().toString().c_str());
        DEBUG_PRINTLN(F("[省电] 启用 Modem Sleep"));
        enterModemSleep();
    } else {
        DEBUG_PRINTF("AP 地址: %s\n", WiFi.softAPIP().toString().c_str());
    }
    DEBUG_PRINTLN(F("========================================\n"));
}

// ===================== 主循环 =====================
void loop() {
    // 处理 Web 服务器请求
    server.handleClient();
    
    // 检查 WiFi 连接状态
    checkWiFiConnection();
    
    // 处理幻灯片轮播
    if (slideShowActive && currentDisplayMode == MODE_SLIDESHOW) {
        handleSlideShow();
    }
    
    // 省电管理
    handlePowerManagement();
    
    delay(1);
}

// ===================== 文件系统初始化 =====================
void setupFileSystem() {
    DEBUG_PRINTLN(F("[初始化] 文件系统 (LittleFS)..."));
    
    if (!LittleFS.begin(true)) {
        DEBUG_PRINTLN(F("[错误] LittleFS 挂载失败，尝试格式化..."));
        if (!LittleFS.begin(false)) {
            DEBUG_PRINTLN(F("[错误] LittleFS 格式化失败!"));
            return;
        }
    }
    
    // 创建图片存储目录
    if (!LittleFS.exists(IMAGE_DIR)) {
        LittleFS.mkdir(IMAGE_DIR);
        DEBUG_PRINTLN(F("[文件系统] 创建图片目录 /images"));
    }
    
    // 获取存储空间信息
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    DEBUG_PRINTF("[文件系统] 总空间: %d KB, 已用: %d KB, 可用: %d KB\n",
                 totalBytes / 1024, usedBytes / 1024, (totalBytes - usedBytes) / 1024);
    
    // 统计已存储的图片数量
    totalSlides = getImageCount();
    DEBUG_PRINTF("[文件系统] 已存储图片: %d 张\n", totalSlides);
}

// ===================== WiFi 初始化 =====================
void setupWiFi() {
    DEBUG_PRINTLN(F("[初始化] WiFi..."));
    
    // 尝试加载已保存的 WiFi 配置
    String savedSSID, savedPassword;
    loadWiFiConfig(savedSSID, savedPassword);
    
    if (savedSSID.length() == 0) {
        DEBUG_PRINTLN(F("[WiFi] 无保存配置，启动 AP_STA 模式"));
        startAccessPoint();
        wifiState = MODE_AP_ONLY;
        return;
    }
    
    DEBUG_PRINTF("[WiFi] 尝试连接已保存网络: %s\n", savedSSID.c_str());
    
    // 第 1 次连接尝试
    if (tryConnectWiFi(savedSSID, savedPassword, WIFI_CONNECT_TIMEOUT_MS)) {
        DEBUG_PRINTLN(F("[WiFi] 第 1 次连接成功，关闭 AP 进入纯 STA"));
        wifiConnected = true;
        wifiState = MODE_STA_ONLY;
        return;
    }
    
    DEBUG_PRINTLN(F("[WiFi] 第 1 次连接失败，准备第 2 次尝试..."));
    delay(WIFI_CONNECT_RETRY_DELAY_MS);
    
    // 第 2 次连接尝试
    if (tryConnectWiFi(savedSSID, savedPassword, WIFI_CONNECT_TIMEOUT_MS)) {
        DEBUG_PRINTLN(F("[WiFi] 第 2 次连接成功，关闭 AP 进入纯 STA"));
        wifiConnected = true;
        wifiState = MODE_STA_ONLY;
        return;
    }
    
    DEBUG_PRINTLN(F("[WiFi] 2 次连接均失败，启动 AP_STA 模式"));
    // 2 次都失败 → 启动 AP_STA（AP 开放配网 + STA 可扫描）
    startAccessPoint();
    wifiState = MODE_AP_ONLY;
}

// ===================== 启动 AP 热点 =====================
void startAccessPoint() {
    DEBUG_PRINTLN(F("[WiFi] 启动 AP_STA 模式..."));
    
    // 默认启动 AP_STA，AP 保持开放供配置，STA 可执行扫描
    WiFi.mode(WIFI_AP_STA);
    delay(50);
    
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );
    
    bool result = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CLIENTS);
    
    if (result) {
        DEBUG_PRINTF("[WiFi] AP 已启动: %s\n", AP_SSID);
        DEBUG_PRINTF("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    } else {
        DEBUG_PRINTLN(F("[WiFi] AP 启动失败!"));
    }
}
// ===================== 连接 WiFi =====================
void connectToWiFi(const String& ssid, const String& password) {
    DEBUG_PRINTF("[WiFi] 正在连接: %s\n", ssid.c_str());
    
    WiFi.mode(WIFI_STA);  // 纯 STA 模式
    WiFi.begin(ssid.c_str(), password.c_str());
}

// ===================== 尝试连接 WiFi（带超时）=====================
bool tryConnectWiFi(const String& ssid, const String& password, unsigned long timeoutMs) {
    connectToWiFi(ssid, password);
    
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        delay(100);
    }
    return false;
}

// ===================== 检查 WiFi 连接 =====================
void checkWiFiConnection() {
    if (wifiState != MODE_STA_ONLY) return;
    
    unsigned long now = millis();
    if (now - lastWifiCheck < WIFI_CHECK_INTERVAL) return;
    lastWifiCheck = now;
    
    if (WiFi.status() != WL_CONNECTED) {
        if (wifiConnected) {
            DEBUG_PRINTLN(F("[WiFi] STA 连接断开，回退到 AP 模式"));
            wifiConnected = false;
        }
        
        // STA 断线 → 回退 AP 模式
        WiFi.mode(WIFI_OFF);
        delay(100);
        startAccessPoint();
        wifiState = MODE_AP_ONLY;
        powerLevel = PWR_NORMAL;
        
        DEBUG_PRINTLN(F("[WiFi] 已切换到 AP 模式"));
    }
}

// ===================== Web 服务器初始化 =====================
void setupWebServer() {
    DEBUG_PRINTLN(F("[初始化] Web 服务器..."));
    
    // 主页 - 返回配置页面
    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", INDEX_HTML);
        lastHttpRequest = millis();
    });
    
    server.on("/wifi", HTTP_GET, []() {
        server.send(200, "text/html", INDEX_HTML);
        lastHttpRequest = millis();
    });
    
    // WiFi 配置提交
    server.on("/connect", HTTP_POST, handleWiFiConfig);
    
    // WiFi 扫描
    server.on("/scan", HTTP_GET, handleWiFiScan);
    
    // WiFi 断开
    server.on("/disconnect", HTTP_POST, handleDisconnectWiFi);
    
    // 图片上传
    server.on("/upload", HTTP_POST, []() {
        // 统一发送一次响应（结果由 handleUpload 在 UPLOAD_FILE_END 记录，默认 500 兜底）
        if (uploadRespCode == 0) {
            uploadRespCode = 500;
            uploadRespMsg = "Error: Upload failed";
        }
        server.send(uploadRespCode, "text/plain", uploadRespMsg);
        uploadRespCode = 0;
        
        // 响应发送后再刷新屏幕，避免阻塞 HTTP 导致前端超时
        if (uploadRefreshPending) {
            uploadRefreshPending = false;
            DEBUG_PRINTLN(F("[显示] 开始刷新屏幕..."));
            displayImageFromBuffer(imageBuffer, uploadSavedLen);
            DEBUG_PRINTLN(F("[显示] 屏幕刷新完成"));
        }
    }, handleUpload);
    
    // 切换到实时模式
    server.on("/switchToRealTime", HTTP_POST, handleSwitchRealTime);
    
    // 切换到幻灯片模式
    server.on("/switchToSlideShow", HTTP_POST, handleSwitchSlideShow);
    
    // 获取设备状态
    server.on("/status", HTTP_GET, handleGetStatus);
    
    server.begin();
    DEBUG_PRINTF("[Web] 服务器已启动，端口: %d\n", WEB_SERVER_PORT);
}

// ===================== WiFi 配置处理 =====================
void handleWiFiConfig() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    DEBUG_PRINTF("[Web] 收到 WiFi 配置: SSID=%s\n", ssid.c_str());
    lastHttpRequest = millis();
    
    if (ssid.length() == 0) {
        server.send(400, "text/plain", "Error: SSID is required");
        return;
    }
    
    // 保存配置
    saveWiFiConfig(ssid, password);
    
    server.send(200, "text/plain", "WiFi配置已保存，设备即将重启");
    delay(1000);
    
    // 重启设备以应用新配置
    ESP.restart();
}

// ===================== 图片上传处理 =====================
void handleUpload() {
    lastHttpRequest = millis();
    
    static size_t uploadOffset = 0;
    static bool uploadError = false;
    HTTPUpload& upload = server.upload();
    
    if (upload.status == UPLOAD_FILE_START) {
        DEBUG_PRINTLN(F("[Web] 收到图片上传请求"));
        
        // 确保退出 Modem Sleep，给射频恢复留时间（只在 START 执行一次，不再每个数据块 delay）
        enterNormalMode();
        delay(20);  // 20ms 让射频从睡眠恢复
        
        uploadInProgress = true;
        uploadOffset = 0;
        uploadError = false;
        uploadRespCode = 0;
        uploadRespMsg = "";
        uploadRefreshPending = false;
        
        DEBUG_PRINTF("[上传] 文件名: %s\n", upload.filename.c_str());
        
        // 清空缓冲区
        if (imageBuffer) {
            memset(imageBuffer, 0, IMAGE_DATA_SIZE);
        }
        
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        DEBUG_PRINTF("[上传] 接收数据块: %d bytes\n", upload.currentSize);
        
        // 将数据存入缓冲区；放不进缓冲区则标记超尺寸错误，丢弃后续数据块
        if (!uploadError) {
            if (imageBuffer && (uploadOffset + upload.currentSize) <= IMAGE_DATA_SIZE) {
                memcpy(imageBuffer + uploadOffset, upload.buf, upload.currentSize);
                uploadOffset += upload.currentSize;
            } else {
                uploadError = true;
                DEBUG_PRINTF("[上传] 数据超出缓冲区 (%d + %d > %d)，标记为超尺寸\n",
                             (int)uploadOffset, (int)upload.currentSize, IMAGE_DATA_SIZE);
            }
        }
        
    } else if (upload.status == UPLOAD_FILE_END) {
        uploadInProgress = false;
        DEBUG_PRINTF("[上传] 上传完成, 总大小: %d bytes\n", (int)uploadOffset);
        
        if (uploadError) {
            // 数据超过缓冲区上限，不保存
            uploadRespCode = 413;
            uploadRespMsg = "Error: Image too large";
        } else if (uploadOffset == 0) {
            uploadRespCode = 400;
            uploadRespMsg = "Error: Empty upload";
        } else if (!ensureSpaceForImage(uploadOffset)) {
            // 淘汰所有旧图后空间仍不足
            uploadRespCode = 500;
            uploadRespMsg = "Error: Not enough storage space";
        } else if (saveImageData(imageBuffer, uploadOffset, totalSlides)) {
            totalSlides++;
            updateSlideIndex();
            
            // 记录成功响应，由 /upload 完成处理器统一发送一次
            uploadRespCode = 200;
            uploadRespMsg = "图片上传成功";
            DEBUG_PRINTF("[上传] 图片已保存 (共 %d 张)\n", totalSlides);
            
            // 屏幕刷新推迟到响应发送之后执行（uploadRefreshPending），避免阻塞 HTTP 导致前端超时
            if (currentDisplayMode == MODE_REALTIME) {
                uploadSavedLen = uploadOffset;
                uploadRefreshPending = true;
            }
        } else {
            uploadRespCode = 500;
            uploadRespMsg = "Error: Failed to save image";
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        // 客户端中止上传：复位状态，避免 uploadInProgress 卡死省电管理
        uploadInProgress = false;
        uploadOffset = 0;
        uploadError = false;
        uploadRefreshPending = false;
        DEBUG_PRINTLN(F("[上传] 上传被客户端中止，状态已复位"));
    }
}

// ===================== 切换实时模式 =====================
void handleSwitchRealTime() {
    DEBUG_PRINTLN(F("[Web] 切换到实时模式"));
    
    currentDisplayMode = MODE_REALTIME;
    slideShowActive = false;
    
    // 显示最近上传的图片
    if (totalSlides > 0 && imageBuffer) {
        String path = getImagePath(totalSlides - 1);
        File f = LittleFS.open(path, "r");
        if (f) {
            size_t len = f.read(imageBuffer, IMAGE_DATA_SIZE);
            f.close();
            displayImageFromBuffer(imageBuffer, len);
        }
    }
    
    server.send(200, "text/plain", "切换到实时模式成功");
}

// ===================== 切换幻灯片模式 =====================
void handleSwitchSlideShow() {
    DEBUG_PRINTLN(F("[Web] 切换到幻灯片模式"));
    
    if (totalSlides == 0) {
        server.send(400, "text/plain", "Error: No images stored");
        return;
    }
    
    currentDisplayMode = MODE_SLIDESHOW;
    slideShowActive = true;
    currentSlideIndex = 0;
    lastSlideChange = millis();
    
    // 立即显示第一张
    displayImageFromFile(getImagePath(0));
    
    server.send(200, "text/plain", "切换到轮播模式成功");
}

// ===================== 获取设备状态 =====================
void handleGetStatus() {
    StaticJsonDocument<1024> doc;
    
    doc["mode"] = (currentDisplayMode == MODE_REALTIME) ? "realtime" : "slideshow";
    doc["wifi_connected"] = wifiConnected;
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["sta_ip"] = WiFi.localIP().toString();
    doc["sta_ssid"] = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "";
    doc["sta_rssi"] = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
    doc["image_count"] = totalSlides;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["free_psram"] = ESP.getFreePsram();
    doc["uptime_seconds"] = (millis() - bootTime) / 1000;
    doc["power_mode"] = (powerLevel == PWR_NORMAL) ? "normal" : "modem_sleep";
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
    lastHttpRequest = millis();
}

// ===================== 幻灯片轮播逻辑 =====================
void handleSlideShow() {
    if (totalSlides <= 1) return;
    
    unsigned long now = millis();
    if (now - lastSlideChange < slideInterval) return;
    lastSlideChange = now;
    
    // 切换到下一张
    currentSlideIndex = (currentSlideIndex + 1) % totalSlides;
    String path = getImagePath(currentSlideIndex);
    
    DEBUG_PRINTF("[幻灯片] 显示第 %d/%d 张: %s\n", 
                 currentSlideIndex + 1, totalSlides, path.c_str());
    
    displayImageFromFile(path);
}

// ===================== 从文件显示图片 =====================
void displayImageFromFile(const String& path) {
    File f = LittleFS.open(path, "r");
    if (!f) {
        DEBUG_PRINTF("[错误] 无法打开文件: %s\n", path.c_str());
        return;
    }
    
    size_t len = f.size();
    if (len > IMAGE_DATA_SIZE) len = IMAGE_DATA_SIZE;
    
    if (imageBuffer) {
        f.read(imageBuffer, len);
        displayImageFromBuffer(imageBuffer, len);
    }
    
    f.close();
}

// ===================== 从缓冲区显示图片 =====================
void displayImageFromBuffer(uint8_t* data, uint32_t len) {
    if (!data || len == 0) {
        DEBUG_PRINTLN(F("[错误] 无效的图像数据"));
        return;
    }
    
    DEBUG_PRINTF("[显示] 刷新图像: %d bytes\n", len);
    
    enterNormalMode();  // 刷新前退出省电
    displayRefreshing = true;
    
    // 调用驱动刷新屏幕
    epd.display(data, len);
    
    displayRefreshing = false;
}

// ===================== 保存图片数据 =====================
bool saveImageData(const uint8_t* data, size_t len, int index) {
    String path = getImagePath(index);
    
    File f = LittleFS.open(path, "w");
    if (!f) {
        DEBUG_PRINTF("[错误] 无法创建文件: %s\n", path.c_str());
        return false;
    }
    
    size_t written = f.write(data, len);
    f.close();
    
    if (written != len) {
        DEBUG_PRINTF("[错误] 写入不完整: %d/%d，删除残留文件\n", (int)written, (int)len);
        LittleFS.remove(path);  // 删除残留半截文件，避免占用空间堵死后续上传
        return false;
    }
    
    return true;
}

// ===================== 淘汰最旧图片 (FIFO) =====================
// 删除 img_0.bin，并将 img_i.bin 重命名为 img_{i-1}.bin (i=1..totalSlides-1)，
// totalSlides-- 并更新索引文件。
void evictOldestImage() {
    if (totalSlides <= 0) return;
    
    String oldest = getImagePath(0);
    DEBUG_PRINTF("[存储] 淘汰最旧图片: %s\n", oldest.c_str());
    LittleFS.remove(oldest);
    
    // 重排索引：img_i.bin -> img_{i-1}.bin
    for (int i = 1; i < totalSlides; i++) {
        LittleFS.rename(getImagePath(i), getImagePath(i - 1));
    }
    
    totalSlides--;
    if (currentSlideIndex >= totalSlides) {
        currentSlideIndex = 0;  // 防止轮播索引越界
    }
    updateSlideIndex();
}

// ===================== 确保有足够空间写入图片 =====================
// 剩余空间不足（预留 8KB 给索引/元数据）时按 FIFO 淘汰最旧图片，
// 循环直到空间足够或无图可删；同时执行 MAX_IMAGES 上限。
// 返回 true 表示可以写入，false 表示无法腾出足够空间。
bool ensureSpaceForImage(size_t len) {
    const size_t RESERVE_BYTES = 8 * 1024;  // 索引文件和元数据余量
    
    // 执行 MAX_IMAGES 上限：超限先淘汰最旧
    while (totalSlides >= MAX_IMAGES && totalSlides > 0) {
        evictOldestImage();
    }
    
    // 空间不足时循环淘汰最旧图片
    while (true) {
        size_t freeBytes = LittleFS.totalBytes() - LittleFS.usedBytes();
        if (freeBytes >= len + RESERVE_BYTES) {
            return true;
        }
        if (totalSlides <= 0) {
            DEBUG_PRINTF("[存储] 空间不足: 需要 %d 字节, 可用 %d 字节, 且无旧图可淘汰\n",
                         (int)(len + RESERVE_BYTES), (int)freeBytes);
            return false;
        }
        DEBUG_PRINTF("[存储] 可用空间 %d 字节 < 需要 %d 字节，淘汰最旧图片\n",
                     (int)freeBytes, (int)(len + RESERVE_BYTES));
        evictOldestImage();
    }
}

// ===================== 获取图片路径 =====================
String getImagePath(int index) {
    String path = IMAGE_PREFIX;
    path += String(index);
    path += ".bin";
    return path;
}

// ===================== 更新图片索引 =====================
void updateSlideIndex() {
    // 将图片数量保存到索引文件
    File f = LittleFS.open(IMAGE_INDEX_FILE, "w");
    if (f) {
        StaticJsonDocument<256> doc;
        doc["count"] = totalSlides;
        doc["interval"] = slideInterval;
        serializeJson(doc, f);
        f.close();
    }
}

// ===================== 获取图片数量 =====================
int getImageCount() {
    // 从索引文件读取
    if (LittleFS.exists(IMAGE_INDEX_FILE)) {
        File f = LittleFS.open(IMAGE_INDEX_FILE, "r");
        if (f) {
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, f);
            f.close();
            if (!error) {
                slideInterval = doc["interval"] | SLIDE_SHOW_INTERVAL;
                return doc["count"] | 0;
            }
            // 解析失败：不信任索引中的 count，回退到目录扫描，避免下次上传覆盖好图
            DEBUG_PRINTF("[文件系统] 索引文件解析失败: %s，回退到目录扫描\n", error.c_str());
        }
    }
    
    // 如果没有索引文件，扫描目录
    int count = 0;
    File root = LittleFS.open(IMAGE_DIR);
    if (!root) return 0;
    
    File file = root.openNextFile();
    while (file) {
        String name = file.name();
        if (name.endsWith(".bin")) count++;
        file = root.openNextFile();
    }
    
    return count;
}

// ===================== WiFi 扫描 =====================
void handleWiFiScan() {
    lastHttpRequest = millis();
    DEBUG_PRINTLN(F("[WiFiScan] 收到扫描请求"));
    
    // 设备默认 AP_STA 模式，无需切换即可扫描
    wifi_mode_t currentMode = WiFi.getMode();
    DEBUG_PRINTF("[WiFiScan] 当前模式: %d (OFF=0, STA=1, AP=2, AP_STA=3)\n", currentMode);
    
    // 清理旧扫描结果
    WiFi.scanDelete();
    delay(50);
    
    // 启动异步扫描：每个通道 150ms，全通道扫描，被动扫描
    DEBUG_PRINTLN(F("[WiFiScan] 启动扫描 (每通道 150ms)..."));
    int16_t scanStatus = WiFi.scanNetworks(true, false, false, 150, 0);
    DEBUG_PRINTF("[WiFiScan] scanNetworks 返回值: %d (RUNNING=-1, FAILED=-2)\n", scanStatus);
    
    if (scanStatus == WIFI_SCAN_FAILED) {
        DEBUG_PRINTLN(F("[WiFiScan] 扫描启动失败！"));
        WiFi.scanDelete();
        server.send(500, "application/json", "{\"status\":\"failed\",\"error\":\"scan start failed\",\"networks\":[]}");
        return;
    }
    
    // 等待扫描完成（最多等 10 秒）
    unsigned long start = millis();
    int16_t scanResult;
    int waitLoops = 0;
    while ((scanResult = WiFi.scanComplete()) < 0 && millis() - start < 10000) {
        delay(50);
        waitLoops++;
        server.handleClient(); // 保持 WebServer 响应其他请求
    }
    
    unsigned long elapsed = millis() - start;
    DEBUG_PRINTF("[WiFiScan] 等待结束: 耗时=%lu ms, 轮询次数=%d, scanComplete=%d\n", elapsed, waitLoops, scanResult);
    
    // 扫描超时
    if (scanResult < 0) {
        DEBUG_PRINTLN(F("[WiFiScan] 扫描超时！"));
        WiFi.scanDelete();
        server.send(200, "application/json", "{\"status\":\"timeout\",\"error\":\"scan timeout\",\"networks\":[]}");
        return;
    }
    
    // 扫描成功
    int n = scanResult;
    DEBUG_PRINTF("[WiFiScan] 扫描完成，发现 %d 个网络\n", n);
    
    StaticJsonDocument<4096> doc;
    JsonArray arr = doc.createNestedArray("networks");
    for (int i = 0; i < n; i++) {
        JsonObject net = arr.createNestedObject();
        String ssid = WiFi.SSID(i);
        int rssi = WiFi.RSSI(i);
        int ch = WiFi.channel(i);
        bool open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        net["ssid"] = ssid;
        net["rssi"] = rssi;
        net["channel"] = ch;
        net["open"] = open;
        DEBUG_PRINTF("[WiFiScan]  #%02d: SSID=%s, RSSI=%d dBm, CH=%d, Open=%d\n", 
                     i, ssid.c_str(), rssi, ch, open ? 1 : 0);
    }
    doc["count"] = n;
    doc["status"] = "ok";
    doc["elapsed_ms"] = (int)elapsed;
    
    WiFi.scanDelete();
    
    String response;
    serializeJson(doc, response);
    DEBUG_PRINTF("[WiFiScan] 响应 JSON 大小: %d bytes\n", response.length());
    server.send(200, "application/json", response);
}

// ===================== WiFi 断开 =====================
void handleDisconnectWiFi() {
    lastHttpRequest = millis();
    DEBUG_PRINTLN(F("[Web] 断开 WiFi 请求"));
    
    // 清除保存的配置
    if (LittleFS.exists(WIFI_CONFIG_FILE)) {
        LittleFS.remove(WIFI_CONFIG_FILE);
    }
    
    server.send(200, "text/plain", "WiFi配置已清除，设备即将重启");
    delay(1000);
    ESP.restart();
}

// ===================== 省电管理 =====================
void handlePowerManagement() {
    // 仅在 STA 模式下启用省电
    if (wifiState != MODE_STA_ONLY) return;
    
    unsigned long now = millis();
    
    // 如果正在上传或刷新，保持正常模式
    if (uploadInProgress || displayRefreshing) {
        if (powerLevel != PWR_NORMAL) {
            enterNormalMode();
        }
        return;
    }
    
    // 检查是否需要进入 Modem Sleep
    if (powerLevel == PWR_NORMAL && (now - lastHttpRequest > PWR_IDLE_TO_MODEM_MS)) {
        enterModemSleep();
    }
}

// ===================== 进入正常模式 =====================
void enterNormalMode() {
    if (powerLevel == PWR_NORMAL) return;
    
    DEBUG_PRINTLN(F("[省电] 退出 Modem Sleep，进入正常模式"));
    WiFi.setSleep(false);
    powerLevel = PWR_NORMAL;
}

// ===================== 进入 Modem Sleep =====================
void enterModemSleep() {
    if (powerLevel == PWR_MODEM_SLEEP) return;
    
    DEBUG_PRINTLN(F("[省电] 启用 Modem Sleep"));
    WiFi.setSleep(true);
    powerLevel = PWR_MODEM_SLEEP;
}

// ===================== 加载 WiFi 配置 =====================
void loadWiFiConfig(String& ssid, String& password) {
    ssid = "";
    password = "";
    
    if (!LittleFS.exists(WIFI_CONFIG_FILE)) return;
    
    File f = LittleFS.open(WIFI_CONFIG_FILE, "r");
    if (!f) return;
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, f);
    f.close();
    
    if (error) {
        DEBUG_PRINTF("[WiFi] 配置解析错误: %s\n", error.c_str());
        return;
    }
    
    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
}

// ===================== 保存 WiFi 配置 =====================
void saveWiFiConfig(const String& ssid, const String& password) {
    File f = LittleFS.open(WIFI_CONFIG_FILE, "w");
    if (!f) {
        DEBUG_PRINTLN(F("[错误] 无法保存 WiFi 配置"));
        return;
    }
    
    StaticJsonDocument<512> doc;
    doc["ssid"] = ssid;
    doc["password"] = password;
    
    serializeJson(doc, f);
    f.close();
    
    DEBUG_PRINTLN(F("[WiFi] 配置已保存"));
}
