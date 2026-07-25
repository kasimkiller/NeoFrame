#ifndef WEB_PAGE_H
#define WEB_PAGE_H

const char INDEX_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Image Dithering to Epaper</title>
    <link href="https://fonts.googleapis.com/css2?family=Orbitron&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background-color: #1a1a2e;
            color: #e5e5e5;
            margin: 0;
            padding: 20px;
            min-height: 100vh;
            box-sizing: border-box;
            display: flex;
            flex-direction: column;
            align-items: center;
        }
        h1 {
            font-family: 'Orbitron', sans-serif;
            color: #00adb5;
            margin: 10px 0;
            font-size: 2.5em;
            text-shadow: 2px 2px 5px rgba(0, 173, 181, 0.7);
            text-align: center;
        }
        .container {
            background-color: #16213e;
            border-radius: 12px;
            padding: 15px;
            box-shadow: 0 10px 20px rgba(0, 0, 0, 0.3);
            max-width: 600px;
            width: 100%;
            box-sizing: border-box;
        }
        .file-upload-container {
            position: sticky;
            top: 10px;
            z-index: 100;
            background: #16213e;
            padding: 10px 0;
            border-radius: 8px;
            text-align: center;
        }
        label.file-label {
            display: inline-block;
            padding: 12px 24px;
            background-color: #00adb5;
            color: white;
            border-radius: 8px;
            cursor: pointer;
            font-weight: bold;
            transition: background-color 0.3s ease, transform 0.2s;
            font-size: 1.1em;
        }
        label.file-label:hover {
            background-color: #007f91;
            transform: scale(1.05);
        }
        input[type="file"] {
            display: none;
        }
        select, input[type="range"] {
            width: 100%;
            padding: 8px;
            border-radius: 5px;
            border: none;
            outline: none;
            font-size: 1em;
            background-color: #1f4068;
            color: #e5e5e5;
            margin-bottom: 10px;
        }
        #canvas {
            border: 2px solid #00adb5;
            border-radius: 8px;
            margin: 15px auto;
            display: block;
            max-width: 100%;
            height: auto;
        }
        button {
            padding: 10px 20px;
            border: none;
            border-radius: 8px;
            background-color: #00adb5;
            color: white;
            cursor: pointer;
            font-weight: bold;
            margin: 5px;
            transition: background-color 0.3s ease, transform 0.2s;
            font-size: 1em;
        }
        button:hover {
            background-color: #007f91;
            transform: scale(1.05);
        }
        .slider-container {
            display: flex;
            align-items: center;
            gap: 10px;
            margin-bottom: 10px;
        }
        .slider-container label {
            flex: 0 0 100px;
            font-size: 0.9em;
        }
        .controls-group {
            margin-bottom: 15px;
        }
        .controls-group label {
            display: block;
            font-size: 0.9em;
            margin-bottom: 5px;
        }
        .button-group {
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            gap: 10px;
        }
        #progressBar {
            width: 100%;
            height: 8px;
            background-color: #333;
            border-radius: 5px;
            margin-top: 15px;
            overflow: hidden;
        }
        #progressBar > div {
            height: 100%;
            background-color: #00adb5;
            width: 0;
            transition: width 0.3s;
        }
        #log {
            margin-top: 15px;
            padding: 10px;
            background-color: #1f4068;
            border-radius: 8px;
            max-height: 150px;
            overflow-y: auto;
            font-size: 0.9em;
            line-height: 1.4;
        }
        .back-to-top {
            position: fixed;
            bottom: 20px;
            right: 20px;
            padding: 10px 15px;
            background-color: #00adb5;
            color: white;
            border-radius: 50%;
            cursor: pointer;
            display: none;
            transition: background-color 0.3s ease;
        }
        .back-to-top:hover {
            background-color: #007f91;
        }
        @media (max-width: 600px) {
            h1 {
                font-size: 2em;
            }
            .container {
                padding: 10px;
            }
            button, label.file-label {
                padding: 10px 15px;
                font-size: 0.9em;
            }
            .slider-container label {
                font-size: 0.8em;
                flex: 0 0 80px;
            }
            select, input[type="range"] {
                font-size: 0.9em;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>NeoFrame</h1>
        <!-- WiFi 状态区域（动态渲染） -->
        <div id="wifi-section" class="controls-group">
            <h2>home WiFi network</h2>
            <p style="text-align:center;color:#888;">正在加载 WiFi 状态...</p>
        </div>
<div class="file-upload-container">
            <label for="upload" class="file-label">Upload Image</label>
            <input type="file" id="upload" accept="image/*">
        </div>
        <div class="controls-group" >
            <label for="esp32-ip">ESP32 IP Address:</label>
            <input type="text" id="esp32-ip" value="" placeholder="Auto-detected or enter manually" style="width: 100%; padding: 8px; border-radius: 5px; border: none; background-color: #1f4068; color: #e5e5e5;" />
        </div>
        <div class="controls-group">
            <label for="ditherMode">Colors Mode:</label>
            <select id="ditherMode">
                <option value="sixColor">Six Colors</option>
                <option value="fourColor">Four Colors</option>
                <option value="blackWhiteColor">Black & White</option>
                <option value="threeColor">Three Colors</option>
            </select>
        </div>
        <div class="controls-group">
            <label for="ditherType">Dithering Mode:</label>
    <select id="ditherType">
      <option value="floyd">标准Floyd-Steinberg</option>
      <option value="floydSerpentine" selected>Floyd-Steinberg 蛇形扫描（推荐）</option>
      <option value="atkinson">Atkinson</option>
      <option value="stucki">Stucki</option>
      <option value="jjn">Jarvis-Judice-Ninke</option>
            </select>
        </div>

        <canvas id="canvas"></canvas>
        <div class="button-group">
            <button id="download" style="display:none;">Download Dithered Image</button>
            <button id="sendToESP32">Send to Frame</button>
            <button id="downloadArray" style="display:none;">Download Data Array</button>
            <button id="switchToRealTime">Switch To Real-time Mode</button>
            <button id="switchToSlideShow">Switch To Slideshow Mode</button>
        </div>
        <div id="log"></div>
    </div>
    <div class="back-to-top" onclick="window.scrollTo({top: 0, behavior: 'smooth'})">↑</div>

    <script>
// 真实调色板 + 驱动纯色映射
const REAL_PALETTE = [[255,255,255],[0,0,0],[160,32,32],[240,224,80],[80,128,184],[96,128,80]];
const DRIVER_PALETTE = [[255,255,255],[0,0,0],[255,0,0],[255,255,0],[0,0,255],[0,255,0]];
const COLOR_MAP = new Map();
REAL_PALETTE.forEach((real, i) => COLOR_MAP.set(real.toString(), DRIVER_PALETTE[i]));


        // 事件监听
        document.getElementById('upload').addEventListener('change', handleFileUpload);
        document.getElementById('download').addEventListener('click', downloadImage);
        document.getElementById('sendToESP32').addEventListener('click', sendToESP32);
        document.getElementById('downloadArray').addEventListener('click', downloadDataArray);
        document.getElementById('switchToRealTime').addEventListener('click', switchToRealTime);
        document.getElementById('switchToSlideShow').addEventListener('click', switchToSlideShow);
        
        document.getElementById('ditherMode').addEventListener('change', updateImage);
        document.getElementById('ditherType').addEventListener('change', updateImage);

        let currentImageData = null;

        // 显示返回顶部按钮
        window.addEventListener('scroll', () => {
            const backToTop = document.querySelector('.back-to-top');
            backToTop.style.display = window.scrollY > 300 ? 'block' : 'none';
        });

        function handleFileUpload(event) {
            const file = event.target.files[0];
            event.target.value = '';
            const reader = new FileReader();

            reader.onload = function (e) {
                const img = new Image();
                img.onload = function () {
                    const canvas = document.getElementById('canvas');
                    const ctx = canvas.getContext('2d');

                    const imgWidth = img.width;
                    const imgHeight = img.height;

                    canvas.width = 1200;
                    canvas.height = 1600;

                    const scale = Math.max(canvas.width / imgWidth, canvas.height / imgHeight);
                    const scaledWidth = imgWidth * scale;
                    const scaledHeight = imgHeight * scale;

                    const cropX = (scaledWidth - canvas.width) / 2;
                    const cropY = (scaledHeight - canvas.height) / 2;

                    ctx.clearRect(0, 0, canvas.width, canvas.height);
                    ctx.drawImage(
                        img,
                        cropX / scale, cropY / scale,
                        canvas.width / scale, canvas.height / scale,
                        0, 0,
                        canvas.width, canvas.height
                    );

                    currentImageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
                    updateImage();

                    document.getElementById('sendToESP32').style.display = 'inline';
                    document.getElementById('switchToRealTime').style.display = 'inline';
                    document.getElementById('switchToSlideShow').style.display = 'inline';
                };
                img.src = e.target.result;
            };
            reader.readAsDataURL(file);
        }

function updateImage() {
    if (!currentImageData) return;

    const canvas = document.getElementById('canvas');
    const ctx = canvas.getContext('2d');
    const imageData = new ImageData(
        new Uint8ClampedArray(currentImageData.data),
        currentImageData.width,
        currentImageData.height
    );

    // 直接进行抖动，不再调整对比度
    ditherImage(imageData);

    // 可选：如果你想在发送前看到“驱动纯色”效果，可以在这里加
     remapToDriver(imageData);  // 注意：这会把真实颜色换成纯 RGB，会看起来很奇怪，仅用于调试

    ctx.putImageData(imageData, 0, 0);
}

        function adjustContrast(imageData, factor) {
            const data = imageData.data;
            for (let i = 0; i < data.length; i += 4) {
                data[i] = Math.min(255, Math.max(0, (data[i] - 128) * factor + 128));
                data[i + 1] = Math.min(255, Math.max(0, (data[i + 1] - 128) * factor + 128));
                data[i + 2] = Math.min(255, Math.max(0, (data[i + 2] - 128) * factor + 128));
            }
            return imageData;
        }

        function rgbToLab(r, g, b) {
            r = r / 255;
            g = g / 255;
            b = b / 255;

            r = r > 0.04045 ? Math.pow((r + 0.055) / 1.055, 2.4) : r / 12.92;
            g = g > 0.04045 ? Math.pow((g + 0.055) / 1.055, 2.4) : g / 12.92;
            b = b > 0.04045 ? Math.pow((b + 0.055) / 1.055, 2.4) : b / 12.92;

            r *= 100;
            g *= 100;
            b *= 100;

            let x = r * 0.4124 + g * 0.3576 + b * 0.1805;
            let y = r * 0.2126 + g * 0.7152 + b * 0.0722;
            let z = r * 0.0193 + g * 0.1192 + b * 0.9505;

            x /= 95.047;
            y /= 100.0;
            z /= 108.883;

            x = x > 0.008856 ? Math.pow(x, 1/3) : (7.787 * x) + (16 / 116);
            y = y > 0.008856 ? Math.pow(y, 1/3) : (7.787 * y) + (16 / 116);
            z = z > 0.008856 ? Math.pow(z, 1/3) : (7.787 * z) + (16 / 116);

            const l = (116 * y) - 16;
            const a = 500 * (x - y);
            const bLab = 200 * (y - z);

            return { l, a, b: bLab };
        }

        function labDistance(lab1, lab2) {
            const dl = lab1.l - lab2.l;
            const da = lab1.a - lab2.a;
            const db = lab1.b - lab2.b;
            return Math.sqrt(0.2 * dl * dl + 3 * da * da + 3 * db * db);
        }

        function findClosestColor(r, g, b) {
            if (r < 50 && g < 150 && b > 100) {
                return rgbPalette[2];
            }

            const inputLab = rgbToLab(r, g, b);
            let minDistance = Infinity;
            let closestColor = rgbPalette[0];

            for (const color of rgbPalette) {
                const colorLab = rgbToLab(color.r, color.g, color.b);
                const distance = labDistance(inputLab, colorLab);
                if (distance < minDistance) {
                    minDistance = distance;
                    closestColor = color;
                }
            }

            return closestColor;
        }

      function closestColor(c, palette) {
  let best = palette[0], minD = Infinity;
  for (const p of palette) {
    const d = Math.pow(c[0]-p[0],2) + Math.pow(c[1]-p[1],2) + Math.pow(c[2]-p[2],2);
    if (d < minD) { minD = d; best = p; }
  }
  return best;
}

// 标准 Floyd-Steinberg
function floydDither(imgData) {
  const w = imgData.width, h = imgData.height, data = imgData.data;
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      const i = (y * w + x) * 4;
      const old = [data[i], data[i+1], data[i+2]];
      const realC = closestColor(old, REAL_PALETTE);
      data[i] = realC[0]; data[i+1] = realC[1]; data[i+2] = realC[2];
      const err = [old[0]-realC[0], old[1]-realC[1], old[2]-realC[2]];
      const diff = (dx, dy, f) => {
        const nx = x + dx, ny = y + dy;
        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
          const ni = (ny * w + nx) * 4;
          data[ni] += err[0] * f;
          data[ni+1] += err[1] * f;
          data[ni+2] += err[2] * f;
        }
      };
      diff(1,0,7/16); diff(-1,1,3/16); diff(0,1,5/16); diff(1,1,1/16);
    }
  }
}

// Floyd-Steinberg 蛇形扫描优化变体
function floydSerpentineDither(imgData) {
  const w = imgData.width, h = imgData.height, data = imgData.data;
  for (let y = 0; y < h; y++) {
    const leftToRight = (y % 2 === 0);
    const startX = leftToRight ? 0 : w - 1;
    const endX = leftToRight ? w : -1;
    const step = leftToRight ? 1 : -1;
    for (let x = startX; leftToRight ? x < endX : x > endX; x += step) {
      const i = (y * w + x) * 4;
      const old = [data[i], data[i+1], data[i+2]];
      const realC = closestColor(old, REAL_PALETTE);
      data[i] = realC[0]; data[i+1] = realC[1]; data[i+2] = realC[2];
      const err = [old[0]-realC[0], old[1]-realC[1], old[2]-realC[2]];
      const diff = (dx, dy, f) => {
        const nx = x + dx * step;  // 方向随扫描调整
        const ny = y + dy;
        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
          const ni = (ny * w + nx) * 4;
          data[ni] += err[0] * f;
          data[ni+1] += err[1] * f;
          data[ni+2] += err[2] * f;
        }
      };
      diff(1,0,7/16); diff(-1,1,3/16); diff(0,1,5/16); diff(1,1,1/16);
    }
  }
}

// Atkinson
function atkinsonDither(imgData) {
  const w = imgData.width, h = imgData.height, data = imgData.data;
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const i = (y * w + x) * 4;
    const old = [data[i], data[i+1], data[i+2]];
    const realC = closestColor(old, REAL_PALETTE);
    data[i] = realC[0]; data[i+1] = realC[1]; data[i+2] = realC[2];
    const err = [(old[0]-realC[0])/8, (old[1]-realC[1])/8, (old[2]-realC[2])/8];
    const diff = (dx, dy, f) => {
      const nx = x + dx, ny = y + dy;
      if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
        const ni = (ny * w + nx) * 4;
        data[ni] += err[0] * f; data[ni+1] += err[1] * f; data[ni+2] += err[2] * f;
      }
    };
    diff(1,0,1); diff(2,0,1); diff(-1,1,1); diff(0,1,1); diff(1,1,1); diff(0,2,1);
  }
}

// Stucki
function stuckiDither(imgData) {
  const w = imgData.width, h = imgData.height, data = imgData.data;
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const i = (y * w + x) * 4;
    const old = [data[i], data[i+1], data[i+2]];
    const realC = closestColor(old, REAL_PALETTE);
    data[i] = realC[0]; data[i+1] = realC[1]; data[i+2] = realC[2];
    const err = [old[0]-realC[0], old[1]-realC[1], old[2]-realC[2]];
    const total = 42;
    const diff = (dx, dy, weight) => {
      const nx = x + dx, ny = y + dy;
      if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
        const ni = (ny * w + nx) * 4;
        const f = weight / total;
        data[ni] += err[0] * f; data[ni+1] += err[1] * f; data[ni+2] += err[2] * f;
      }
    };
    diff(1,0,8); diff(2,0,4);
    diff(-2,1,2); diff(-1,1,4); diff(0,1,8); diff(1,1,4); diff(2,1,2);
    diff(-2,2,1); diff(-1,2,2); diff(0,2,4); diff(1,2,2); diff(2,2,1);
  }
}

// Jarvis-Judice-Ninke
function jjnDither(imgData) {
  const w = imgData.width, h = imgData.height, data = imgData.data;
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {
    const i = (y * w + x) * 4;
    const old = [data[i], data[i+1], data[i+2]];
    const realC = closestColor(old, REAL_PALETTE);
    data[i] = realC[0]; data[i+1] = realC[1]; data[i+2] = realC[2];
    const err = [old[0]-realC[0], old[1]-realC[1], old[2]-realC[2]];
    const total = 48;
    const diff = (dx, dy, weight) => {
      const nx = x + dx, ny = y + dy;
      if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
        const ni = (ny * w + nx) * 4;
        const f = weight / total;
        data[ni] += err[0] * f; data[ni+1] += err[1] * f; data[ni+2] += err[2] * f;
      }
    };
    diff(1,0,7); diff(2,0,5);
    diff(-2,1,3); diff(-1,1,5); diff(0,1,7); diff(1,1,5); diff(2,1,3);
    diff(-2,2,1); diff(-1,2,3); diff(0,2,5); diff(1,2,3); diff(2,2,1);
  }
}

// 替换为驱动纯色
function remapToDriver(imgData) {
  const data = imgData.data;
  for (let i = 0; i < data.length; i += 4) {
    const key = [data[i], data[i+1], data[i+2]].toString();
    const mapped = COLOR_MAP.get(key);
    if (mapped) {
      data[i] = mapped[0]; data[i+1] = mapped[1]; data[i+2] = mapped[2];
    }
  }
}
let resultCanvas;
function ditherImage(imageData) {
    const ditherType = document.getElementById('ditherType').value;
    // ditherStrength 目前只有 Atkinson/Stucki/JJN 在用，Floyd 类一般不需要，但保留参数

    switch (ditherType) {
        case 'floyd':
            floydDither(imageData);
            break;
        case 'floydSerpentine':
            floydSerpentineDither(imageData);
            break;
        case 'atkinson':
            atkinsonDither(imageData);
            break;
        case 'stucki':
            stuckiDither(imageData);
            break;
        case 'jjn':
            jjnDither(imageData);
            break;
        default:
            // 不做任何抖动
            break;
    }
    // 注意：所有抖动函数都是直接修改 imageData，不需要返回
}

        function processImageData(imageData) {
            const width = imageData.width;
            const height = imageData.height;
            const data = imageData.data;
            const mode = document.getElementById('ditherMode').value;

            let processedData;

if (mode === 'sixColor') {
    const width = imageData.width;
    const height = imageData.height;
    const data = imageData.data;
    const processedData = new Uint8Array(Math.ceil((width * height) / 2));  // 每个字节存储2个像素

    // 将 RGB 转换为 6 色模式
    function rgbToSixColor(r, g, b) {
        if (r < 128 && g < 128 && b < 128) return 0x00;  // 黑色
        if (r > 128 && g > 128 && b > 128) return 0x01;  // 白色
        if (r > 128 && g < 128 && b < 128) return 0x03;  // 红色
        if (r > 128 && g > 128 && b < 128) return 0x02;  // 黄色
        if (r < 128 && g > 128 && b < 128) return 0x06;  // 绿色
        if (r < 128 && g < 128 && b > 128) return 0x05;  // 蓝色
        return 0x01;  // 默认白色
    }

    // 处理图像数据
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x += 2) { // 每两个像素处理一次
            const index1 = (y * width + x) * 4;       // 第一个像素的索引
            const index2 = (y * width + x + 1) * 4;   // 第二个像素的索引

            // 获取两个像素的颜色值
            const r1 = data[index1];
            const g1 = data[index1 + 1];
            const b1 = data[index1 + 2];

            const r2 = data[index2];
            const g2 = data[index2 + 1];
            const b2 = data[index2 + 2];

            // 将 RGB 转换为 6 色模式
            const colorValue1 = rgbToSixColor(r1, g1, b1);
            const colorValue2 = rgbToSixColor(r2, g2, b2);

            // 组合成一个字节，每两个4位颜色值组成一个字节
            const combinedValue = (colorValue1 << 4) | colorValue2;

            // 计算在 processedData 中的索引
            const newIndex = (y * (width / 2)) + (x / 2);
            processedData[newIndex] = combinedValue;
        }
    }

  

    return processedData;
}
           /*
           if (mode === 'sixColor') {
    // 调整 processedData 的大小为 (width * height) / 2，因为 2 个像素占 1 字节
    processedData = new Uint8Array(Math.ceil((width * height) / 2));
    
    for (let y = 0; y < height; y++) {
        for (let x = 0; x < width; x++) {
            // 获取当前像素的颜色
            const index = (y * width + x) * 4;
            const r = data[index];
            const g = data[index + 1];
            const b = data[index + 2];

            const closest = findClosestColor(r, g, b);
            
            // 计算新索引（按照你的旋转方式：(x * height) + (height - 1 - y)）
            const pixelIndex = (x * height) + (height - 1 - y);
            
            // 计算 processedData 中的字节索引和位位置
            const byteIndex = Math.floor(pixelIndex / 2); // 每 2 个像素占 1 字节
            const isFirstPixel = pixelIndex % 2 === 0; // 当前像素是字节中的第一个还是第二个
            
            // 确保 closest.value 在 0-15 范围内（4 位）
            const colorValue = closest.value & 0x0F; // 取低 4 位
            
            // 将颜色值写入 processedData
            if (isFirstPixel) {
                // 第一个像素，写入高 4 位，并保留低 4 位（可能是之前的值或 0）
                processedData[byteIndex] = (processedData[byteIndex] & 0x0F) | (colorValue << 4);
            } else {
                // 第二个像素，写入低 4 位，并保留高 4 位
                processedData[byteIndex] = (processedData[byteIndex] & 0xF0) | colorValue;
            }
        }
    }
}
      */     
           
           
            else if (mode === 'fourColor') {
                processedData = new Uint8Array(Math.ceil((width * height) / 4));
                function rgbToGray(r, g, b) {
                    const grayscale = Math.round(0.299 * r + 0.587 * g + 0.114 * b);
                    if (grayscale < 64) return 0x03;
                    if (grayscale < 128) return 0x02;
                    if (grayscale < 140) return 0x00;
                    return 0x01;
                }

                for (let y = 0; y < height; y++) {
                    for (let x = 0; x < width; x++) {
                        const index = (y * width + x) * 4;
                        const r = data[index];
                        const g = data[index + 1];
                        const b = data[index + 2];
                        const grayValue = rgbToGray(r, g, b);
                        const newIndex = (y * width + x) / 4 | 0;
                        const shift = 6 - ((x % 4) * 2);
                        processedData[newIndex] |= (grayValue << shift);
                    }
                }
            } else if (mode === 'blackWhiteColor') {
                const byteWidth = Math.ceil(width / 8);
                processedData = new Uint8Array(byteWidth * height);
                const threshold = 140;

                for (let y = 0; y < height; y++) {
                    for (let x = 0; x < width; x++) {
                        const index = (y * width + x) * 4;
                        const r = data[index];
                        const g = data[index + 1];
                        const b = data[index + 2];
                        const grayscale = Math.round(0.299 * r + 0.587 * g + 0.114 * b);
                        const bit = grayscale >= threshold ? 1 : 0;
                        const byteIndex = y * byteWidth + Math.floor(x / 8);
                        const bitIndex = 7 - (x % 8);
                        processedData[byteIndex] |= (bit << bitIndex);
                    }
                }
            } else if (mode === 'threeColor') {
                const byteWidth = Math.ceil(width / 8);
                const blackWhiteThreshold = 140;
                const redThreshold = 160;

                const blackWhiteData = new Uint8Array(height * byteWidth);
                const redWhiteData = new Uint8Array(height * byteWidth);

                for (let y = 0; y < height; y++) {
                    for (let x = 0; x < width; x++) {
                        const index = (y * width + x) * 4;
                        const r = data[index];
                        const g = data[index + 1];
                        const b = data[index + 2];
                        const grayscale = Math.round(0.299 * r + 0.587 * g + 0.114 * b);

                        const blackWhiteBit = grayscale >= blackWhiteThreshold ? 1 : 0;
                        const blackWhiteByteIndex = y * byteWidth + Math.floor(x / 8);
                        const blackWhiteBitIndex = 7 - (x % 8);
                        if (blackWhiteBit) {
                            blackWhiteData[blackWhiteByteIndex] |= (0x01 << blackWhiteBitIndex);
                        } else {
                            blackWhiteData[blackWhiteByteIndex] &= ~(0x01 << blackWhiteBitIndex);
                        }

                        const redWhiteBit = (r > redThreshold && r > g && r > b) ? 0 : 1;
                        const redWhiteByteIndex = y * byteWidth + Math.floor(x / 8);
                        const redWhiteBitIndex = 7 - (x % 8);
                        if (redWhiteBit) {
                            redWhiteData[redWhiteByteIndex] |= (0x01 << redWhiteBitIndex);
                        } else {
                            redWhiteData[redWhiteByteIndex] &= ~(0x01 << redWhiteBitIndex);
                        }
                    }
                }

                processedData = new Uint8Array(blackWhiteData.length + redWhiteData.length);
                processedData.set(blackWhiteData, 0);
                processedData.set(redWhiteData, blackWhiteData.length);
            }

            return processedData;
        }

        function downloadImage() {
            const canvas = document.getElementById('canvas');
            const dataURL = canvas.toDataURL('image/png');
            const link = document.createElement('a');
            link.href = dataURL;
            link.download = 'dithered_image.png';
            link.click();
        }

        async function sendToESP32() {
            const canvas = document.getElementById('canvas');
            const ctx = canvas.getContext('2d');
            const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
            const processedData = processImageData(imageData);
            const esp32IP = document.getElementById('esp32-ip').value.trim() || window.location.hostname || '192.168.4.1';
            const CHUNK_SIZE = 960000;

            if (!esp32IP) {
                alert('Please enter the IP address of the ESP32 first');
                return;
            }

            const totalChunks = Math.ceil(processedData.length / CHUNK_SIZE);
            
            // 创建进度弹窗
            const modal = document.createElement('div');
            modal.id = 'upload-modal';
            modal.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;background:rgba(0,0,0,0.7);display:flex;justify-content:center;align-items:center;z-index:9999;';
            modal.innerHTML = `
                <div style="background:#16213e;padding:24px 32px;border-radius:12px;max-width:420px;width:90%;text-align:center;box-shadow:0 8px 32px rgba(0,0,0,0.4);">
                    <h3 style="margin:0 0 16px 0;color:#00adb5;">Uploading to Frame</h3>
                    <p id="upload-status" style="margin:0 0 12px 0;color:#e5e5e5;font-size:0.95em;">Preparing image data...</p>
                    <div style="width:100%;height:20px;background:#1f4068;border-radius:10px;overflow:hidden;margin-bottom:12px;">
                        <div id="upload-bar" style="width:0%;height:100%;background:linear-gradient(90deg,#00adb5,#0077b6);transition:width 0.2s;border-radius:10px;"></div>
                    </div>
                    <p id="upload-percent" style="margin:0;color:#888;font-size:0.85em;">0%</p>
                    <p style="margin:12px 0 0 0;color:#666;font-size:0.8em;">Please do not close the page</p>
                </div>
            `;
            document.body.appendChild(modal);

            function updateProgress(percent, statusText) {
                const bar = document.getElementById('upload-bar');
                const pct = document.getElementById('upload-percent');
                const status = document.getElementById('upload-status');
                if (bar) bar.style.width = percent + '%';
                if (pct) pct.textContent = Math.round(percent) + '%';
                if (status && statusText) status.textContent = statusText;
            }

            function closeModal() {
                const m = document.getElementById('upload-modal');
                if (m) m.remove();
            }

            try {
                for (let i = 0; i < totalChunks; i++) {
                    const chunk = processedData.slice(i * CHUNK_SIZE, (i + 1) * CHUNK_SIZE);
                    const blob = new Blob([chunk], { type: 'application/octet-stream' });
                    const formData = new FormData();
                    formData.append('data', blob, 'image_data.bin');

                    updateProgress(0, `Uploading chunk ${i + 1}/${totalChunks}...`);

                    await new Promise((resolve, reject) => {
                        const xhr = new XMLHttpRequest();
                        
                        // 上传进度
                        xhr.upload.onprogress = (e) => {
                            if (e.lengthComputable) {
                                const chunkPct = (e.loaded / e.total) * 100;
                                const overallPct = ((i * 100) + chunkPct) / totalChunks;
                                updateProgress(overallPct, `Uploading... ${(e.loaded / 1024).toFixed(1)}KB / ${(e.total / 1024).toFixed(1)}KB`);
                            }
                        };

                        xhr.onload = () => {
                            if (xhr.status >= 200 && xhr.status < 300) {
                                if (xhr.responseText.includes("图片上传成功")) {
                                    resolve(xhr.responseText);
                                } else {
                                    reject(new Error('Server: ' + xhr.responseText));
                                }
                            } else {
                                reject(new Error('HTTP ' + xhr.status + ': ' + xhr.statusText));
                            }
                        };

                        xhr.onerror = () => reject(new Error('Network error'));
                        xhr.ontimeout = () => reject(new Error('Request timeout (120s)'));
                        xhr.onabort = () => reject(new Error('Request aborted'));

                        xhr.open('POST', `http://${esp32IP}/upload`, true);
                        xhr.timeout = 120000; // 120 seconds
                        xhr.send(formData);
                    });

                    updateProgress(100, 'Upload complete! Processing...');
                    await new Promise(r => setTimeout(r, 500));
                }

                closeModal();
                alert('Image uploaded successfully! The frame is refreshing...');
            } catch (error) {
                closeModal();
                console.error('Failed to send data:', error);
                alert('Upload failed\nTarget IP: ' + esp32IP + '\nError: ' + error.message + '\n\nPlease check:\n1. Is the IP address correct?\n2. Is the device on the same network?\n3. Try refreshing the page');
            }
        }
        async function switchToRealTime() {
            const esp32IP = document.getElementById('esp32-ip').value.trim() || '192.168.4.1';
            if (!esp32IP) {
                alert('Please enter the IP address of ESP32 first');
                return;
            }
            try {
                const response = await fetch(`http://${esp32IP}/switchToRealTime`, {
                    method: 'POST',
                    mode: 'cors'
                });
                if (!response.ok) {
                    throw new Error(`Error: ${response.statusText}`);
                }
                const responseText = await response.text();
                console.log(`Server response: ${responseText}`);
                if (responseText.includes("切换到实时模式成功")) {
                    alert('Successfully switched to real-time mode');
                }
            } catch (error) {
                console.error('Failed to switch to real-time mode:', error);
                alert('Failed to switch to real-time mode');
            }
        }

        async function switchToSlideShow() {
            const esp32IP = document.getElementById('esp32-ip').value.trim() || '192.168.4.1';
            if (!esp32IP) {
                alert('Please enter the ESP32 IP address first');
                return;
            }
            try {
                const response = await fetch(`http://${esp32IP}/switchToSlideShow`, {
                    method: 'POST',
                    mode: 'cors'
                });
                if (!response.ok) {
                    throw new Error(`Error: ${response.statusText}`);
                }
                const responseText = await response.text();
                console.log(`Server response: ${responseText}`);
                if (responseText.includes("切换到轮播模式成功")) {
                    alert('Successfully switched to slideshow mode');
                }
            } catch (error) {
                console.error('Failed to switch to slideshow mode:', error);
                alert('Failed to switch to slideshow mode');
            }
        }

        function downloadDataArray() {
            const canvas = document.getElementById('canvas');
            const ctx = canvas.getContext('2d');
            const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
            const processedData = processImageData(imageData);

            let dataString = 'const unsigned char gImage[] = {\n';
            for (let i = 0; i < processedData.length; i++) {
                dataString += `0x${processedData[i].toString(16).padStart(2, '0')}`;
                if (i !== processedData.length - 1) dataString += ', ';
                if ((i + 1) % 16 === 0) dataString += '\n';
            }
            dataString += '\n};';

            const blob = new Blob([dataString], { type: 'text/plain' });
            const url = URL.createObjectURL(blob);

            const link = document.createElement('a');
            link.download = 'image_data_array.c';
            link.href = url;
            link.click();
            URL.revokeObjectURL(url);
        }
    
        // ========== 动态状态加载 ==========
        async function loadStatus() {
            try {
                const resp = await fetch('/status');
                const data = await resp.json();
                renderWiFiSection(data);
            } catch (e) {
                console.error('Failed to load status:', e);
                document.getElementById('wifi-section').innerHTML = 
                    '<h2>home WiFi network</h2><p style="color:#ff6b6b;text-align:center;">无法获取设备状态</p>';
            }
        }
        
        function renderWiFiSection(data) {
            const section = document.getElementById('wifi-section');
            
            if (data.wifi_connected) {
                // 已连接 → 显示连接信息
                section.innerHTML = `
                    <h2>WiFi 已连接</h2>
                    <div class="info-grid" style="background:#1f4068;padding:12px;border-radius:8px;line-height:1.8;">
                        <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid #333;">
                            <span style="color:#888;">SSID:</span>
                            <span>${escapeHtml(data.sta_ssid)}</span>
                        </div>
                        <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid #333;">
                            <span style="color:#888;">信号强度:</span>
                            <span>${data.sta_rssi} dBm</span>
                        </div>
                        <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid #333;">
                            <span style="color:#888;">IP 地址:</span>
                            <span>${data.sta_ip}</span>
                        </div>
                        <div style="display:flex;justify-content:space-between;padding:4px 0;border-bottom:1px solid #333;">
                            <span style="color:#888;">运行时间:</span>
                            <span>${formatUptime(data.uptime_seconds)}</span>
                        </div>
                        <div style="display:flex;justify-content:space-between;padding:4px 0;">
                            <span style="color:#888;">省电模式:</span>
                            <span>${data.power_mode === 'modem_sleep' ? 'Modem Sleep' : '正常'}</span>
                        </div>
                    </div>
                    <button onclick="disconnectWiFi()" style="margin-top:10px;background:#e74c3c;">断开 WiFi</button>
                `;
            } else {
                // 未连接 → 显示 WiFi 配置表单 + 扫描按钮
                section.innerHTML = `
                    <h2>home WiFi network</h2>
                    <form onsubmit="return handleConnect(event)">
                        <label for="ssid">WiFi name(SSID):</label>
                        <div style="display:flex;gap:8px;margin-bottom:10px;">
                            <input type="text" id="ssid" name="ssid" placeholder="Enter the WiFi name" required 
                                style="flex:1;padding:8px;border-radius:5px;border:none;background-color:#1f4068;color:#e5e5e5;">
                            <button type="button" id="scan-btn" onclick="startScan()" style="margin:0;white-space:nowrap;">Scan WiFi</button>
                        </div>
                        <div id="wifi-scan-results" style="display:none;margin-bottom:10px;"></div>
                        <label for="password">WiFi password:</label>
                        <input type="password" id="password" name="password" placeholder="Enter the WiFi password" 
                            style="width:100%;padding:8px;border-radius:5px;border:none;background-color:#1f4068;color:#e5e5e5;margin-bottom:10px;">
                        <button type="submit">Connect to WiFi</button>
                    </form>
                `;
            }
        }
        
        function escapeHtml(text) {
            if (!text) return '';
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }
        
        function formatUptime(seconds) {
            const d = Math.floor(seconds / 86400);
            const h = Math.floor((seconds % 86400) / 3600);
            const m = Math.floor((seconds % 3600) / 60);
            const s = seconds % 60;
            let parts = [];
            if (d > 0) parts.push(d + '天');
            if (h > 0) parts.push(h + '小时');
            if (m > 0) parts.push(m + '分钟');
            parts.push(s + '秒');
            return parts.join('');
        }
        
        function startScan() {
            const resultsDiv = document.getElementById('wifi-scan-results');
            const scanBtn = document.getElementById('scan-btn');
            if (!resultsDiv || !scanBtn) return;
            
            // 取消旧超时
            if (window.__scanTimeoutId) {
                clearTimeout(window.__scanTimeoutId);
            }
            
            scanBtn.textContent = 'Scanning...';
            scanBtn.disabled = true;
            resultsDiv.style.display = 'block';
            resultsDiv.innerHTML = '<p style="color:#888;text-align:center;margin:8px 0;">正在扫描 WiFi...</p>';
            
            // 15 秒超时兜底：后端最多等 10 秒，前端给 15 秒
            window.__scanTimeoutId = setTimeout(() => {
                scanBtn.textContent = 'Scan WiFi';
                scanBtn.disabled = false;
                resultsDiv.innerHTML = '<p style="color:#ff6b6b;text-align:center;margin:8px 0;">扫描超时 (15s)，请重试</p>';
                window.__scanTimeoutId = null;
            }, 15000);
            
            scanWiFiNetworks();
        }
        
        async function scanWiFiNetworks() {
            const resultsDiv = document.getElementById('wifi-scan-results');
            const scanBtn = document.getElementById('scan-btn');
            if (!resultsDiv) return;
            
            try {
                const resp = await fetch('/scan');
                if (!resp.ok) {
                    throw new Error('HTTP ' + resp.status);
                }
                const data = await resp.json();
                
                // 后端返回 scanning 状态（不应发生，但兼容处理）
                if (data.status === 'scanning') {
                    resultsDiv.innerHTML = '<p style="color:#888;text-align:center;margin:8px 0;">正在扫描 WiFi...</p>';
                    setTimeout(scanWiFiNetworks, 2000);
                    return;
                }
                
                // 扫描完成，清除超时
                if (window.__scanTimeoutId) {
                    clearTimeout(window.__scanTimeoutId);
                    window.__scanTimeoutId = null;
                }
                
                if (scanBtn) {
                    scanBtn.textContent = 'Scan WiFi';
                    scanBtn.disabled = false;
                }
                
                // 处理失败/超时
                if (data.status === 'failed') {
                    const err = data.error || 'scan failed';
                    resultsDiv.innerHTML = `<p style="color:#ff6b6b;text-align:center;margin:8px 0;">扫描失败: ${escapeHtml(err)}</p>`;
                    return;
                }
                if (data.status === 'timeout') {
                    resultsDiv.innerHTML = '<p style="color:#ff6b6b;text-align:center;margin:8px 0;">扫描超时，未找到网络</p>';
                    return;
                }
                
                // 空结果
                if (!data.networks || data.networks.length === 0) {
                    resultsDiv.innerHTML = '<p style="color:#888;text-align:center;margin:8px 0;">未找到 WiFi 网络，请手动输入</p>';
                    return;
                }
                
                // 渲染结果列表
                let html = '<div style="max-height:200px;overflow-y:auto;background:#1f4068;border-radius:8px;padding:8px;">';
                html += '<p style="margin:0 0 8px 0;color:#00adb5;font-weight:bold;font-size:0.9em;">可用 WiFi 网络 (点击选择):</p>';
                data.networks.forEach((net, i) => {
                    const lock = net.open ? '\uD83D\uDD13' : '\uD83D\uDD12';
                    const bars = net.rssi > -50 ? '\u2582\u2584\u2586\u2588' : net.rssi > -65 ? '\u2582\u2584\u2586_' : '\u2582\u2584__';
                    html += `<div class="wifi-item" onclick="selectSSID('${escapeHtml(net.ssid)}')" 
                        style="padding:8px 10px;margin:4px 0;border-radius:5px;cursor:pointer;background:#16213e;transition:background 0.2s;display:flex;justify-content:space-between;align-items:center;"
                        onmouseover="this.style.background='#1a1a2e'" onmouseout="this.style.background='#16213e'">
                        <span style="font-size:0.95em;">${lock} ${escapeHtml(net.ssid)}</span>
                        <span style="color:#888;font-size:0.85em;white-space:nowrap;">${bars} ${net.rssi}dBm</span>
                    </div>`;
                });
                html += '</div>';
                resultsDiv.innerHTML = html;
            } catch (e) {
                // 清除超时
                if (window.__scanTimeoutId) {
                    clearTimeout(window.__scanTimeoutId);
                    window.__scanTimeoutId = null;
                }
                if (scanBtn) {
                    scanBtn.textContent = 'Scan WiFi';
                    scanBtn.disabled = false;
                }
                resultsDiv.innerHTML = `<p style="color:#ff6b6b;text-align:center;margin:8px 0;">请求失败: ${escapeHtml(e.message)}</p>`;
            }
        }
        function selectSSID(ssid) {
            const input = document.getElementById('ssid');
            if (input) input.value = ssid;
        }
        
        async function handleConnect(event) {
            event.preventDefault();
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            const btn = event.target.querySelector('button[type="submit"]');
            
            btn.textContent = '正在连接...';
            btn.disabled = true;
            
            try {
                const resp = await fetch('/connect', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: 'ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password)
                });
                const text = await resp.text();
                
                if (text.includes('已保存')) {
                    alert('WiFi 配置已保存，设备即将重启。\n重启后请通过 WiFi IP 访问后台。');
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
        
        async function disconnectWiFi() {
            if (!confirm('确定要断开 WiFi 并清除配置吗？')) return;
            try {
                const resp = await fetch('/disconnect', {method: 'POST'});
                const text = await resp.text();
                alert(text);
            } catch (e) {
                alert('请求失败');
            }
        }
        
        // 页面加载时获取状态
        // 自动检测当前访问的 IP/域名并填入输入框
        const ipInput = document.getElementById('esp32-ip');
        if (ipInput && window.location.hostname) {
            ipInput.value = window.location.hostname;
        }
        
        // 立即加载状态（DOM 已 ready，脚本在 body 末尾）
        loadStatus();
        // 同时每 10 秒刷新一次状态
        setInterval(loadStatus, 10000);
        
    </script>
</body>
</html>
)=====";

#endif
