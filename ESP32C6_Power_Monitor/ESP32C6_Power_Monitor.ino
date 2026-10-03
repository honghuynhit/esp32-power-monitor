#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <Preferences.h>

// ========== CONFIGURATION MODE ==========
// Uncomment để vào chế độ setup credentials lần đầu
// #define FIRST_TIME_SETUP

Preferences preferences;
Preferences configStore;

// ========== Biến lưu credentials từ NVRAM ==========
String wifiSSID;
String wifiPassword;
String webhookURL;
String telegramToken;
String telegramChatID;
String firmwareVersionURL;
String firmwareBinURL;

// ========== Settings ==========
const char* DEVICE_NAME = "ESP32-Power-Monitor-PNP";
const char* DEVICE_SHORT_NAME = "PNP";  // Tên ngắn gọn cho thông báo
const char* FIRMWARE_VERSION = "1.0.9";  // ⚠️ CHỈ DÙNG KHI NVRAM TRỐNG
String currentVersion;  // Version thực tế (luôn từ NVRAM)

const int NIGHT_CHECK_HOUR = 21;
const int NIGHT_CHECK_MINUTE = 30;
const int NIGHT_ALERT_INTERVAL = 15;
const unsigned long LONG_RUN_THRESHOLD = 3 * 60 * 60 * 1000;
const unsigned long LONG_RUN_INTERVAL = 30 * 60 * 1000;
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 7 * 3600;
const int DAYLIGHT_OFFSET_SEC = 0;

// ========== Runtime variables ==========
int alertCount = 0;
int lastCheckedDay = -1;
int lastAlertMinute = -1;
unsigned long powerOnStartTime = 0;
unsigned long lastLongRunAlert = 0;
int longRunAlertCount = 0;
int dailyPowerOnCount = 0;
int savedDay = 0;
unsigned long lastOTACheck = 0;
const unsigned long OTA_CHECK_INTERVAL = 6 * 60 * 60 * 1000;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║   ESP32 Power Monitor v1.0.7           ║");
  Serial.println("║   + NVRAM + OTA Fixed                  ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  // Khởi tạo NVRAM storage
  configStore.begin("config", false);
  preferences.begin("power-monitor", false);
  
  // ✅ FIX: CHỈ ĐỌC VERSION TỪ NVRAM, KHÔNG GHI ĐÈ
  currentVersion = configStore.getString("current_ver", "");
  
  if (currentVersion.length() == 0) {
    // Lần đầu tiên chạy → lưu version từ code
    Serial.println("🆕 Lần đầu khởi động, lưu version: " + String(FIRMWARE_VERSION));
    currentVersion = String(FIRMWARE_VERSION);
    configStore.putString("current_ver", currentVersion);
  } else {
    // Đã có version trong NVRAM → dùng version đó
    Serial.println("📦 Version từ NVRAM: " + currentVersion);
  }
  
#ifdef FIRST_TIME_SETUP
  // ========== CHẾ ĐỘ SETUP LẦN ĐẦU ==========
  Serial.println("\n🔧 FIRST TIME SETUP MODE");
  Serial.println("Nhập credentials qua Serial Monitor...\n");
  
  setupCredentials();
  
  Serial.println("\n✓ Setup hoàn tất!");
  Serial.println("→ Comment dòng #define FIRST_TIME_SETUP");
  Serial.println("→ Upload lại code");
  Serial.println("→ ESP32 sẽ dùng credentials đã lưu");
  while(1) delay(1000);
#endif
  
  // ========== LOAD CREDENTIALS TỪ NVRAM ==========
  if (!loadCredentials()) {
    Serial.println("\n❌ CHƯA CÓ CREDENTIALS!");
    Serial.println("→ Uncomment #define FIRST_TIME_SETUP");
    Serial.println("→ Upload lại để setup");
    while(1) {
      delay(1000);
      Serial.print(".");
    }
  }
  
  Serial.println("✓ Đã load credentials từ NVRAM");
  Serial.println("WiFi SSID: " + wifiSSID);
  Serial.println("Device: " + String(DEVICE_NAME));
  Serial.println("Current Version: " + currentVersion);
  
  // Kết nối WiFi
  connectWiFi();
  
  // Đồng bộ thời gian
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  Serial.println("Đang đồng bộ thời gian...");
  struct tm timeinfo;
  int timeoutCount = 0;
  while (!getLocalTime(&timeinfo) && timeoutCount < 20) {
    delay(500);
    Serial.print(".");
    timeoutCount++;
  }
  
  if (timeoutCount >= 20) {
    Serial.println("\n✗ Không thể đồng bộ thời gian!");
  } else {
    Serial.println("\n✓ Đã đồng bộ thời gian!");
    Serial.println(&timeinfo, "%A, %d/%m/%Y %H:%M:%S");
  }
  
  // Kiểm tra daily count
  savedDay = preferences.getInt("day", 0);
  int currentDay = timeinfo.tm_mday;
  
  if (savedDay != currentDay) {
    dailyPowerOnCount = 1;
    preferences.putInt("day", currentDay);
    preferences.putInt("count", 1);
  } else {
    dailyPowerOnCount = preferences.getInt("count", 0) + 1;
    preferences.putInt("count", dailyPowerOnCount);
  }
  
  Serial.printf("\n⚡ NGUỒN %s BẬT - Lần #%d hôm nay\n", DEVICE_SHORT_NAME, dailyPowerOnCount);
  powerOnStartTime = millis();
  
  Serial.printf("\n📋 Chế độ cảnh báo:\n");
  Serial.printf("   1️⃣  Ban đêm: %02d:%02d, mỗi %d phút\n", 
                NIGHT_CHECK_HOUR, NIGHT_CHECK_MINUTE, NIGHT_ALERT_INTERVAL);
  Serial.printf("   2️⃣  Hoạt động liên tục: >3h, mỗi 30 phút\n");
  
  sendPowerOnLog(timeinfo);
  checkForOTAUpdate();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠ WiFi mất kết nối...");
    connectWiFi();
  }
  
  if (millis() - lastOTACheck > OTA_CHECK_INTERVAL) {
    checkForOTAUpdate();
    lastOTACheck = millis();
  }
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    delay(60000);
    return;
  }
  
  if (timeinfo.tm_mday != lastCheckedDay) {
    alertCount = 0;
    lastCheckedDay = timeinfo.tm_mday;
    lastAlertMinute = -1;
  }
  
  unsigned long runTime = millis() - powerOnStartTime;
  
  if (runTime > LONG_RUN_THRESHOLD) {
    unsigned long timeSinceLastAlert = millis() - lastLongRunAlert;
    
    if (lastLongRunAlert == 0 || timeSinceLastAlert >= LONG_RUN_INTERVAL) {
      longRunAlertCount++;
      lastLongRunAlert = millis();
      
      unsigned long hours = runTime / (60 * 60 * 1000);
      unsigned long minutes = (runTime % (60 * 60 * 1000)) / (60 * 1000);
      
      sendLongRunAlert(hours, minutes, longRunAlertCount);
    }
  }
  
  bool shouldAlert = checkIfShouldAlert(timeinfo);
  
  if (shouldAlert && timeinfo.tm_min != lastAlertMinute) {
    alertCount++;
    lastAlertMinute = timeinfo.tm_min;
    bool isUrgent = (alertCount > 1);
    
    // sendWebhookAlert(alertCount, isUrgent);
    sendTelegramAlert(alertCount, isUrgent);
  }
  
  static int lastMinute = -1;
  if (timeinfo.tm_min != lastMinute) {
    lastMinute = timeinfo.tm_min;
    
    unsigned long runTime = millis() - powerOnStartTime;
    unsigned long hours = runTime / (60 * 60 * 1000);
    unsigned long minutes = (runTime % (60 * 60 * 1000)) / (60 * 1000);
    
    Serial.printf("[%02d:%02d] Alert:%d | Run:%luh%lum | Bật:#%d\n", 
                  timeinfo.tm_hour, timeinfo.tm_min, alertCount, hours, minutes, dailyPowerOnCount);
  }
  
  // ✅ DEBUG COMMANDS QUA SERIAL
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    
    if (cmd == "version") {
      Serial.println("\n╔═══════════════════════════════════════╗");
      Serial.println("║  VERSION INFO                         ║");
      Serial.println("╚═══════════════════════════════════════╝");
      Serial.println("Code hardcoded: " + String(FIRMWARE_VERSION));
      Serial.println("NVRAM stored:   " + configStore.getString("current_ver", "N/A"));
      Serial.println("Current active: " + currentVersion);
    }
    
    if (cmd == "reset_version") {
      Serial.println("\n⚠️  RESET VERSION VỀ CODE DEFAULT");
      configStore.putString("current_ver", FIRMWARE_VERSION);
      Serial.println("✓ Đã reset về: " + String(FIRMWARE_VERSION));
      Serial.println("🔄 Khởi động lại...");
      delay(2000);
      ESP.restart();
    }
    
    if (cmd == "force_ota") {
      Serial.println("\n🔄 FORCE CHECK OTA...");
      checkForOTAUpdate();
    }
    
    if (cmd == "help") {
      Serial.println("\n╔═══════════════════════════════════════╗");
      Serial.println("║  SERIAL COMMANDS                      ║");
      Serial.println("╚═══════════════════════════════════════╝");
      Serial.println("version       - Xem thông tin version");
      Serial.println("reset_version - Reset về code default");
      Serial.println("force_ota     - Kiểm tra OTA ngay");
      Serial.println("help          - Hiển thị menu này");
    }
  }
  
  delay(1000);
}

// ============ NVRAM CREDENTIALS MANAGEMENT ============

void setupCredentials() {
  Serial.println("\n=== SETUP CREDENTIALS ===\n");
  
  Serial.println("1. WiFi SSID:");
  String ssid = readSerialInput();
  configStore.putString("wifi_ssid", ssid);
  
  Serial.println("2. WiFi Password:");
  String pass = readSerialInput();
  configStore.putString("wifi_pass", pass);
  
  Serial.println("3. Google Apps Script Webhook URL:");
  String webhook = readSerialInput();
  configStore.putString("webhook", webhook);
  
  Serial.println("4. Telegram Bot Token:");
  String token = readSerialInput();
  configStore.putString("tg_token", token);
  
  Serial.println("5. Telegram Chat ID:");
  String chatid = readSerialInput();
  configStore.putString("tg_chatid", chatid);
  
  Serial.println("6. Firmware Version URL:");
  Serial.println("   (VD: https://github.com/USER/REPO/releases/latest/download/version.txt)");
  String verUrl = readSerialInput();
  configStore.putString("ver_url", verUrl);
  
  Serial.println("7. Firmware Binary URL:");
  Serial.println("   (VD: https://github.com/USER/REPO/releases/latest/download/firmware.bin)");
  String binUrl = readSerialInput();
  configStore.putString("bin_url", binUrl);
  
  configStore.putBool("configured", true);
  
  Serial.println("\n✓ Credentials đã được lưu vào NVRAM!");
}

bool loadCredentials() {
  if (!configStore.getBool("configured", false)) {
    return false;
  }
  
  wifiSSID = configStore.getString("wifi_ssid", "");
  wifiPassword = configStore.getString("wifi_pass", "");
  webhookURL = configStore.getString("webhook", "");
  telegramToken = configStore.getString("tg_token", "");
  telegramChatID = configStore.getString("tg_chatid", "");
  firmwareVersionURL = configStore.getString("ver_url", "");
  firmwareBinURL = configStore.getString("bin_url", "");
  
  return (wifiSSID.length() > 0);
}

String readSerialInput() {
  String input = "";
  Serial.print("> ");
  
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (input.length() > 0) {
          Serial.println();
          return input;
        }
      } else {
        input += c;
        Serial.print(c);
      }
    }
    delay(10);
  }
}

// ============ HELPER FUNCTIONS ============

void connectWiFi() {
  Serial.println("Kết nối WiFi: " + wifiSSID);
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ Đã kết nối WiFi!");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n✗ Không thể kết nối WiFi!");
  }
}

void sendPowerOnLog(struct tm timeinfo) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%d/%m/%Y %H:%M:%S", &timeinfo);
  
  HTTPClient http;
  http.begin(webhookURL.c_str());
  http.addHeader("Content-Type", "application/json");
  
  String jsonData = "{";
  jsonData += "\"status\":\"power_on\",";
  jsonData += "\"device\":\"" + String(DEVICE_SHORT_NAME) + "\",";
  jsonData += "\"message\":\"Nguồn " + String(DEVICE_SHORT_NAME) + " bật - Lần #" + String(dailyPowerOnCount) + "\",";
  jsonData += "\"daily_count\":" + String(dailyPowerOnCount) + ",";
  jsonData += "\"version\":\"" + currentVersion + "\",";
  jsonData += "\"time\":\"" + String(timeStr) + "\",";
  jsonData += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
  jsonData += "}";
  
  // http.POST(jsonData);
  http.end();
  
  String teleMsg = "⚡ Nguồn " + String(DEVICE_SHORT_NAME) + " BẬT\n🔢 Lần #" + String(dailyPowerOnCount) + "\n📦 v" + currentVersion + "\n⏰ " + String(timeStr);
  sendTelegramMessage(teleMsg);
}

void sendLongRunAlert(unsigned long hours, unsigned long minutes, int count) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%d/%m/%Y %H:%M:%S", &timeinfo);
  
  HTTPClient http;
  http.begin(webhookURL.c_str());
  http.addHeader("Content-Type", "application/json");
  
  String jsonData = "{";
  jsonData += "\"status\":\"long_running\",";
  jsonData += "\"device\":\"" + String(DEVICE_SHORT_NAME) + "\",";
  jsonData += "\"alert_count\":" + String(count) + ",";
  jsonData += "\"run_time_hours\":" + String(hours) + ",";
  jsonData += "\"run_time_minutes\":" + String(minutes) + ",";
  jsonData += "\"message\":\"" + String(DEVICE_SHORT_NAME) + " hoạt động " + String(hours) + "h " + String(minutes) + "m\",";
  jsonData += "\"time\":\"" + String(timeStr) + "\"";
  jsonData += "}";
  
  // http.POST(jsonData);
  http.end();
  
  String teleMsg = "⏰ " + String(DEVICE_SHORT_NAME) + " HOẠT ĐỘNG LÂU\n🔌 " + String(hours) + "h " + String(minutes) + "m\n📊 Lần #" + String(count);
  sendTelegramMessage(teleMsg);
}

// ============ OTA UPDATE ============

void checkForOTAUpdate() {
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  🔍 KIỂM TRA OTA UPDATE               ║");
  Serial.println("╚═══════════════════════════════════════╝");
  Serial.println("📦 Current version: " + currentVersion);
  
  WiFiClientSecure client;
  client.setInsecure();
  
  HTTPClient http;
  
  String versionURL = firmwareVersionURL;
  if (versionURL.indexOf('?') == -1) {
    versionURL += "?t=" + String(millis());
  } else {
    versionURL += "&t=" + String(millis());
  }
  
  Serial.println("🌐 URL: " + firmwareVersionURL);
  
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setRedirectLimit(10);
  
  http.begin(client, versionURL.c_str());
  http.addHeader("Cache-Control", "no-cache");
  
  int httpCode = http.GET();
  
  Serial.printf("📡 HTTP Response: %d", httpCode);
  
  if (httpCode == 200) {
    String latestVersion = http.getString();
    latestVersion.trim();
    
    Serial.println(" ✓");
    Serial.println("🆕 Latest version: " + latestVersion);
    
    if (latestVersion.length() == 0) {
      Serial.println("❌ Version file trống!");
      http.end();
      return;
    }
    
    if (latestVersion != currentVersion) {
      Serial.println("\n╔═══════════════════════════════════════╗");
      Serial.println("║  🎉 CÓ BẢN CẬP NHẬT MỚI!             ║");
      Serial.println("╚═══════════════════════════════════════╝");
      Serial.println("   📦 " + currentVersion + " → " + latestVersion);
      
      sendTelegramMessage("🆕 " + String(DEVICE_SHORT_NAME) + " phát hiện update!\n📦 " + currentVersion + " → " + latestVersion + "\n🔄 Đang cập nhật...");
      
      performOTAUpdate(latestVersion);
    } else {
      Serial.println("✓ Đã là phiên bản mới nhất");
    }
  } else if (httpCode > 0) {
    Serial.println(" ✗");
    Serial.printf("❌ HTTP Error: %d\n", httpCode);
    
    if (httpCode >= 300 && httpCode < 400) {
      Serial.println("⚠️  Redirect không được xử lý!");
    } else if (httpCode == 404) {
      Serial.println("⚠️  File version.txt không tồn tại!");
    }
  } else {
    Serial.println(" ✗");
    Serial.printf("❌ Connection failed: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

void performOTAUpdate(String newVersion) {
  Serial.println("\n╔═══════════════════════════════════════╗");
  Serial.println("║  📥 DOWNLOAD FIRMWARE                 ║");
  Serial.println("╚═══════════════════════════════════════╝");
  
  WiFiClientSecure client;
  client.setInsecure();
  
  HTTPClient http;
  
  String url = firmwareBinURL;
  if (url.indexOf('?') == -1) {
    url += "?t=" + String(millis());
  } else {
    url += "&t=" + String(millis());
  }
  
  Serial.println("🌐 URL: " + firmwareBinURL);
  
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setRedirectLimit(10);
  
  http.begin(client, url.c_str());
  http.addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  http.addHeader("Pragma", "no-cache");
  http.addHeader("Expires", "0");
  
  int httpCode = http.GET();
  
  Serial.printf("📡 HTTP Response: %d", httpCode);
  
  if (httpCode == 200) {
    Serial.println(" ✓");
    
    int contentLength = http.getSize();
    
    Serial.printf("📦 Content-Length: %d bytes (%.2f KB)\n", contentLength, contentLength / 1024.0);
    
    if (contentLength <= 0) {
      Serial.println("❌ Không lấy được kích thước firmware!");
      http.end();
      return;
    }
    
    if (contentLength < 100000) {
      Serial.printf("❌ File quá nhỏ: %d bytes (%.2f KB)\n", contentLength, contentLength / 1024.0);
      http.end();
      return;
    }
    
    if (!Update.begin(contentLength)) {
      Serial.println("❌ Không đủ bộ nhớ để update!");
      http.end();
      return;
    }
    
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║  🔄 ĐANG CẬP NHẬT FIRMWARE...         ║");
    Serial.println("╚═══════════════════════════════════════╝\n");
    
    WiFiClient * stream = http.getStreamPtr();
    uint8_t buff[128] = { 0 };
    size_t written = 0;
    int lastPercent = -1;
    unsigned long startTime = millis();
    
    while (http.connected() && (written < contentLength)) {
      size_t available = stream->available();
      
      if (available) {
        int c = stream->readBytes(buff, min(available, sizeof(buff)));
        
        if (c > 0) {
          Update.write(buff, c);
          written += c;
          
          int percent = (written * 100) / contentLength;
          
          if (percent != lastPercent && percent % 5 == 0) {
            Serial.println("\n");
            Serial.print("\r[");
            int bars = percent / 5;
            for (int i = 0; i < 20; i++) {
              if (i < bars) Serial.print("█");
              else Serial.print("░");
            }
            
            unsigned long elapsed = millis() - startTime;
            float speed = (elapsed > 0) ? (written / 1024.0) / (elapsed / 1000.0) : 0;
            
            Serial.printf("] %3d%% | %d/%d KB | %.1f KB/s", 
                         percent, 
                         written / 1024, 
                         contentLength / 1024,
                         speed);
            
            lastPercent = percent;
          }
        }
      }
      delay(1);
    }
    
    Serial.println();
    
    if (Update.end(true)) {
      if (Update.isFinished()) {
        unsigned long elapsed = millis() - startTime;
        
        Serial.println("\n╔═══════════════════════════════════════╗");
        Serial.println("║  ✅ CẬP NHẬT THÀNH CÔNG!              ║");
        Serial.println("╚═══════════════════════════════════════╝");
        Serial.printf("📊 Đã ghi: %d bytes trong %.1f giây\n", written, elapsed / 1000.0);
        Serial.printf("⚡ Tốc độ: %.2f KB/s\n", (written / 1024.0) / (elapsed / 1000.0));
        
        // ✅ LƯU VERSION MỚI VÀO NVRAM
        configStore.putString("current_ver", newVersion);
        Serial.println("💾 Đã lưu version mới vào NVRAM: " + newVersion);
        
        sendTelegramMessage("✅ " + String(DEVICE_SHORT_NAME) + " cập nhật thành công!\n📦 v" + newVersion + "\n💾 " + String(contentLength / 1024) + " KB\n🔄 Khởi động lại...");
        
        Serial.println("\n🔄 Khởi động lại trong 3 giây...");
        delay(3000);
        ESP.restart();
      } else {
        Serial.println("\n❌ Update không hoàn tất!");
      }
    } else {
      Serial.println("\n❌ Update.end() thất bại!");
      Serial.printf("Error: %s\n", Update.errorString());
    }
  } else if (httpCode > 0) {
    Serial.println(" ✗");
    Serial.printf("❌ HTTP Error: %d\n", httpCode);
    
    if (httpCode >= 300 && httpCode < 400) {
      Serial.println("⚠️  Redirect không được xử lý!");
    } else if (httpCode == 404) {
      Serial.println("⚠️  File firmware.bin không tồn tại!");
    }
  } else {
    Serial.println(" ✗");
    Serial.printf("❌ Connection failed: %s\n", http.errorToString(httpCode).c_str());
  }
  
  http.end();
}

bool checkIfShouldAlert(struct tm timeinfo) {
  if (timeinfo.tm_hour < NIGHT_CHECK_HOUR) return false;
  if (timeinfo.tm_hour == NIGHT_CHECK_HOUR && timeinfo.tm_min < NIGHT_CHECK_MINUTE) return false;
  
  int minutesSinceStart = calculateMinutesSinceStart(timeinfo);
  return (minutesSinceStart >= 0 && minutesSinceStart % NIGHT_ALERT_INTERVAL == 0);
}

int calculateMinutesSinceStart(struct tm timeinfo) {
  int currentMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  int startMinutes = NIGHT_CHECK_HOUR * 60 + NIGHT_CHECK_MINUTE;
  return currentMinutes - startMinutes;
}

void sendWebhookAlert(int count, bool isUrgent) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  HTTPClient http;
  http.begin(webhookURL.c_str());
  http.addHeader("Content-Type", "application/json");
  
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%d/%m/%Y %H:%M:%S", &timeinfo);
  
  String jsonData = "{";
  jsonData += "\"status\":\"power_on\",";
  jsonData += "\"device\":\"" + String(DEVICE_SHORT_NAME) + "\",";
  jsonData += "\"alert_count\":" + String(count) + ",";
  jsonData += "\"is_urgent\":" + String(isUrgent ? "true" : "false") + ",";
  jsonData += "\"message\":\"" + String(DEVICE_SHORT_NAME) + " cảnh báo #" + String(count) + "\",";
  jsonData += "\"time\":\"" + String(timeStr) + "\"";
  jsonData += "}";
  
  // http.POST(jsonData);
  http.end();
}

void sendTelegramAlert(int count, bool isUrgent) {
  String message = isUrgent ? "🚨 " + String(DEVICE_SHORT_NAME) + " KHẨN CẤP #" + String(count) : "⚠️ " + String(DEVICE_SHORT_NAME) + " CẢNH BÁO";
  message += "\n🔌 Nguồn chưa tắt";
  sendTelegramMessage(message);
}

void sendTelegramMessage(String message) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  WiFiClientSecure client;
  client.setInsecure();
  
  String url = "https://api.telegram.org/bot" + telegramToken + "/sendMessage";
  
  HTTPClient http;
  http.begin(client, url.c_str());
  http.addHeader("Content-Type", "application/json");
  
  String payload = "{\"chat_id\":\"" + telegramChatID + "\",\"text\":\"" + message + "\"}";
  
  http.POST(payload);
  http.end();
}
