#include <WiFi.h>
#include <Wire.h>

void testeGPIO() {
  Serial.println("\n--- Teste de GPIO ---");

  int pinos[] = {2, 4, 5, 18, 19, 21, 22, 23};
  int n = sizeof(pinos) / sizeof(pinos[0]);

  for (int i = 0; i < n; i++) {
    pinMode(pinos[i], INPUT);
    int estado = digitalRead(pinos[i]);

    Serial.print("GPIO ");
    Serial.print(pinos[i]);
    Serial.print(" leitura: ");
    Serial.println(estado);
  }
}

void testeADC() {
  Serial.println("\n--- Teste ADC ---");

  int valor = analogRead(34);

  Serial.print("Leitura ADC GPIO34: ");
  Serial.println(valor);
}

void testeWiFi() {
  Serial.println("\n--- Teste WiFi ---");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(1000);

  int n = WiFi.scanNetworks();

  Serial.print("Redes encontradas: ");
  Serial.println(n);

  for (int i = 0; i < n; i++) {
    Serial.print(WiFi.SSID(i));
    Serial.print(" | RSSI: ");
    Serial.println(WiFi.RSSI(i));
  }
}

void testeI2C() {
  Serial.println("\n--- Teste I2C ---");

  Wire.begin(21, 22);

  byte count = 0;

  for (byte i = 1; i < 127; i++) {

    Wire.beginTransmission(i);

    if (Wire.endTransmission() == 0) {
      Serial.print("Dispositivo I2C encontrado no endereco 0x");
      Serial.println(i, HEX);
      count++;
    }
  }

  if (count == 0) {
    Serial.println("Nenhum dispositivo I2C conectado (normal se nada estiver ligado).");
  }
}

void setup() {

  Serial.begin(115200);
  delay(2000);

  Serial.println("===== DIAGNOSTICO COMPLETO ESP32 =====");

  Serial.print("Chip: ");
  Serial.println(ESP.getChipModel());

  Serial.print("Nucleos: ");
  Serial.println(ESP.getChipCores());

  Serial.print("CPU MHz: ");
  Serial.println(ESP.getCpuFreqMHz());

  Serial.print("Flash (MB): ");
  Serial.println(ESP.getFlashChipSize() / (1024 * 1024));

  Serial.print("RAM livre: ");
  Serial.println(ESP.getFreeHeap());

  Serial.print("MAC WiFi: ");
  Serial.println(WiFi.macAddress());

  testeGPIO();
  testeADC();
  testeI2C();
  testeWiFi();

  Serial.println("\n===== TESTE FINALIZADO =====");
}

void loop() {
}
