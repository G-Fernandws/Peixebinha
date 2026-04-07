#define NITRATO_PIN 33

const int NUM_LEITURAS = 100;
const float VREF = 3.3;
const int ADC_RES = 4095;

// Substituit pelos coeficientes obtidos na calibracao
float a = -0.120835;
float b =  2.046716;  

float lerMediaADC(int pin, int nLeituras) {
  long soma = 0;
  for (int i = 0; i < nLeituras; i++) {
    soma += analogRead(pin);
    delay(20);
  }
  return (float)soma / nLeituras;
}

float adcParaTensao(float adc) {
  return (adc / ADC_RES) * VREF;
}

float calcularNitratoISE(float tensao) {
  return pow(10.0, (tensao - b) / a);
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  analogReadResolution(12);

  Serial.println("========================================");
  Serial.println(" LEITURA DE NITRATO ISE - ESP32 ");
  Serial.println(" Modelo: Tensao = a * log10(C) + b ");
  Serial.println("========================================");
  Serial.print("a = ");
  Serial.println(a, 6);
  Serial.print("b = ");
  Serial.println(b, 6);
  Serial.println();
}

void loop() {
  float adcMedio = lerMediaADC(NITRATO_PIN, NUM_LEITURAS);
  float tensao = adcParaTensao(adcMedio);
  float nitrato = calcularNitratoISE(tensao);

  if (nitrato < 0) nitrato = 0;

  Serial.println("----------------------------------------");
  Serial.print("ADC medio        : ");
  Serial.println(adcMedio, 2);
  Serial.print("Tensao media (V) : ");
  Serial.println(tensao, 6);
  Serial.print("Nitrato (mg/L)   : ");
  Serial.println(nitrato, 2);

  delay(2000);
}