#include <esp_now.h>
#include <WiFi.h>

// Broadcast address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Payload to send
typedef struct __attribute__((packed)) {
  uint8_t cmd;  // 1 = Ambulance detected
} payload_t;

payload_t dataToSend;

// Callback when data is sent
void OnDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  if (status == ESP_NOW_SEND_SUCCESS) Serial.println("Success");
  else Serial.println("Fail");

  // NOTE: tx_info->mac no longer exists, cannot print receiver MAC
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // Required for ESP-NOW

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(OnDataSent);

  // Register broadcast peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("ESP-NOW Master Initialized (Broadcast Mode)");
}

void loop() {
  // Set ambulance command
  dataToSend.cmd = 1;  // 1 = Ambulance detected

  // Send broadcast
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&dataToSend, sizeof(dataToSend));
  if (result == ESP_OK) Serial.println("Broadcast sent");
  else Serial.println("Broadcast failed");

  delay(2000); // Send every 2 sec
}
