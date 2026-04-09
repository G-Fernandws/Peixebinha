#define NH3_PIN 33

const int NUM_LEITURAS = 100;
const int TEMPO_ENTRE_LEITURAS = 20;
const int INTERVALO = 2000;

const float VREF = 3.3;
const int ADC_MAX = 4095;

// ===== COEFICIENTES DA SUA CALIBRAÇÃO =====
const float A = 0.26350191;
const float B = 2.48339534;

// --------------------------------------------------
float lerADCMedio() {
  long soma = 0;

  for (int i = 0; i < NUM_LEITURAS; i++) {
    soma += analogRead(NH3_PIN);
    delay(TEMPO_ENTRE_LEITURAS);
  }

  return (float)soma / NUM_LEITURAS;
}

float adcParaTensao(float adc) {
  return (adc / ADC_MAX) * VREF;
}

// --------------------------------------------------
// C = 10^((V - B)/A)
// --------------------------------------------------
float calcularConcentracao(float tensao) {
  float c = pow(10.0, (tensao - B) / A);

  if (isnan(c) || isinf(c) || c < 0) return 0.0;

  return c;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  analogReadResolution(12);

  Serial.println("====================================");
  Serial.println(" LEITURA SENSOR AMONIA ");
  Serial.println("====================================");
}

void loop() {
  float adcMedio = lerADCMedio();
  float tensao = adcParaTensao(adcMedio);
  float concentracao = calcularConcentracao(tensao);

  Serial.println("------------------------------------");

  Serial.print("ADC medio        : ");
  Serial.println(adcMedio, 2);

  Serial.print("Tensao           : ");
  Serial.print(tensao, 4);
  Serial.println(" V");

  Serial.print("Amonia estimada  : ");
  Serial.print(concentracao, 4);
  Serial.println(" mg/L");

  delay(INTERVALO);
}