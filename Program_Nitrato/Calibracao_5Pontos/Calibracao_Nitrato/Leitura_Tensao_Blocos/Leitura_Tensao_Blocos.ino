#define NITRATO_PIN 33

const int NUM_LEITURAS_BLOCO = 20;
const int NUM_BLOCOS = 10;
const float VREF = 3.3;
const int ADC_RES = 4095;

float a = -0.277675;
float b =  2.323254;

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
  Serial.println(" DIAGNOSTICO DE LEITURA - ISE NITRATO ");
  Serial.println("========================================");
  Serial.print("a = ");
  Serial.println(a, 6);
  Serial.print("b = ");
  Serial.println(b, 6);
  Serial.println();
  Serial.println("Acompanhe a estabilizacao da tensao em blocos.");
  Serial.println();
}

void loop() {
  Serial.println("----------------------------------------");

  float tensaoAnterior = -999.0;

  for (int bloco = 1; bloco <= NUM_BLOCOS; bloco++) {
    float adcMedio = lerMediaADC(NITRATO_PIN, NUM_LEITURAS_BLOCO);
    float tensao = adcParaTensao(adcMedio);
    float nitrato = calcularNitratoISE(tensao);

    Serial.print("Bloco ");
    Serial.print(bloco);
    Serial.print(" | ADC: ");
    Serial.print(adcMedio, 2);
    Serial.print(" | V: ");
    Serial.print(tensao, 6);
    Serial.print(" | NO3-: ");
    Serial.print(nitrato, 2);
    Serial.print(" mg/L");

    if (tensaoAnterior > 0) {
      Serial.print(" | dV: ");
      Serial.print(tensao - tensaoAnterior, 6);
      Serial.print(" V");
    }

    Serial.println();
    tensaoAnterior = tensao;
  }

  Serial.println();
  Serial.println("Se os blocos ainda estiverem variando, a leitura nao estabilizou.");
  Serial.println("Somente use a concentracao apos estabilizacao da tensao.");
  Serial.println();

  delay(4000);
}