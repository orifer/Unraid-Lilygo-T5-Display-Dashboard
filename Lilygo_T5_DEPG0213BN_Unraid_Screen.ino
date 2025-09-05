#include <WiFi.h>
#include <WebServer.h>
#include <GxEPD2_BW.h>

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

#define EPD_CS   5
#define EPD_DC   17
#define EPD_RST  16
#define EPD_BUSY 4

GxEPD2_213_BN epd(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);
GxEPD2_BW<GxEPD2_213_BN, GxEPD2_213_BN::HEIGHT> display(epd);

// WiFi settings
const char* ssid = "WiFi Name";
const char* password = "passwd";

IPAddress local_IP(192, 168, 1, 5);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

// Full refresh every 48h
unsigned long lastFullUpdate = 0;
const unsigned long fullUpdateInterval = 48 * 60 * 60 * 1000;

void setup() {
  Serial.begin(115200);

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());

  display.init();
  display.setRotation(3);
  fullUpdate("Waiting for data...");

  server.on("/stats", HTTP_POST, handleStats);
  server.begin();
}

void handleStats() {
  String stats = server.arg("plain");
  Serial.println("Received stats: ");
  Serial.println(stats);

  partialUpdate(stats.c_str());
  server.send(200, "text/plain", "Stats received");
  display.hibernate();
}

void fullUpdate(const char* text) {
  Serial.println("Performing full update...");

  display.setFullWindow();

  // White screen wipe to remove ghosting
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
  } while (display.nextPage());

  delay(100);  // Let display settle a bit

  // Draw actual content
  display.firstPage();
  do {
    drawStats(text);
  } while (display.nextPage());

  lastFullUpdate = millis();
}


void partialUpdate(const char* text) {
  if (millis() - lastFullUpdate > fullUpdateInterval) {
    fullUpdate(text);
    return;
  }

  display.setPartialWindow(0, 0, display.width(), display.height());
  display.firstPage();
  do {
    drawStats(text);
  } while (display.nextPage());
}

void drawStats(const char* text) {
  display.setTextColor(GxEPD_BLACK);

  // Parse
  int cpu = parseStat(text, "CPU");
  int ram = parseStat(text, "RAM");
  int temp = parseStat(text, "TEMP");
  int diskCache = parseStat(text, "CACHE");
  int disk01 = parseStat(text, "DISK01");
  int disk02 = parseStat(text, "DISK02");
  String uptime = parseUptime(text);

  int leftX = 8;
  int rightX = display.width() / 2 + 5;
  int barWidth = 110;

  // Left Column (System)
  // Vertical line
  display.drawLine(55, 10, 55, 62, GxEPD_BLACK); // de (50,20) a (50,80)

  // Horitzontal line
  display.drawLine(8, 62, 115, 62, GxEPD_BLACK); // de (50,20) a (50,80)

  // CPU text
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(leftX, 20);
  display.print("CPU");

  // Temp text
  display.setCursor(leftX + 60, 20);
  display.print("TEMP");

  // CPU value
  display.setCursor(leftX + 5, 50);
  display.setFont(&FreeSansBold12pt7b);
  display.print(cpu);
  display.setCursor(leftX + 33, 50);
  display.setFont(NULL);
  display.print("%");

  // Temp value
  display.setCursor(75, 44);
  display.setFont(&FreeSansBold12pt7b);
  display.print(temp);
  display.setCursor(103, 50);
  display.setFont(NULL);
  display.print("C");

  // RAM Text
  display.setCursor(leftX, 75);
  display.setFont(&FreeMonoBold9pt7b);
  display.print("RAM");
  display.setFont(NULL);

  // RAM Bar
  int filled = (ram / 100.0) * barWidth;
  display.drawRect(leftX, 85, barWidth, 16, GxEPD_BLACK);
  display.fillRect(leftX, 85, filled, 16, GxEPD_BLACK);

  // RAM Capacity
  display.setFont(NULL);
  display.setCursor(88, 76);
  display.print("64 GB");

  // Right Column (Disks)
  drawLabelAndBar(rightX, 15, "CACHE", diskCache, barWidth, "1 TB");
  drawLabelAndBar(rightX, 50, "DISK01", disk01, barWidth, "8 TB");
  drawLabelAndBar(rightX, 85, "DISK02", disk02, barWidth, "18 TB");

  // Uptime
  display.setCursor(leftX, display.height() - 10);
  display.print(uptime);
}

void drawLabelAndBar(int x, int y, const char* label, float percent, int barWidth, const char* capacity) {
  // Label text
  display.setCursor(x, y);
  display.setFont(&FreeMono9pt7b);
  display.print(label);
  display.setFont(NULL);

  // Bar
  int filled = (percent / 100.0) * barWidth;
  display.drawRect(x, y + 8, barWidth, 8, GxEPD_BLACK);
  display.fillRect(x, y + 8, filled, 8, GxEPD_BLACK);

  // Capacity
  display.setFont(NULL);
  display.setCursor(x + barWidth - 28, y);
  display.print(capacity);
}

float parseStat(const char* text, const char* key) {
  String input(text);
  int idx = input.indexOf(key);
  if (idx == -1) return 0;

  int start = idx + strlen(key) + 1; // skip over key and '='
  int end = input.indexOf(';', start);
  if (end == -1) end = input.length();

  String value = input.substring(start, end);
  return value.toInt();
}

String parseUptime(const char* text) {
  String input(text);
  int idx = input.indexOf("UPTIME=");
  if (idx == -1) return "";
  int start = idx + 7; // after "UPTIME="
  int end = input.indexOf(';', start);
  if (end == -1) end = input.length();
  return input.substring(start, end);
}


void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected! Attempting reconnect...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Connecting...");
    }
    return;
  }

  server.handleClient();

  if (millis() - lastFullUpdate > fullUpdateInterval) {
    Serial.println("Scheduled full refresh due to timer...");
    fullUpdate("Refreshing...");
    display.hibernate();
  }
}
