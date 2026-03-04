// ===== Sensor de Condutividade - ESP32 DOIT DEVKIT V1 =====

const int sensorPin = 32;     // GPIO 32
const float Vref = 3.3;       // Tensão de referência do ESP32
const int ADC_resolution = 4095;  // 12 bits

// Coeficientes obtidos pela regressão linear
const float a = 802.23;   // inclinação
const float b = 52.58;    // intercepto

void setup() {
  Serial.begin(115200);
}

void loop() {

  const int numSamples = 100;
  float soma = 0;

  for (int i = 0; i < numSamples; i++) {
    soma += analogRead(sensorPin);
    delay(5);
  }

  float adcMedio = soma / numSamples;

  // Converte ADC para tensão
  float tensao = (adcMedio / ADC_resolution) * Vref;

  // Calcula condutividade usando apenas tensão
  float condutividade = a * tensao + b;

  // TDS aproximado (fator 0.5)
  float TDS = condutividade * 0.5;

  Serial.print("Tensão média: ");
  Serial.print(tensao, 4);
  Serial.print(" V  |  Condutividade: ");
  Serial.print(condutividade, 1);
  Serial.print(" uS/cm  |  TDS: ");
  Serial.print(TDS, 1);
  Serial.println(" ppm");

  delay(2000);
}