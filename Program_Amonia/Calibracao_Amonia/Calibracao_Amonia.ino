// =====================================================
// LEITURA DO SENSOR DE AMÔNIA PARA CURVA DE CALIBRAÇÃO
// ESP32
// =====================================================

#include <WiFi.h>

const int sensorPin = 34;   // GPIO conectado ao sensor

void setup() {

  Serial.begin(115200);

  // Desliga WiFi e Bluetooth para reduzir ruído
  WiFi.mode(WIFI_OFF);
  btStop();

  // Configuração do ADC
  analogReadResolution(12);
  analogSetPinAttenuation(sensorPin, ADC_11db);

  Serial.println("Curva de calibração - Sensor de amônia");
  Serial.println("Concentracao(mg/L)\tTensao(mV)");
}

void loop() {

  long soma = 0;
  int N = 100;

  // Média de 100 leituras
  for(int i = 0; i < N; i++) {

    soma += analogRead(sensorPin);

    delay(20);
  }

  float leituraMedia = soma/(float)N;

  // Conversão para mV
  float tensao_mV = leituraMedia * 3300.0 / 4095.0;

  Serial.print("Tensao = ");
  Serial.print(tensao_mV,2);
  Serial.println(" mV");

  delay(2000);

}