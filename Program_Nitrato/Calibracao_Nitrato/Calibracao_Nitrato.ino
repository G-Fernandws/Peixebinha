#define NITRATO_PIN 34

const int NUM_LEITURAS = 100;
const float VREF = 3.3;
const int ADC_RES = 4095;

// Para ISE, evite zero na regressão
const int NUM_PONTOS = 4;
float concentracoes[NUM_PONTOS] = {25, 50, 75, 100};

float adcMedio[NUM_PONTOS];
float tensaoMedia[NUM_PONTOS];
float logConc[NUM_PONTOS];

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

void esperarEnter() {
  while (Serial.available()) Serial.read();
  Serial.println("Pressione ENTER para iniciar a leitura...");
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') break;
    }
  }
}

void calcularRegressaoLinear(float x[], float y[], int n, float &a, float &b, float &r2) {
  float somaX = 0, somaY = 0, somaXY = 0, somaX2 = 0;

  for (int i = 0; i < n; i++) {
    somaX += x[i];
    somaY += y[i];
    somaXY += x[i] * y[i];
    somaX2 += x[i] * x[i];
  }

  float denom = (n * somaX2 - somaX * somaX);
  if (denom == 0) {
    a = 0; b = 0; r2 = 0;
    return;
  }

  a = (n * somaXY - somaX * somaY) / denom;
  b = (somaY - a * somaX) / n;

  float mediaY = somaY / n;
  float ssTot = 0, ssRes = 0;

  for (int i = 0; i < n; i++) {
    float yPrev = a * x[i] + b;
    ssTot += pow(y[i] - mediaY, 2);
    ssRes += pow(y[i] - yPrev, 2);
  }

  r2 = (ssTot == 0) ? 0 : 1.0 - (ssRes / ssTot);
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  analogReadResolution(12);

  Serial.println("========================================");
  Serial.println(" CALIBRACAO ISE DE NITRATO - ESP32 ");
  Serial.println(" Modelo: Tensao = a * log10(C) + b ");
  Serial.println("========================================");
  Serial.println();

  for (int i = 0; i < NUM_PONTOS; i++) {
    Serial.print("Coloque o sensor na solucao de ");
    Serial.print(concentracoes[i], 1);
    Serial.println(" mg/L");
    esperarEnter();

    float adc = lerMediaADC(NITRATO_PIN, NUM_LEITURAS);
    float tensao = adcParaTensao(adc);

    adcMedio[i] = adc;
    tensaoMedia[i] = tensao;
    logConc[i] = log10(concentracoes[i]);

    Serial.print("ADC medio: ");
    Serial.println(adcMedio[i], 2);
    Serial.print("Tensao media: ");
    Serial.print(tensaoMedia[i], 6);
    Serial.println(" V");
    Serial.print("log10(C): ");
    Serial.println(logConc[i], 6);
    Serial.println();
  }

  float a, b, r2;
  calcularRegressaoLinear(logConc, tensaoMedia, NUM_PONTOS, a, b, r2);

  Serial.println("========================================");
  Serial.println(" RESULTADO DA CALIBRACAO ");
  Serial.println("========================================");
  Serial.print("Tensao (V) = ");
  Serial.print(a, 6);
  Serial.print(" * log10(C mg/L) + ");
  Serial.println(b, 6);

  Serial.print("R² = ");
  Serial.println(r2, 6);

  Serial.println();
  Serial.println("Equacao inversa para uso posterior:");
  Serial.println("C = 10^((Tensao - b) / a)");
}

void loop() {
}