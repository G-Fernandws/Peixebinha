#include "WiFi.h"

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("===== DIAGNOSTICO DO ESP32 =====");

  Serial.print("Modelo do chip: ");
  Serial.println(ESP.getChipModel());

  Serial.print("Numero de nucleos: ");
  Serial.println(ESP.getChipCores());

  Serial.print("Frequencia da CPU (MHz): ");
  Serial.println(ESP.getCpuFreqMHz());

  Serial.print("Memoria Flash (MB): ");
  Serial.println(ESP.getFlashChipSize() / (1024 * 1024));

  Serial.print("Memoria RAM livre (bytes): ");
  Serial.println(ESP.getFreeHeap());

  Serial.print("Endereco MAC do WiFi: ");
  Serial.println(WiFi.macAddress());

  Serial.println("\nEscaneando redes WiFi proximas...");
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();

  if (n == 0) {
    Serial.println("Nenhuma rede encontrada");
  } else {
    Serial.println("Redes encontradas:");
    for (int i = 0; i < n; i++) {
      Serial.print(i + 1);
      Serial.print(" - ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" | Sinal: ");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm");
    }
  }

  Serial.println("\nDiagnostico finalizado.");
}

void loop() {
}