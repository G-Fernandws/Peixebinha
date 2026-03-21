#include "WiFi.h"

void setup() {
  Serial.begin(115200);
  delay(2000);

  WiFi.mode(WIFI_STA);
  delay(1000);

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
}

void loop() {
}