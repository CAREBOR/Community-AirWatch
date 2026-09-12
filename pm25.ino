#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

// -------- ตั้งค่า Wi-Fi และ LINE Notify --------
const char* ssid = "YOUR_WIFI_SSID";          // เปลี่ยนเป็นชื่อ Wi-Fi ของคุณ
const char* password = "YOUR_WIFI_PASSWORD";  // เปลี่ยนเป็นรหัสผ่าน Wi-Fi ของคุณ

//Token: https://notify-bot.line.me/
#define LINE_TOKEN "YOUR_LINE_NOTIFY_TOKEN"   // ใส่ LINE Notify Token ของคุณที่นี่

// -------- ตั้งค่าจอ OLED --------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// -------- ตั้งค่าเซนเซอร์ DHT22 --------
#define DHTPIN 4
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);

// -------- ตั้งค่าเซนเซอร์ฝุ่น PMS5003 --------
#define RX_PIN 16
#define TX_PIN 17
HardwareSerial pmsSerial(2);

// -------- ค่ากำหนดการแจ้งเตือน --------
const int PM25_THRESHOLD = 50;  // กำหนดเกณฑ์แจ้งเตือน PM2.5 (เช่น เกิน 50 µg/m³ ส่งไลน์เตือน)
unsigned long lastLineSendTime = 0;
const unsigned long lineInterval = 300000; // ส่งไลน์ซ้ำได้เร็วที่สุดทุกๆ 5 นาที (ป้องกันข้อความรัวเกินไป)

struct PMData {
  int pm2_5;
  int pm10;
};

void setup() {
  Serial.begin(115200);
  pmsSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  
  // เริ่มต้นใช้งาน DHT
  dht.begin();

  // เริ่มต้นใช้งานจอ OLED (I2C Address ปกติคือ 0x3C)
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Community-AirWatch"));
  display.println(F("Connecting WiFi..."));
  display.display();

  // เชื่อมต่อ Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println(F("WiFi connected!"));
  
  display.setCursor(0, 30);
  display.println(F("WiFi Connected!"));
  display.display();
  delay(2000);
}

void loop() {
  // 1. อ่านค่าจาก DHT22
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  }

  // 2. อ่านค่าจาก PMS5003
  PMData pm = readPMS5003();

  // 3. แสดงผลผ่าน Serial Monitor
  Serial.println(F("--- AirWatch Status ---"));
  Serial.print(F("Temp: ")); Serial.print(temperature); Serial.println(F(" C"));
  Serial.print(F("Humidity: ")); Serial.print(humidity); Serial.println(F(" %"));
  Serial.print(F("PM 2.5: ")); Serial.print(pm.pm2_5); Serial.println(F(" ug/m3"));
  Serial.print(F("PM 10: ")); Serial.print(pm.pm10); Serial.println(F(" ug/m3"));
  Serial.println(F("-----------------------"));

  // 4. ตรวจสอบเงื่อนไขส่งแจ้งเตือนเข้า LINE
  if (pm.pm2_5 >= PM25_THRESHOLD) {
    unsigned long currentMillis = millis();
    // เช็คว่าผ่านเวลาที่กำหนดไปหรือยัง เพื่อไม่ให้ส่งไลน์ถี่เกินไป
    if (currentMillis - lastLineSendTime >= lineInterval || lastLineSendTime == 0) {
      String message = "\n🚨 แจ้งเตือนคุณภาพอากาศ Community-AirWatch!\n";
      message += "⚠️ พบค่า PM2.5 เกินมาตรฐาน: " + String(pm.pm2_5) + " µg/m³\n";
      message += "🌡 อุณหภูมิ: " + String(temperature, 1) + " °C\n";
      message += "💧 ความชื้น: " + String(humidity, 1) + " %\n";
      message += "กรุณาสวมหน้ากากอนามัยก่อนออกจากอาคาร";
      
      sendLineNotify(message);
      lastLineSendTime = currentMillis;
    }
  }

  // 5. แสดงผลบนจอ OLED
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println(F("Community-AirWatch"));
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 15);
  display.print(F("Temp: ")); display.print(temperature, 1); display.print(F(" C"));

  display.setCursor(0, 27);
  display.print(F("Humid: ")); display.print(humidity, 1); display.print(F(" %"));

  display.setCursor(0, 42);
  display.setTextSize(2);
  display.print(F("PM2.5:")); display.print(pm.pm2_5);

  display.setCursor(0, 56);
  display.setTextSize(1);
  display.print(F("ug/m3 | PM10: ")); display.print(pm.pm10);

  display.display();

  delay(3000); // หน่วงเวลา 3 วินาทีก่อนวนลูปใหม่
}

// ฟังก์ชันสำหรับอ่านค่าไบต์จาก PMS5003
PMData readPMS5003() {
  PMData data = {0, 0};
  uint8_t buffer[32];
  
  if (pmsSerial.find(0x42)) { 
    pmsSerial.readBytes(buffer, 31);
    if (buffer[0] == 0x4d) {
      data.pm2_5 = (buffer[10] << 8) | buffer[11];
      data.pm10  = (buffer[12] << 8) | buffer[13];
    }
  }
  return data;
}

// ฟังก์ชันส่งข้อความเข้า LINE Notify ผ่าน HTTPS
void sendLineNotify(String message) {
  WiFiClientSecure client;
  client.setInsecure(); // ข้ามการตรวจสอบใบรับรอง SSL เพื่อความง่ายในการเชื่อมต่อบน ESP32

  Serial.println(F("Connecting to LINE Notify..."));
  if (client.connect("notify-api.line.me", 443)) {
    String query = "message=" + urlEncode(message);
    
    client.println("POST /api/notify HTTP/1.1");
    client.println("Host: notify-api.line.me");
    client.println("Authorization: Bearer " + String(LINE_TOKEN));
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(query.length());
    client.println();
    client.println(query);
    
    Serial.println(F("LINE notification sent successfully!"));
    client.stop();
  } else {
    Serial.println(F("Connection to LINE failed."));
  }
}

// ฟังก์ชันแปลงข้อความรองรับอักขระพิเศษสำหรับ HTTP POST
String urlEncode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
  }
  return encodedString;
}
