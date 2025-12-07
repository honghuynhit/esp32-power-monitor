# 🔌 ESP32 Power Monitor with OTA Update

Hệ thống giám sát nguồn điện tự động với khả năng cập nhật firmware từ xa (OTA).

![Build Status](https://github.com/honghuynhit/esp32-power-monitor/actions/workflows/build.yml/badge.svg)

## ✨ Tính năng

- ⚡ **Giám sát nguồn điện** tự động vào 21:30 hàng ngày
- 📧 **Gửi email** cảnh báo qua Gmail
- 📱 **Telegram notification** realtime
- 🔄 **OTA Update** - Cập nhật firmware từ xa
- ⏰ **Cảnh báo lặp lại** mỗi 15 phút nếu chưa tắt nguồn
- 🚨 **Chế độ URGENT** từ lần cảnh báo thứ 2

## 📋 Yêu cầu phần cứng

- ESP32 (bất kỳ variant nào)
- USB Type-C cable
- WiFi 2.4GHz

## 🚀 Cài đặt

### 1. Clone repository

```bash
git clone https://github.com/honghuynhit/esp32-power-monitor.git
cd esp32-power-monitor
```

### 2. Cấu hình WiFi & Services

Mở file `.ino` và sửa các thông tin:

```cpp
// WiFi
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Telegram Bot
const char* telegramToken = "YOUR_BOT_TOKEN";
const char* telegramChatID = "YOUR_CHAT_ID";

// Gmail Webhook (Google Apps Script)
const char* webhookURL = "YOUR_GOOGLE_APPS_SCRIPT_URL";
```

### 3. Upload lên ESP32

Sử dụng Arduino IDE:
- Board: ESP32 Dev Module
- Upload Speed: 921600
- Flash Frequency: 80MHz

## 🔄 OTA Update

### Tự động (Khuyến nghị)

Mỗi khi bạn push code lên GitHub, **GitHub Actions** sẽ tự động:
1. Build firmware
2. Tạo file `firmware.bin`
3. Commit và push lại repo
4. ESP32 sẽ tự động nhận update trong vòng 6 giờ

### Thủ công

1. Sửa code và tăng version trong `version.txt`
2. Build firmware: `Sketch → Export compiled Binary`
3. Đổi tên file `.bin` thành `firmware.bin`
4. Commit và push lên GitHub

### Kiểm tra update ngay

Restart ESP32 → Sẽ check update ngay khi khởi động

## 📱 Setup Telegram Bot

1. Tìm **@BotFather** trong Telegram
2. Gửi `/newbot` và làm theo hướng dẫn
3. Copy **Token**
4. Chat với bot của bạn và gửi `/start`
5. Lấy **Chat ID** từ: `https://api.telegram.org/bot<TOKEN>/getUpdates`

## 📧 Setup Gmail (Google Apps Script)

1. Vào https://script.google.com
2. Tạo project mới
3. Copy code từ `google-apps-script.js`
4. Deploy as Web App (Execute as: Me, Access: Anyone)
5. Copy URL và paste vào `webhookURL`

## 📂 Cấu trúc thư mục

```
esp32-power-monitor/
├── .github/
│   └── workflows/
│       └── build.yml          # GitHub Actions workflow
├── esp32_power_monitor.ino    # Main sketch
├── firmware.bin               # Binary firmware (auto-generated)
├── version.txt                # Version hiện tại
└── README.md                  # Documentation
```

## 🎯 Cách hoạt động

### Timeline hàng ngày

```
21:30 → Email & Telegram #1 ⚠️  (Cảnh báo lần đầu)
  ↓ (chờ 15 phút)
21:45 → Email & Telegram #2 🚨 (URGENT)
  ↓ (chờ 15 phút)
22:00 → Email & Telegram #3 🚨 (URGENT)
  ↓ (tiếp tục mỗi 15 phút...)
```

### OTA Update Flow

```
Code thay đổi → Push to GitHub
       ↓
GitHub Actions tự động build
       ↓
firmware.bin được update
       ↓
ESP32 check (mỗi 6h hoặc khi restart)
       ↓
Tải firmware mới → Update → Restart
       ↓
Telegram: "✅ Update thành công!"
```

## 🔧 Cấu hình

### Thay đổi thời gian kiểm tra

```cpp
const int START_HOUR = 21;        // Giờ bắt đầu
const int START_MINUTE = 30;      // Phút bắt đầu
const int ALERT_INTERVAL = 15;    // Gửi lại mỗi X phút
```

### Thay đổi tần suất check OTA

```cpp
// Mặc định: 6 giờ
const unsigned long OTA_CHECK_INTERVAL = 6 * 60 * 60 * 1000;

// Thay thành 1 giờ:
const unsigned long OTA_CHECK_INTERVAL = 1 * 60 * 60 * 1000;
```

## 🐛 Troubleshooting

### ESP32 không update firmware

- Kiểm tra Serial Monitor xem có log lỗi không
- Đảm bảo repository là **Public**
- Test URL trong trình duyệt: `https://raw.githubusercontent.com/honghuynhit/esp32-power-monitor/main/firmware.bin`

### Không nhận được Telegram

- Kiểm tra token và chat ID
- Đảm bảo đã chat với bot và nhấn `/start`
- Test bằng cách gọi API trực tiếp

### Build thất bại trên GitHub Actions

- Kiểm tra file `.ino` có lỗi cú pháp không
- Xem log trong tab **Actions**
- Đảm bảo `version.txt` tồn tại

## 📊 Monitoring

Xem build status và logs:
- GitHub Actions: https://github.com/honghuynhit/esp32-power-monitor/actions
- Serial Monitor: 115200 baud

## 📝 Changelog

### v1.0.0 (Initial Release)
- ✨ Giám sát nguồn điện tự động
- 📧 Gmail notification
- 📱 Telegram integration
- 🔄 OTA Update từ GitHub
- ⏰ Cảnh báo lặp lại mỗi 15 phút

## 🤝 Contributing

Pull requests are welcome! For major changes, please open an issue first.

## 📄 License

MIT License - feel free to use this project for your own purposes.

## 👤 Author

**honghuynhit**
- GitHub: [@honghuynhit](https://github.com/honghuynhit)

## 🙏 Acknowledgments

- Arduino ESP32 Core
- GitHub Actions
- Telegram Bot API

---

⭐ Nếu project này hữu ích, hãy cho một star nhé!