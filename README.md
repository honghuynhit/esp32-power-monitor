# ESP32 Power Monitor

Hệ thống giám sát nguồn điện thông minh với cảnh báo tự động và OTA updates qua GitHub.

## ✨ Tính năng

- 🔌 **Giám sát nguồn điện 24/7**
- 🌙 **Cảnh báo ban đêm** (21:30, mỗi 15 phút)
- ⏰ **Cảnh báo hoạt động liên tục** (>3 giờ, mỗi 30 phút)
- 📊 **Đếm số lần bật nguồn hàng ngày**
- 📱 **Thông báo Telegram real-time**
- 📝 **Log data lên Google Sheets**
- 🔄 **OTA firmware update tự động**
- 💾 **Lưu cấu hình vào NVRAM** (không mất khi mất điện)

## 🛠️ Hardware

### ESP32 bất kỳ
- ESP32 Dev Module
- ESP32-C6
- ESP32-S3
- Hoặc bất kỳ board ESP32 nào

### Cấp nguồn
Khi nguồn chính bật → ESP32 được cấp điện → Gửi cảnh báo

## 📦 Cài đặt

### 1. Clone repository

```bash
git clone https://github.com/honghuynhit/esp32-power-monitor.git
cd esp32-power-monitor
```

### 2. Mở Arduino IDE

```
File → Open → ESP32_Power_Monitor/ESP32_Power_Monitor.ino
```

### 3. Cài đặt ESP32 Board

```
Tools → Board → Boards Manager → Tìm "esp32" → Install
```

### 4. Cấu hình Board

```
Tools → Board → ESP32 Arduino → ESP32 Dev Module
Tools → Partition Scheme → Minimal SPIFFS (1.9MB APP with OTA)
Tools → Upload Speed → 921600
Tools → Port → [Chọn cổng của ESP32]
```

### 5. Setup lần đầu

**Mở file `ESP32_Power_Monitor.ino`, uncomment dòng:**

```cpp
#define FIRST_TIME_SETUP
```

**Upload code và mở Serial Monitor (115200 baud):**

```
Tools → Serial Monitor
Baud rate: 115200
```

**Nhập thông tin khi được yêu cầu:**

```
1. WiFi SSID: [Tên WiFi của bạn]
2. WiFi Password: [Mật khẩu WiFi]
3. Google Webhook URL: [URL từ Google Apps Script]
4. Telegram Bot Token: [Token từ BotFather]
5. Telegram Chat ID: [ID chat của bạn]
6. Firmware Version URL: 
   https://github.com/honghuynhit/esp32-power-monitor/releases/latest/download/version.txt
7. Firmware Binary URL:
   https://github.com/honghuynhit/esp32-power-monitor/releases/latest/download/firmware.bin
```

**Sau khi nhập xong:**
- Comment lại dòng `// #define FIRST_TIME_SETUP`
- Upload lại code
- ESP32 sẽ tự động chạy với cấu hình đã lưu

## 🔗 Setup Services

### Telegram Bot

1. Tìm **@BotFather** trên Telegram
2. Gửi `/newbot` và làm theo hướng dẫn
3. Lưu **Bot Token** (dạng: `123456:ABC-DEF...`)
4. Tìm **@userinfobot** để lấy **Chat ID**

### Google Apps Script Webhook

**Tạo Apps Script:**

```javascript
function doPost(e) {
  const sheet = SpreadsheetApp.openById('YOUR_SPREADSHEET_ID').getActiveSheet();
  const data = JSON.parse(e.postData.contents);
  
  sheet.appendRow([
    new Date(),
    data.status,
    data.message,
    data.daily_count || '',
    data.version || '',
    data.ip || ''
  ]);
  
  return ContentService.createTextOutput(JSON.stringify({success: true}));
}
```

**Deploy:**
1. Click **Deploy** → **New deployment**
2. Type: **Web app**
3. Execute as: **Me**
4. Who has access: **Anyone**
5. Copy **Web app URL**

## 🚀 OTA Updates

### Tự động (GitHub Actions)

Mỗi khi push code mới:
1. GitHub Actions tự động build firmware
2. Tạo release mới với version từ `version.txt`
3. ESP32 tự động kiểm tra update mỗi 6 giờ
4. Download và cài đặt firmware mới
5. Restart với version mới

### Manual Release

```bash
# 1. Cập nhật version
echo "1.0.8" > version.txt

# 2. Commit và push
git add .
git commit -m "Release v1.0.8"
git push

# 3. GitHub Actions sẽ tự động build và release
```

## 📊 Monitoring

### Serial Monitor Output

```
╔════════════════════════════════════════╗
║   ESP32 Power Monitor v1.0.7           ║
║   + NVRAM + OTA + GitHub Releases      ║
╚════════════════════════════════════════╝
✓ Đã load credentials từ NVRAM
WiFi SSID: MyWiFi
Device: ESP32-Power-Monitor-PNP
Current Version: 1.0.7

⚡ NGUỒN BẬT - Lần #3 hôm nay

📋 Chế độ cảnh báo:
   1️⃣  Ban đêm: 21:30, mỗi 15 phút
   2️⃣  Hoạt động liên tục: >3h, mỗi 30 phút

[21:30] Alert:1 | Run:0h5m | Bật:#3
[21:45] Alert:2 | Run:0h20m | Bật:#3
```

### Telegram Notifications

```
⚡ NGUỒN BẬT
🔢 Lần #3
📦 v1.0.7
⏰ 30/12/2025 21:30:15

⚠️ CẢNH BÁO
🔌 Nguồn chưa tắt

🚨 KHẨN CẤP #2
🔌 Nguồn chưa tắt

⏰ HOẠT ĐỘNG LÂU
🔌 3h 15m
📊 Lần #1

🆕 ESP32-Power-Monitor-PNP
📦 1.0.6 → 1.0.7
🔄 Đang cập nhật...

✅ ESP32-Power-Monitor-PNP
📦 v1.0.7
💾 1032 KB
🔄 Khởi động lại...
```

## 🔧 Configuration

### Thay đổi thời gian cảnh báo

```cpp
const int NIGHT_CHECK_HOUR = 21;        // Giờ bắt đầu kiểm tra
const int NIGHT_CHECK_MINUTE = 30;      // Phút bắt đầu
const int NIGHT_ALERT_INTERVAL = 15;    // Cảnh báo mỗi 15 phút
```

### Thay đổi ngưỡng hoạt động liên tục

```cpp
const unsigned long LONG_RUN_THRESHOLD = 3 * 60 * 60 * 1000;  // 3 giờ
const unsigned long LONG_RUN_INTERVAL = 30 * 60 * 1000;       // 30 phút
```

### Thay đổi tần suất kiểm tra OTA

```cpp
const unsigned long OTA_CHECK_INTERVAL = 6 * 60 * 60 * 1000;  // 6 giờ
```

### Thiết bị thứ 2 (PNC)

Chỉ cần thay đổi device name:

```cpp
const char* DEVICE_NAME = "ESP32-Power-Monitor-PNC";
```

Mỗi thiết bị sẽ lưu credentials riêng vào NVRAM của nó.

## 🐛 Troubleshooting

### ESP32 không kết nối WiFi

```
✗ Không thể kết nối WiFi!
```

**Fix:**
- Kiểm tra SSID và password
- Đảm bảo WiFi là 2.4GHz (ESP32 không hỗ trợ 5GHz)
- Kiểm tra router có bật không

### OTA update thất bại

```
❌ HTTP GET failed: 302
```

**Fix:**
- Code mới đã xử lý redirect tự động
- Kiểm tra GitHub release đã có file `firmware.bin` và `version.txt`
- Test URL trên browser

### Serial Monitor không hiển thị gì

**Fix:**
- Kiểm tra baud rate: **115200**
- Nhấn nút **EN/RST** trên ESP32
- Thử cổng USB khác

### Không nhập được credentials

**Fix:**
- Đảm bảo baud rate: **115200** (không phải 921600)
- Line ending: **Newline** hoặc **Both NL & CR**
- Thử dùng `screen` hoặc `minicom` thay vì Arduino Serial Monitor

## 📁 Project Structure

```
esp32-power-monitor/
├── .github/
│   └── workflows/
│       └── build.yml          # GitHub Actions workflow
├── ESP32_Power_Monitor/
│   └── ESP32_Power_Monitor.ino # Main code
├── version.txt                 # Current version
├── firmware.bin               # Built firmware (auto-generated)
└── README.md                  # This file
```

## 🔐 Security Notes

- ⚠️ **Không commit WiFi password** vào Git
- ⚠️ **Không commit Telegram token** vào Git
- ✅ Tất cả credentials được lưu trong NVRAM của ESP32
- ✅ Chỉ setup một lần, không cần hardcode

## 📝 Version History

- **v1.0.7** - Fix OTA redirect handling
- **v1.0.6** - Add device name to notifications
- **v1.0.5** - Improve version management
- **v1.0.4** - Add OTA progress bar
- **v1.0.3** - Initial release with OTA

## 🤝 Contributing

Pull requests are welcome! For major changes, please open an issue first.

## 📄 License

MIT License - see LICENSE file for details

## 👤 Author

**Hong Huynh**
- GitHub: [@honghuynhit](https://github.com/honghuynhit)

## 🙏 Acknowledgments

- ESP32 Arduino Core
- Arduino HTTPClient
- Telegram Bot API
- Google Apps Script

---

Made with ❤️ for smart home automation
