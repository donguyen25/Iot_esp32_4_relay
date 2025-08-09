#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#define RELAY_1  19
#define RELAY_2  18
#define RELAY_3  5
#define RELAY_4  4

#define BTN_1    13
#define BTN_2    12
#define BTN_3    14
#define BTN_4    27

#define RESET_PIN  0

const int relayPins[4] = { RELAY_1, RELAY_2, RELAY_3, RELAY_4 };
const int buttonPins[4] = { BTN_1, BTN_2, BTN_3, BTN_4 };

AsyncWebServer server(80);
bool relayStates[4] = {false, false, false, false};

unsigned long lastResetPress = 0;
const unsigned long resetDebounce = 1000;

void setupRelays() {
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }
}

void setupButtons() {
  for (int i = 0; i < 4; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  pinMode(RESET_PIN, INPUT_PULLUP);
}

void toggleRelay(int index) {
  if (index < 0 || index >= 4) return;
  relayStates[index] = !relayStates[index];
  digitalWrite(relayPins[index], relayStates[index] ? HIGH : LOW);
  Serial.printf("Relay %d is now %s\n", index + 1, relayStates[index] ? "ON" : "OFF");
}

void checkButtons() {
  for (int i = 0; i < 4; i++) {
    if (!digitalRead(buttonPins[i])) {
      toggleRelay(i);
      delay(300);  // debounce đơn giản
    }
  }

  if (!digitalRead(RESET_PIN)) {
    unsigned long now = millis();
    if (now - lastResetPress > resetDebounce) {
      Serial.println("Reset WiFi...");
      WiFiManager wm;
      wm.resetSettings();
      delay(100);
      ESP.restart();
    }
    lastResetPress = millis();
  }
}

void setupWebServer() {
  // Serve file style.css từ SPIFFS và ép trình duyệt luôn tải lại
  server.serveStatic("/style.css", SPIFFS, "/style.css")
        .setCacheControl("no-cache, no-store, must-revalidate");

  // Trang chính: sinh HTML động thể hiện trạng thái relay
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html lang=\"vi\"><head><meta charset=\"UTF-8\" />";
    html += "<title>ESP32 Iot</title>";
    html += "<link rel=\"stylesheet\" href=\"/style.css\" />";
    html += "</head><body><div class=\"container\">";
    html += "<h1>Điều khiển 4 Relay</h1>";

    for (int i = 0; i < 4; i++) {
      String relayClass = relayStates[i] ? "relay on" : "relay off";
      html += "<div class=\"" + relayClass + "\">";
      html += "<p>Relay " + String(i + 1) + "</p>";
      html += "<a href=\"/relay?ch=" + String(i) + "\" class=\"btn\">Bật \\ Tắt</a>";
      html += "</div>";
    }

    html += "</div></body></html>";
    request->send(200, "text/html", html);
  });

  // Xử lý khi nhấn nút relay
  server.on("/relay", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("ch")) {
      int ch = request->getParam("ch")->value().toInt();
      toggleRelay(ch);
    }
    request->redirect("/");
  });

  // Trang lỗi 404 nếu truy cập sai
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "404: Not found");
  });

  server.begin();
}

void setup() {
  Serial.begin(115200);
  setupRelays();
  setupButtons();

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  if (!wm.autoConnect("ESP32_Config")) {
    Serial.println("Failed to connect WiFi");
    ESP.restart();
  }

  Serial.println("Connected! IP: " + WiFi.localIP().toString());
  setupWebServer();
}

void loop() {
  checkButtons();
}
