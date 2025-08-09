// Blynk IoT settings
#define BLYNK_TEMPLATE_ID "TMPL6TtEcBRvE"
#define BLYNK_TEMPLATE_NAME "8 RELAY2"
#define BLYNK_AUTH_TOKEN "hu4ZNkatb-mOmF457xoA-e_Z1zjH2YgK"

#include <WiFiManager.h>
#include <BlynkSimpleEsp32.h>
#include <EEPROM.h>

#define RELAY1 19
#define RELAY2 18
#define RELAY3 5
#define RELAY4 4

#define BTN1 13
#define BTN2 12
#define BTN3 14
#define BTN4 27

#define RESET_WIFI_BTN 25  // 👉 Nút riêng dùng để reset WiFi (kết nối GPIO25 với GND)

#define EEPROM_SIZE 10

bool relayState[4];
BlynkTimer timer;

int getRelayPin(int index) {
  switch (index) {
    case 0: return RELAY1;
    case 1: return RELAY2;
    case 2: return RELAY3;
    case 3: return RELAY4;
    default: return -1;
  }
}

int getButtonPin(int index) {
  switch (index) {
    case 0: return BTN1;
    case 1: return BTN2;
    case 2: return BTN3;
    case 3: return BTN4;
    default: return -1;
  }
}

void writeRelayToEEPROM() {
  for (int i = 0; i < 4; i++) {
    EEPROM.write(i, relayState[i]);
  }
  EEPROM.commit();
}

void readRelayFromEEPROM() {
  for (int i = 0; i < 4; i++) {
    relayState[i] = EEPROM.read(i) == 1;
    digitalWrite(getRelayPin(i), relayState[i]);
    Blynk.virtualWrite(V1 + i, relayState[i]);
  }
}

void toggleRelay(int index) {
  relayState[index] = !relayState[index];
  digitalWrite(getRelayPin(index), relayState[index]);
  writeRelayToEEPROM();
  Blynk.virtualWrite(V1 + index, relayState[index]);
}

// Xử lý điều khiển từ Blynk App/Web
BLYNK_WRITE(V1) {
  relayState[0] = param.asInt();
  digitalWrite(RELAY1, relayState[0]);
  writeRelayToEEPROM();
}

BLYNK_WRITE(V2) {
  relayState[1] = param.asInt();
  digitalWrite(RELAY2, relayState[1]);
  writeRelayToEEPROM();
}

BLYNK_WRITE(V3) {
  relayState[2] = param.asInt();
  digitalWrite(RELAY3, relayState[2]);
  writeRelayToEEPROM();
}

BLYNK_WRITE(V4) {
  relayState[3] = param.asInt();
  digitalWrite(RELAY4, relayState[3]);
  writeRelayToEEPROM();
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(getRelayPin(i), OUTPUT);
    pinMode(getButtonPin(i), INPUT_PULLUP);
  }
  pinMode(RESET_WIFI_BTN, INPUT_PULLUP);

  EEPROM.begin(EEPROM_SIZE);

  // Kiểm tra nút reset WiFi có được nhấn không
  if (digitalRead(RESET_WIFI_BTN) == LOW) {
    WiFiManager wm;
    wm.resetSettings();
    Serial.println("WiFi settings reset. Please restart and reconnect.");
    delay(2000);
  }

  WiFiManager wifiManager;
  if (!wifiManager.autoConnect("ESP32_Config")) {
    Serial.println("Failed to connect. Restarting...");
    delay(3000);
    ESP.restart();
  }

  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());

  // Cập nhật trạng thái relay từ EEPROM và gửi về Blynk
  readRelayFromEEPROM();

  // Kiểm tra nút vật lý định kỳ
  timer.setInterval(100L, []() {
    for (int i = 0; i < 4; i++) {
      static bool lastState[4] = {HIGH, HIGH, HIGH, HIGH};
      bool current = digitalRead(getButtonPin(i));
      if (current == LOW && lastState[i] == HIGH) {
        toggleRelay(i);
        delay(300); // đơn giản debounce
      }
      lastState[i] = current;
    }
  });
}

void loop() {
  Blynk.run();
  timer.run();
}
