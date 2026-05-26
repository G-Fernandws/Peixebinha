#include <WiFi.h>
#include <math.h>

// ===========================
// CONFIGURAÇÕES
// ===========================

// Pino analógico
const int sensorPin = 33;

// Calibração
const float slope = -56.0;

// Ponto conhecido
const float ponto1_ppm = 0.5;
const float ponto1_mV  = 1667.0;

// Referência ADC ESP32
const float Vref = 3300.0; // mV

// ===========================

void setup() {

  Serial.begin(115200);

  // Desliga WiFi/Bluetooth
  WiFi.mode(WIFI_OFF);
  btStop();

  // Resolução ADC
  analogReadResolution(12);

  // Atenuação
  analogSetPinAttenuation(sensorPin, ADC_11db);

  Serial.println("Sensor NH4+");
}

void loop() {

  // ===========================
  // MÉDIA DAS LEITURAS
  // ===========================

  long soma = 0;
  int amostras = 50;

  for(int i=0; i<amostras; i++) {

    soma += analogRead(sensorPin);

    delay(20);
  }

  float adcMedio = soma / (float)amostras;

  // ===========================
  // CONVERSÃO ADC -> mV
  // ===========================

  float tensao_mV =
      (adcMedio / 4095.0) * Vref;

  // ===========================
  // EQUAÇÃO DE NERNST
  // ===========================

  float concentracao =
      ponto1_ppm *
      pow(10, (tensao_mV - ponto1_mV) / slope);

  // ===========================
  // SERIAL
  // ===========================

  Serial.print("ADC: ");
  Serial.print(adcMedio);

  Serial.print(" | mV: ");
  Serial.print(tensao_mV, 2);

  Serial.print(" | NH4: ");
  Serial.print(concentracao, 3);

  Serial.println(" ppm");

  delay(1000);
}