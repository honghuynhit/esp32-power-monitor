# 🔌 ESP32 Power Monitor with OTA Update

Hệ thống giám sát nguồn điện tự động với khả năng cập nhật firmware từ xa qua GitHub Releases.

![Version](https://img.shields.io/badge/version-1.0.6-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![ESP32](https://img.shields.io/badge/platform-ESP32-red)

## ✨ Tính năng

- ⚡ **Giám sát nguồn điện** - Tự động theo dõi 24/7
- 🌙 **Cảnh báo ban đêm** - 21:30 hàng ngày, lặp mỗi 15 phút
- ⏰ **Cảnh báo hoạt động lâu** - Khi chạy liên tục > 3 giờ
- 📊 **Đếm số lần bật nguồn** - Theo dõi daily power-on count
- 📱 **Telegram notification** - Realtime alerts
- 📧 **Google Apps Script webhook** - Log dữ liệu vào Google Sheets
- 🔄 **OTA Update** - Cập nhật firmware từ GitHub Releases
- 💾 **NVRAM Storage** - Lưu credentials và version persistent
- 🚨 **Chế độ URGENT** - Từ lần cảnh báo thứ 2

## 📋 Yêu cầu phần cứng

- ESP32 Dev Module (hoặc bất kỳ variant nào)
- USB Type-C cable để upload code
- WiFi 2.4GHz network

## 🚀 Cài đặt

### Bước 1: Clone repository

```bash
git clone https://github.com/honghuynhit/esp32-power-monitor.git
cd esp32-power-monitor
```

### Bước 2: Setup lần đầu tiên

1. Mở file `.ino` trong Arduino IDE
2. **Uncomment** dòng này:
   ```cpp
   #define FIRST_TIME_SETUP
   ```
3. Upload lên ESP32
4. Mở Serial Monitor (115200 baud)
5. Nhập thông tin khi được yêu cầu:

```
1. WiFi SSID: YOUR_WIFI_NAME
2. WiFi Password: YOUR_WIFI_PASSWORD
3. Google Apps Script Webhook URL: YOUR_WEBHOOK_URL
4. Telegram Bot Token: YOUR_BOT_TOKEN
5. Telegram Chat ID: YOUR_CHAT_ID
6. Firmware Version URL: https://github.com/USER/REPO/releases/latest/download/version.txt
7. Firmware Binary URL: https://github.com/USER/REPO/releases/latest/download/firmware.bin
```

6. **Comment lại** dòng `#define FIRST_TIME_SETUP`
7. Upload lại code → ESP32 sẽ dùng credentials đã lưu

### Bước 3: Upload code chính thức

```cpp
// #define FIRST_TIME_SETUP  // ← Comment dòng này
```

Upload → ESP32 sẽ tự động load credentials từ NVRAM và bắt đầu hoạt động.

## 📱 Setup Telegram Bot

### Tạo Bot

1. Tìm **@BotFather** trong Telegram
2. Gửi lệnh: `/newbot`
3. Nhập tên bot (ví dụ: `ESP32 Power Monitor`)
4. Nhập username (ví dụ: `esp32_power_bot`)
5. Copy **Bot Token** (dạng: `123456789:ABCdefGHIjklMNOpqrsTUVwxyz`)

### Lấy Chat ID

1. Chat với bot của bạn và gửi `/start`
2. Truy cập URL này (thay `<TOKEN>` bằng bot token):
   ```
   https://api.telegram.org/bot<TOKEN>/getUpdates
   ```
3. Tìm `"chat":{"id":123456789}` và copy số đó

**Hoặc dùng @userinfobot:**
- Chat với @userinfobot
- Gửi bất kỳ tin nhắn nào
- Copy Chat ID

## 📧 Setup Google Apps Script Webhook

### Tạo Web App

1. Truy cập: https://script.google.com
2. Tạo **New Project**
3. Copy code sau vào Editor:

```javascript
function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var data = JSON.parse(e.postData.contents);
  
  sheet.appendRow([
    new Date(),
    data.status,
    data.message,
    data.alert_count || 0,
    data.daily_count || 0,
    data.version || "",
    data.time || ""
  ]);
  
  return ContentService.createTextOutput(JSON.stringify({result: "success"}));
}

function doGet(e) {
  return ContentService.createTextOutput("ESP32 Power Monitor Logger");
}
```

4. **Deploy** → **New deployment**:
   - Type: **Web app**
   - Execute as: **Me**
   - Who has access: **Anyone**
5. Copy **Web app URL**

### Tạo Google Sheet

1. Tạo Google Sheet mới
2. Thêm header row:
   ```
   Timestamp | Status | Message | Alert Count | Daily Count | Version | Time
   ```
3. Vào **Extensions → Apps Script**
4. Paste code ở trên
5. Deploy và copy URL

## 🔄 OTA Update qua GitHub Releases

### Workflow cập nhật firmware

#### 1. Sửa code và tăng version

```cpp
const char* FIRMWARE_VERSION = "1.0.7";  // Tăng từ 1.0.6 → 1.0.7
```

#### 2. Build firmware binary

**Arduino IDE:**
- `Sketch` → `Export Compiled Binary`
- File `.bin` sẽ xuất hiện trong thư mục sketch
- Đổi tên thành `firmware.bin`

**PlatformIO:**
```bash
pio run
# File output: .pio/build/esp32dev/firmware.bin
```

#### 3. Tạo file version.txt

```bash
echo "1.0.7" > version.txt
```

#### 4. Tạo GitHub Release

```bash
# Tag version
git tag v1.0.7
git push origin v1.0.7

# Hoặc tạo Release trên GitHub UI:
# 1. Vào Releases → Draft a new release
# 2. Tag: v1.0.7
# 3. Upload assets: firmware.bin và version.txt
# 4. Publish release
```

#### 5. ESP32 tự động update

- ESP32 check update mỗi **6 giờ**
- Hoặc ngay khi **restart**
- Telegram thông báo: `"🆕 Phát hiện update!"`
- Download → Flash → Restart
- Telegram xác nhận: `"✅ Cập nhật thành công!"`

### Kiểm tra update thủ công

Reset ESP32 → Xem Serial Monitor:

```
--- Kiểm tra OTA Update ---
Current version: 1.0.6
Latest version: 1.0.7
🆕 Có bản cập nhật mới!
   1.0.6 → 1.0.7
```

## 📂 Cấu trúc Project

```
esp32-power-monitor/
├── esp32-power-monitor.ino    # Main firmware code
├── version.txt                # Current version (for OTA)
├── firmware.bin               # Compiled binary (for OTA)
├── README.md                  # Documentation
└── .gitignore                 # Ignore build files
```

## 🎯 Cách hoạt động

### 1. Cảnh báo ban đêm (21:30)

```
21:30 → ⚠️  Cảnh báo #1 (Email + Telegram)
  ↓ (15 phút)
21:45 → 🚨 KHẨN CẤP #2 (Email + Telegram)
  ↓ (15 phút)
22:00 → 🚨 KHẨN CẤP #3
  ↓ (tiếp tục mỗi 15 phút...)
```

### 2. Cảnh báo hoạt động lâu

```
0h → Nguồn bật
3h → ⏰ HOẠT ĐỘNG LÂU #1 (3h 0m)
3h30m → ⏰ HOẠT ĐỘNG LÂU #2 (3h 30m)
4h → ⏰ HOẠT ĐỘNG LÂU #3 (4h 0m)
(Tiếp tục mỗi 30 phút...)
```

### 3. Đếm số lần bật nguồn

```
Lần #1 hôm nay → 08:00
Lần #2 hôm nay → 14:30
Lần #3 hôm nay → 19:00
...
```

Số đếm reset về 1 khi qua ngày mới.

### 4. OTA Update Flow

```
GitHub Release mới (v1.0.7)
         ↓
ESP32 check (mỗi 6h hoặc restart)
         ↓
Download version.txt → So sánh version
         ↓
Download firmware.bin (với progress bar)
         ↓
Flash firmware → Lưu version mới vào NVRAM
         ↓
Restart → Chạy version mới
         ↓
Telegram: "✅ Update thành công! v1.0.7"
```

## 🔧 Cấu hình nâng cao

### Thay đổi thời gian cảnh báo

```cpp
const int NIGHT_CHECK_HOUR = 22;        // Từ 21:30 → 22:00
const int NIGHT_CHECK_MINUTE = 0;
const int NIGHT_ALERT_INTERVAL = 10;    // Từ 15 phút → 10 phút
```

### Thay đổi ngưỡng hoạt động lâu

```cpp
// Từ 3 giờ → 2 giờ
const unsigned long LONG_RUN_THRESHOLD = 2 * 60 * 60 * 1000;

// Từ 30 phút → 20 phút
const unsigned long LONG_RUN_INTERVAL = 20 * 60 * 1000;
```

### Thay đổi tần suất check OTA

```cpp
// Mặc định: 6 giờ
const unsigned long OTA_CHECK_INTERVAL = 6 * 60 * 60 * 1000;

// Thay thành 1 giờ:
const unsigned long OTA_CHECK_INTERVAL = 1 * 60 * 60 * 1000;

// Thay thành 12 giờ:
const unsigned long OTA_CHECK_INTERVAL = 12 * 60 * 60 * 1000;
```

### Thay đổi múi giờ

```cpp
const long GMT_OFFSET_SEC = 7 * 3600;    // GMT+7 (Vietnam)
// GMT+8: 8 * 3600
// GMT+0: 0
// GMT-5: -5 * 3600
```

## 🐛 Troubleshooting

### ❌ ESP32 không kết nối WiFi

**Triệu chứng:**
```
Kết nối WiFi: YOUR_SSID
................
✗ Không thể kết nối WiFi!
```

**Giải pháp:**
1. Kiểm tra SSID và password đã đúng chưa
2. Đảm bảo WiFi là **2.4GHz** (ESP32 không hỗ trợ 5GHz)
3. Đưa ESP32 gần router hơn
4. Reset credentials bằng cách uncomment `FIRST_TIME_SETUP` và setup lại

### ❌ HTTP 302 (Redirect Error)

**Triệu chứng:**
```
--- Kiểm tra OTA Update ---
✗ Lỗi kiểm tra version: HTTP 302
```

**Giải pháp:**
- Code đã được fix với `http.setFollowRedirects()`
- Đảm bảo dùng code mới nhất
- Nếu vẫn lỗi, thử dùng raw GitHub URLs thay vì releases URLs

### ❌ Không nhận Telegram notification

**Kiểm tra:**
1. Bot token đúng format: `123456789:ABCdef...`
2. Chat ID đúng (là số, không có chữ)
3. Đã chat với bot và gửi `/start`
4. Test thủ công:
   ```bash
   curl -X POST "https://api.telegram.org/bot<TOKEN>/sendMessage" \
        -d "chat_id=<CHAT_ID>&text=Test"
   ```

### ❌ OTA Update thất bại

**Triệu chứng:**
```
📦 Content-Length: 12345 bytes
❌ File quá nhỏ: 12345 bytes
   Firmware ESP32 phải > 100KB
```

**Giải pháp:**
1. Đảm bảo upload đúng file `firmware.bin` (không phải file text)
2. File size phải > 100KB (firmware ESP32 thường ~200-800KB)
3. Kiểm tra GitHub Release có đúng file không
4. Thử download file bằng trình duyệt để kiểm tra

### ❌ Version không cập nhật sau OTA

**Kiểm tra:**
```cpp
// Trong setup()
Serial.println("Current Version: " + currentVersion);
```

**Giải pháp:**
- Version được lưu tự động vào NVRAM sau OTA thành công
- Nếu không update, có thể `Update.end()` thất bại
- Xem Serial Monitor có log error không

### 🔍 Debug mode

Bật verbose logging:

```cpp
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);  // ← Thêm dòng này
  // ...
}
```

## 📊 Monitoring & Logs

### Serial Monitor Output

```
╔════════════════════════════════════════╗
║   ESP32 Power Monitor v1.0.6           ║
║   + NVRAM + OTA + GitHub Releases      ║
╚════════════════════════════════════════╝
✓ Đã load credentials từ NVRAM
WiFi SSID: YOUR_WIFI
Current Version: 1.0.6

✓ Đã kết nối WiFi!
IP: 192.168.1.100

⚡ NGUỒN BẬT - Lần #5 hôm nay

📋 Chế độ cảnh báo:
   1️⃣  Ban đêm: 21:30, mỗi 15 phút
   2️⃣  Hoạt động liên tục: >3h, mỗi 30 phút

[14:23] Alert:0 | Run:2h15m | Bật:#5
```

### Telegram Notifications

```
⚡ NGUỒN BẬT
🔢 Lần #5
📦 v1.0.6
⏰ 07/12/2025 14:23:45

⏰ HOẠT ĐỘNG LÂU
🔌 3h 30m
📊 Lần #2

⚠️ CẢNH BÁO
🔌 Nguồn chưa tắt

🚨 KHẨN CẤP #3
🔌 Nguồn chưa tắt

🆕 Phát hiện update!
📦 1.0.6 → 1.0.7
🔄 Đang cập nhật...

✅ Cập nhật thành công!
📦 v1.0.7
💾 345 KB
🔄 Khởi động lại...
```

## 📝 Version History

### v1.0.6 (Current)
- ✅ Xử lý HTTP redirect từ GitHub Releases
- ✅ Cache-busting cho OTA download
- ✅ Version management cải tiến
- ✅ Progress bar khi download firmware
- ✅ Validation chặt chẽ (file size, content-length)

### v1.0.5
- ✨ NVRAM storage cho credentials
- ✨ First-time setup mode
- ✨ OTA update từ GitHub

### v1.0.0 (Initial Release)
- ⚡ Giám sát nguồn điện cơ bản
- 📧 Gmail notification
- 📱 Telegram integration
- ⏰ Cảnh báo ban đêm

## 🔐 Security Notes

- Credentials được lưu trong **NVRAM** (không mã hóa)
- **Không commit** file chứa credentials lên GitHub
- Dùng **GitHub Secrets** nếu cần CI/CD
- Telegram Bot Token nên giữ bí mật

## 🤝 Contributing

Contributions are welcome! 

1. Fork repository
2. Tạo branch mới: `git checkout -b feature/amazing-feature`
3. Commit changes: `git commit -m 'Add amazing feature'`
4. Push to branch: `git push origin feature/amazing-feature`
5. Open Pull Request

## 📄 License

MIT License - free to use for personal and commercial projects.

```
Copyright (c) 2025 honghuynhit

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction...
```

## 👤 Author

**Huynh Hong** (honghuynhit)
- GitHub: [@honghuynhit](https://github.com/honghuynhit)
- Email: your.email@example.com

## 🙏 Acknowledgments

- [Arduino ESP32 Core](https://github.com/espressif/arduino-esp32)
- [Telegram Bot API](https://core.telegram.org/bots/api)
- [Google Apps Script](https://developers.google.com/apps-script)
- ESP32 Community

## 📞 Support

Nếu gặp vấn đề:
1. Kiểm tra [Troubleshooting](#-troubleshooting)
2. Xem [Issues](https://github.com/honghuynhit/esp32-power-monitor/issues)
3. Tạo Issue mới với log đầy đủ

---

⭐ Nếu project hữu ích, hãy cho một **Star** nhé!

💡 **Tips:** Nhớ comment `#define FIRST_TIME_SETUP` sau khi setup xong!