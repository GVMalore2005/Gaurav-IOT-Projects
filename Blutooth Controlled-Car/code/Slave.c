#include <WiFi.h>
#include <esp_now.h>

// Lane 1
#define RED1    25
#define YELLOW1 26
#define GREEN1  27

// Lane 2
#define RED2    14
#define YELLOW2 12
#define GREEN2  13

// Lane 3
#define RED3    18
#define YELLOW3 19
#define GREEN3  21

// Timings
const unsigned long GREEN_TIME = 5000;
const unsigned long YELLOW_TIME = 3000;
const unsigned long AMBULANCE_TIMEOUT = 3000; // 3 sec

// State variables
bool ambulanceDetected = false;
unsigned long lastSignalTime = 0;

int currentLane = 1;
enum State { GREEN, YELLOW };
State currentState = GREEN;
unsigned long stateStartTime = 0;

// Payload from Master
typedef struct __attribute__((packed)) {
  uint8_t cmd;
} payload_t;

// 🔹 Helper: Show lane color
void showLaneState(int lane, String color) {
  if (lane == 1) {
    digitalWrite(RED1,    color == "RED"    ? HIGH : LOW);
    digitalWrite(GREEN1,  color == "GREEN"  ? HIGH : LOW);
    digitalWrite(YELLOW1, color == "YELLOW" ? HIGH : LOW);
  }
  if (lane == 2) {
    digitalWrite(RED2,    color == "RED"    ? HIGH : LOW);
    digitalWrite(GREEN2,  color == "GREEN"  ? HIGH : LOW);
    digitalWrite(YELLOW2, color == "YELLOW" ? HIGH : LOW);
  }
  if (lane == 3) {
    digitalWrite(RED3,    color == "RED"    ? HIGH : LOW);
    digitalWrite(GREEN3,  color == "GREEN"  ? HIGH : LOW);
    digitalWrite(YELLOW3, color == "YELLOW" ? HIGH : LOW);
  }
}

// 🔹 Set all lanes RED
void setAllRed() {
  showLaneState(1, "RED");
  showLaneState(2, "RED");
  showLaneState(3, "RED");
}

// 🔹 ESP-NOW receive callback
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  if (len < sizeof(payload_t)) return;
  payload_t pkt;
  memcpy(&pkt, incomingData, sizeof(pkt));

  if (pkt.cmd == 1) {
    ambulanceDetected = true;
    lastSignalTime = millis();
    Serial.println("🚑 Ambulance detected → Lane 1 GREEN, others RED");
  }
}

void setup() {
  Serial.begin(115200);

  // Configure pins
  pinMode(RED1, OUTPUT); pinMode(YELLOW1, OUTPUT); pinMode(GREEN1, OUTPUT);
  pinMode(RED2, OUTPUT); pinMode(YELLOW2, OUTPUT); pinMode(GREEN2, OUTPUT);
  pinMode(RED3, OUTPUT); pinMode(YELLOW3, OUTPUT); pinMode(GREEN3, OUTPUT);

  setAllRed();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW Init Failed");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  stateStartTime = millis();
}

void loop() {
  unsigned long now = millis();

  // Timeout ambulance mode
  if (ambulanceDetected && (now - lastSignalTime > AMBULANCE_TIMEOUT)) {
    ambulanceDetected = false;
    currentLane = 1;
    currentState = GREEN;
    stateStartTime = now;
    Serial.println("❌ Ambulance cleared → back to normal cycle");
  }

  // 🚑 Ambulance mode
  if (ambulanceDetected) {
    showLaneState(1, "GREEN");   // Lane 1 GREEN
    showLaneState(2, "RED");     // Lane 2 RED
    showLaneState(3, "RED");     // Lane 3 RED
    return;
  }

  // 🚦 Normal traffic cycle
  switch (currentState) {
    case GREEN:
      for (int i = 1; i <= 3; i++) {
        if (i == currentLane) showLaneState(i, "GREEN");
        else showLaneState(i, "RED");
      }
      if (now - stateStartTime >= GREEN_TIME) {
        currentState = YELLOW;
        stateStartTime = now;
      }
      break;

    case YELLOW:
      for (int i = 1; i <= 3; i++) {
        if (i == currentLane) showLaneState(i, "YELLOW");
        else showLaneState(i, "RED");
      }
      if (now - stateStartTime >= YELLOW_TIME) {
        currentLane++;
        if (currentLane > 3) currentLane = 1;
        currentState = GREEN;
        stateStartTime = now;
      }
      break;
  }
}
