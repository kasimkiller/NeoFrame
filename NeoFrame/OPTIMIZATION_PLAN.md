# NeoFrame 固件优化方案 v2.0

## 概述

本方案针对 ESP32-S3-WROOM-1 + GDEP133C02 六色电子画框固件进行三项核心优化：
1. **智能 WiFi 连接管理** — AP/STA 自动切换 + WiFi 扫描配置
2. **动态后台界面** — 根据连接状态自适应显示
3. **多级省电策略** — 保持 HTTP 可访问的前提下降功耗

---

## 一、WiFi 连接逻辑方案

### 1.1 设计目标

| 场景 | 行为 |
|------|------|
| 首次开机/无保存配置 | 开启 AP 模式，提供配网界面 |
| 有保存配置 | 自动尝试连接 WiFi（最多 2 次），成功则关闭 AP |
| 连接失败 2 次后 | 始终开启 AP 模式，等待用户配网 |
| AP 模式下用户配置 WiFi | 保存配置，重启后自动连接 |
| STA 模式下 WiFi 断开 | 检测到断线后，自动切回 AP 模式 |

### 1.2 状态机设计

```
                    ┌─────────────────┐
     首次开机        │    BOOT_INIT    │
    (无保存配置)     └────────┬────────┘
                              │ 读取 wifi_config.json
              ┌───────────────┼───────────────┐
              │ 无配置/读取失败│               │ 有配置
              ▼                               ▼
    ┌─────────────────┐            ┌─────────────────┐
    │   MODE_AP_ONLY  │            │  TRY_CONNECT_1  │
    │  直接开AP配网    │            │ 第1次连接尝试   │
    └─────────────────┘            └────────┬────────┘
                                            │ 连接结果
                          ┌─────────────────┼─────────────────┐
                          │ 成功 (WL_CONNECTED)               │ 失败
                          ▼                                   ▼
                ┌─────────────────┐                  ┌─────────────────┐
                │   MODE_STA_ONLY │                  │  TRY_CONNECT_2  │
                │  纯STA模式运行   │                  │ 第2次连接尝试   │
                │  AP已关闭        │                  └────────┬────────┘
                └─────────────────┘                           │ 连接结果
                                                              │
                                    ┌─────────────────────────┼─────────────────┐
                                    │ 成功                    │ 失败(2次都失败)
                                    ▼                         ▼
                          ┌─────────────────┐      ┌─────────────────┐
                          │   MODE_STA_ONLY │      │   MODE_AP_ONLY  │
                          └─────────────────┘      │  始终AP模式运行  │
                                                   └─────────────────┘
```

### 1.3 核心逻辑流程（setup 阶段）

```cpp
void setupWiFi() {
    // 1. 读取保存的配置
    String ssid, password;
    loadWiFiConfig(ssid, password);
    
    if (ssid.length() == 0) {
        // 无配置 → 直接进入 AP 模式
        startAccessPoint();
        wifiState = MODE_AP_ONLY;
        return;
    }
    
    // 2. 第 1 次连接尝试
    if (tryConnectWiFi(ssid, password, 15000)) {  // 15秒超时
        wifiState = MODE_STA_ONLY;
        return;
    }
    
    // 3. 第 2 次连接尝试（等待 3 秒后重试）
    delay(3000);
    if (tryConnectWiFi(ssid, password, 15000)) {
        wifiState = MODE_STA_ONLY;
        return;
    }
    
    // 4. 2 次都失败 → 始终 AP 模式
    startAccessPoint();
    wifiState = MODE_AP_ONLY;
}
```

### 1.4 STA 模式下断线回退 AP

```cpp
// loop() 中周期性检查
void checkWiFiConnection() {
    if (wifiState != MODE_STA_ONLY) return;
    
    unsigned long now = millis();
    if (now - lastWifiCheck < WIFI_CHECK_INTERVAL) return;
    lastWifiCheck = now;
    
    if (WiFi.status() != WL_CONNECTED) {
        // STA 断线，回退到 AP 模式
        WiFi.mode(WIFI_OFF);
        delay(100);
        startAccessPoint();
        wifiState = MODE_AP_ONLY;
    }
}
```

### 1.5 AP 模式下的 WiFi 扫描

**关键发现**：ESP32 在 AP 模式下可以扫描周围 WiFi，但需要使用异步扫描避免阻塞 WebServer。

```cpp
// 新增 API: GET /scan → 返回可用 WiFi 列表
void handleWiFiScan() {
    // 启动异步扫描
    WiFi.scanNetworks(true);
    
    // 等待结果（轮询，不阻塞）
    int n = WiFi.scanComplete();
    if (n < 0) {
        server.send(202, "application/json", "{\"status\":\"scanning\"}");
        return;
    }
    
    // 组装 JSON 响应
    StaticJsonDocument<2048> doc;
    JsonArray arr = doc.createNestedArray("networks");
    for (int i = 0; i < n; i++) {
        JsonObject net = arr.createNestedObject();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["channel"] = WiFi.channel(i);
        net["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
    }
    doc["count"] = n;
    
    WiFi.scanDelete();  // 清理扫描结果
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}
```

**前端交互**：
- 页面加载后调用 `/scan` 获取 WiFi 列表
- 如果返回 `"status":"scanning"`，前端 2 秒后重试
- 用户选择 SSID + 输入密码 → POST `/connect`

---

## 二、后台界面动态化方案

### 2.1 当前问题

当前 `WebPage.h` 是静态 HTML，所有内容在编译时固化：
- AP 模式下显示 WiFi 配置表单
- 但无法显示当前时间、运行时间、IP 地址等动态信息
- 已连接 WiFi 后仍显示"Connect to WiFi"表单

### 2.2 解决方案：JavaScript 动态渲染

页面结构不变，通过 JS 调用 `/status` API 动态更新内容。

#### 2.2.1 `/status` API 扩展

```json
{
  "mode": "sta",
  "wifi_connected": true,
  "ap_ip": "",
  "sta_ip": "192.168.1.105",
  "sta_ssid": "MyHomeWiFi",
  "sta_rssi": -52,
  "uptime_seconds": 86400,
  "image_count": 3,
  "free_heap": 123456,
  "free_psram": 8388608,
  "current_time": "2025-07-22 14:30:00",
  "power_mode": "modem_sleep"
}
```

#### 2.2.2 前端自适应逻辑

```javascript
// 页面加载时获取状态
async function loadStatus() {
    const resp = await fetch('/status');
    const data = await resp.json();
    
    const wifiSection = document.getElementById('wifi-section');
    
    if (data.wifi_connected) {
        // 已连接 → 显示连接信息
        wifiSection.innerHTML = `
            <h2>WiFi 已连接</h2>
            <div class="info-grid">
                <label>SSID:</label><span>${data.sta_ssid}</span>
                <label>信号强度:</label><span>${data.sta_rssi} dBm</span>
                <label>IP 地址:</label><span>${data.sta_ip}</span>
                <label>当前时间:</label><span>${data.current_time}</span>
                <label>运行时间:</label><span>${formatUptime(data.uptime_seconds)}</span>
                <label>省电模式:</label><span>${data.power_mode}</span>
            </div>
            <button onclick="disconnectWiFi()">断开并重新配网</button>
        `;
    } else {
        // 未连接 → 显示 WiFi 配置表单 + 扫描列表
        wifiSection.innerHTML = `
            <h2>home WiFi network</h2>
            <div id="wifi-list">正在扫描 WiFi...</div>
            <form action="/connect" method="POST" onsubmit="return handleConnect(event)">
                <label>WiFi name(SSID):</label>
                <input type="text" id="ssid" name="ssid" required>
                <label>WiFi password:</label>
                <input type="password" id="password" name="password">
                <button type="submit">Connect to WiFi</button>
            </form>
        `;
        scanWiFiNetworks();  // 调用 /scan 获取列表
    }
}
```

### 2.3 WiFi 扫描前端交互

```javascript
async function scanWiFiNetworks() {
    const listDiv = document.getElementById('wifi-list');
    listDiv.innerHTML = '<p>正在扫描周围 WiFi...</p>';
    
    try {
        const resp = await fetch('/scan');
        const data = await resp.json();
        
        if (data.status === 'scanning') {
            setTimeout(scanWiFiNetworks, 2000);  // 2秒后重试
            return;
        }
        
        let html = '<div class="wifi-list"><h3>可用 WiFi 网络:</h3>';
        data.networks.forEach((net, i) => {
            const lock = net.open ? '🔓' : '🔒';
            const bars = net.rssi > -50 ? '▂▄▆█' : net.rssi > -65 ? '▂▄▆_' : '▂▄__';
            html += `<div class="wifi-item" onclick="selectSSID('${net.ssid}')">
                ${lock} ${net.ssid} <span class="rssi">${bars} ${net.rssi}dBm</span>
            </div>`;
        });
        html += '</div>';
        listDiv.innerHTML = html;
    } catch (e) {
        listDiv.innerHTML = '<p style="color:#ff6b6b">扫描失败，请手动输入 SSID</p>';
    }
}

function selectSSID(ssid) {
    document.getElementById('ssid').value = ssid;
}
```

### 2.4 连接结果处理

用户提交表单后，前端改为 AJAX 提交（不刷新页面），显示连接进度：

```javascript
async function handleConnect(event) {
    event.preventDefault();
    const ssid = document.getElementById('ssid').value;
    const password = document.getElementById('password').value;
    const btn = event.target.querySelector('button');
    
    btn.textContent = '正在连接...';
    btn.disabled = true;
    
    try {
        const resp = await fetch('/connect', {
            method: 'POST',
            headers: {'Content-Type': 'application/x-www-form-urlencoded'},
            body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
        });
        const text = await resp.text();
        
        if (text.includes('连接成功')) {
            alert('WiFi 连接成功！设备将重启。\n重启后请通过 WiFi IP 访问后台。');
            // 3秒后刷新页面
            setTimeout(() => location.reload(), 3000);
        } else {
            alert('连接失败: ' + text);
            btn.textContent = 'Connect to WiFi';
            btn.disabled = false;
        }
    } catch (e) {
        alert('请求失败');
        btn.textContent = 'Connect to WiFi';
        btn.disabled = false;
    }
}
```

---

## 三、省电模式方案

### 3.1 核心约束分析

| 约束 | 影响 |
|------|------|
| 必须保持 HTTP 可访问 | 不能进入 Deep Sleep |
| AP 模式功耗高 | AP 模式下无法使用 Modem Sleep |
| STA 模式可省电 | STA 模式支持 Modem Sleep / Light Sleep |
| 墨水屏刷新后无需持续驱动 | 刷新完成后屏幕保持，驱动可休眠 |

### 3.2 推荐策略：三级自适应省电

```
┌─────────────────────────────────────────────────────────────────┐
│                      省电策略层级                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Level 0: 正常工作模式 (NO_SAVE)                        │   │
│  │  ├─ CPU: 240MHz 全速运行                                 │   │
│  │  ├─ WiFi: 无省电, PS=NONE                                │   │
│  │  ├─ 功耗: ~80-150mA                                     │   │
│  │  └─ 触发: 图片上传/刷新中、AP 模式                       │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              ↓                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Level 1: Modem Sleep (STA 模式下默认)                  │   │
│  │  ├─ CPU: 240MHz 运行                                     │   │
│  │  ├─ WiFi: DTIM=3 Modem Sleep                             │   │
│  │  ├─ 功耗: ~3-5mA (WiFi 射频周期性休眠)                   │   │
│  │  └─ 触发: STA 模式 + 无活跃操作                          │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              ↓                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Level 2: Light Sleep (STA 模式下长时间空闲)             │   │
│  │  ├─ CPU: 暂停，GPIO/RTC 唤醒                             │   │
│  │  ├─ WiFi: DTIM=10 保持连接                               │   │
│  │  ├─ 功耗: ~1.5-2mA                                       │   │
│  │  └─ 触发: STA 模式 + 10分钟无 HTTP 请求                  │   │
│  │     ├─ WebServer 请求 → 自动唤醒                         │   │
│  │     ├─ 定时器 30s 唤醒检查状态                           │   │
│  │     └─ 幻灯片切换时间到 → 唤醒刷新                       │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 3.3 各层级详细设计

#### Level 0: 正常工作模式

```cpp
void enterNormalMode() {
    // 上传图片或刷新屏幕时调用
    WiFi.setSleep(false);  // 关闭 WiFi 省电，确保传输速度
    cpuFreq = 240;         // 全速运行
    powerLevel = PWR_NORMAL;
}
```

#### Level 1: Modem Sleep（STA 默认）

```cpp
void enterModemSleep() {
    // STA 模式下的默认状态
    WiFi.setSleep(true);   // 启用 WiFi 省电
    // Arduino-ESP32 内部使用 WIFI_PS_MIN_MODEM
    // 自动在 DTIM 间隔休眠射频
    powerLevel = PWR_MODEM_SLEEP;
}
```

**技术细节**：
- `WiFi.setSleep(true)` 在 Arduino-ESP32 v3.x 中启用 `WIFI_PS_MIN_MODEM`
- ESP32 在连接的 AP 的 DTIM 间隔时关闭射频，自动唤醒接收数据
- HTTP 请求到来时自动唤醒，响应延迟增加约 10-50ms（可接受）
- 需确保路由器的 DTIM 间隔不太长（建议 1-3）

#### Level 2: Light Sleep（深度空闲）

```cpp
void enterLightSleep() {
    // 长时间无操作后调用
    
    // 1. 配置 GPIO 唤醒（WebServer 请求会通过 WiFi 唤醒）
    // 实际上 WiFi 数据包会自动唤醒 CPU，无需额外配置
    
    // 2. 配置定时器唤醒（用于幻灯片轮播检查）
    esp_sleep_enable_timer_wakeup(30 * 1000 * 1000);  // 30秒
    
    // 3. 进入 Light Sleep
    esp_light_sleep_start();
    
    // 4. 唤醒后自动继续执行
    // 检查唤醒原因，决定是否需要刷新屏幕
}
```

**关键实现**：

```cpp
// loop() 中的省电管理
void handlePowerManagement() {
    static unsigned long lastActivity = 0;
    static unsigned long lastLightSleep = 0;
    
    unsigned long now = millis();
    
    // 有 HTTP 请求时记录活动时间
    if (httpActivity) {
        lastActivity = now;
        httpActivity = false;
        if (powerLevel > PWR_NORMAL) {
            enterModemSleep();  // 回到 Modem Sleep
        }
    }
    
    // STA 模式下，空闲 10 分钟后尝试 Light Sleep
    if (wifiState == MODE_STA_ONLY && 
        powerLevel == PWR_MODEM_SLEEP &&
        now - lastActivity > 10 * 60 * 1000) {  // 10分钟
        
        // 检查是否需要刷新幻灯片
        if (currentDisplayMode == MODE_SLIDESHOW) {
            unsigned long timeToNextSlide = slideInterval - (now - lastSlideChange);
            if (timeToNextSlide <= 30 * 1000) {
                // 下一张幻灯片在 30 秒内，只 sleep 到那个时间点
                esp_sleep_enable_timer_wakeup(timeToNextSlide * 1000);
                esp_light_sleep_start();
                // 唤醒后检查是否需要刷新
                handleSlideShow();
            }
        }
        
        // 实时模式下，进入 Light Sleep，30 秒定时唤醒检查
        if (currentDisplayMode == MODE_REALTIME) {
            enterLightSleep();
        }
    }
}
```

### 3.4 AP 模式下的功耗优化

AP 模式无法使用 WiFi 省电（必须持续发送 beacon），但可以：

```cpp
void optimizeAPMode() {
    // 1. 降低 beacon 间隔（增加间隔 = 减少射频发射）
    // 默认值 100ms → 改为 200ms
    // 需底层 API: esp_wifi_set_config() 修改 beacon_interval
    
    // 2. 降低 CPU 频率到 80MHz
    setCpuFrequencyMhz(80);
    
    // 3. 限制 AP 最大连接数
    WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, 2);  // 最多2客户端
    
    // 功耗预估: ~60-100mA（仍较高，但比 240MHz 全速低）
}
```

### 3.5 功耗对比预估

| 模式 | 策略 | 预估电流 | 相对节电 |
|------|------|---------|---------|
| 当前固件 | AP_STA 双开 + 240MHz | ~120-200mA | — |
| STA + Modem Sleep | DTIM=3 | ~3-5mA | **95% ↓** |
| STA + Light Sleep | DTIM=10 | ~1.5-2mA | **97% ↓** |
| AP 模式优化 | 80MHz + beacon 200ms | ~60-100mA | 40% ↓ |

### 3.6 状态/模式切换触发条件

```cpp
enum PowerLevel {
    PWR_NORMAL = 0,      // 全速: 上传/刷新中
    PWR_MODEM_SLEEP = 1, // STA 默认: 射频周期性休眠
    PWR_LIGHT_SLEEP = 2  // 深度空闲: CPU 暂停
};

// 触发条件表
void updatePowerLevel() {
    if (uploadInProgress || displayRefreshing) {
        if (powerLevel != PWR_NORMAL) enterNormalMode();
        return;
    }
    
    if (wifiState == MODE_AP_ONLY) {
        // AP 模式无法 Modem Sleep，只能降频
        if (getCpuFrequencyMhz() > 80) setCpuFrequencyMhz(80);
        return;
    }
    
    // STA 模式
    unsigned long idleTime = millis() - lastHttpRequest;
    
    if (idleTime < 5 * 60 * 1000) {          // < 5分钟
        if (powerLevel != PWR_MODEM_SLEEP) enterModemSleep();
    } else if (idleTime < 30 * 60 * 1000) {  // 5-30分钟
        // 保持 Modem Sleep
    } else {                                 // > 30分钟
        if (powerLevel != PWR_LIGHT_SLEEP) enterLightSleep();
    }
}
```

---

## 四、后端 API 变更汇总

| 方法 | 路径 | 变更 | 说明 |
|------|------|------|------|
| GET | `/` | 无变更 | 返回主页 |
| GET | `/status` | **扩展** | 新增 `sta_ssid`, `sta_rssi`, `uptime_seconds`, `current_time`, `power_mode` |
| GET | `/scan` | **新增** | 异步扫描 WiFi，返回可用网络列表 |
| POST | `/connect` | **修改** | 连接成功后不再重启，改为返回 `"连接成功"`，由前端提示用户刷新 |
| POST | `/disconnect` | **新增** | 断开 WiFi，清除配置，切换到 AP 模式 |
| POST | `/upload` | 无变更 | 图片上传 |
| POST | `/switchToRealTime` | 无变更 | 切换实时模式 |
| POST | `/switchToSlideShow` | 无变更 | 切换轮播模式 |

---

## 五、前端变更汇总

| 功能 | 实现方式 |
|------|---------|
| WiFi 状态自适应 | JS 调用 `/status`，根据 `wifi_connected` 动态渲染不同 UI |
| WiFi 扫描列表 | JS 调用 `/scan`，显示可选 SSID，点击自动填充 |
| 连接信息展示 | 已连接时显示 SSID、RSSI、IP、运行时间、当前时间 |
| 配网表单 | 未连接时显示 SSID 输入框 + 密码输入框 + 扫描结果列表 |
| 时间显示 | 前端 JS 使用 `Intl.DateTimeFormat` 格式化设备时间 |
| 运行时间 | 前端将 `uptime_seconds` 格式化为 "X天Y小时Z分" |

---

## 六、配置参数新增

```cpp
// Config.h 新增

// WiFi 状态
enum WiFiState {
    MODE_AP_ONLY = 0,    // 仅 AP 模式
    MODE_STA_ONLY = 1    // 仅 STA 模式（AP 已关闭）
};

// 省电级别
enum PowerLevel {
    PWR_NORMAL = 0,      // 全速运行
    PWR_MODEM_SLEEP = 1, // 射频休眠
    PWR_LIGHT_SLEEP = 2  // CPU 休眠
};

// 省电配置
#define PWR_IDLE_TO_MODEM_MS     (30 * 1000)     // 30秒空闲 → Modem Sleep
#define PWR_MODEM_TO_LIGHT_MS    (10 * 60 * 1000) // 10分钟空闲 → Light Sleep
#define PWR_LIGHT_WAKE_INTERVAL_MS (30 * 1000)   // Light Sleep 30秒唤醒检查

// AP 模式优化
#define AP_BEACON_INTERVAL       200             // AP beacon 间隔 200ms (默认100)
#define AP_CPU_FREQ_MHZ          80              // AP 模式下 CPU 频率
```

---

## 七、实施建议

### 7.1 实施优先级

1. **P0 - WiFi 连接逻辑** — 必须先实现，否则 AP 始终开启的问题无法解决
2. **P1 - 动态后台界面** — 依赖 WiFi 状态，P0 完成后实施
3. **P2 - Modem Sleep** — STA 模式下默认启用，改动小、收益大
4. **P3 - Light Sleep** — 深度省电，实现复杂度较高，可后续迭代

### 7.2 风险与注意

| 风险 | 缓解措施 |
|------|---------|
| Light Sleep 中 WiFi 数据包可能延迟响应 | 保持 Modem Sleep 作为默认，Light Sleep 仅用于长时间空闲 |
| AP 模式下 beacon 间隔增加导致客户端断开 | 测试 200ms 间隔的稳定性，必要时调回 100ms |
| 降频到 80MHz 后 WebServer 响应变慢 | 仅 AP 模式降频，STA 模式保持 240MHz |
| WiFi 扫描阻塞 WebServer | 使用异步扫描 `scanNetworks(true)` |
| 时间同步 | STA 模式下通过 NTP 获取时间，AP 模式下时间从上次同步保持 |

### 7.3 测试 checklist

- [ ] 无配置首次开机 → 仅 AP 模式
- [ ] 保存配置开机 → 2 次连接尝试 → 成功则关闭 AP
- [ ] 2 次连接失败 → 回退 AP 模式
- [ ] STA 模式下断开 WiFi → 自动回退 AP
- [ ] AP 模式下扫描 WiFi → 列表正确显示
- [ ] AP 模式下配置 WiFi → 保存并重启
- [ ] STA 模式下访问 `/status` → 显示连接信息
- [ ] STA 模式下 30 秒空闲 → Modem Sleep 生效
- [ ] 上传图片 → 自动退出省电，全速传输
- [ ] 上传完成后 → 回到 Modem Sleep

---

*方案版本: v2.0*
*日期: 2025-07-22*
*目标硬件: ESP32-S3-WROOM-1 + GDEP133C02*
