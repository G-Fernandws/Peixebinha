/*
  ============================================================
  CALIBRACAO DO SENSOR ION-SELETIVO DE AMONIA - ESP32
  ============================================================

  Sensor analogico: GPIO 34
  Pontos de calibracao: 10, 25, 50, 75 e 100 mg/L

  Modelo de ajuste:
  V = a * log10(C) + b

  Equacao inversa:
  C = 10^((V - b) / a)

  ------------------------------------------------------------
  OBSERVACAO
  Esta rotina considera apenas a resposta empirica do sensor
  frente aos padroes preparados, sem compensacao de pH ou
  temperatura.
*/

#define NH3_PIN 33

// ---------------- CONFIGURACOES ----------------
const int NUM_PONTOS = 5;
const int NUM_LEITURAS = 100;
const int TEMPO_ENTRE_LEITURAS = 20;   // ms

const float VREF = 3.3;
const int ADC_MAX = 4095;

// Pontos de calibracao em mg/L
float concentracoes[NUM_PONTOS] = {10.0, 25.0, 50.0, 75.0, 100.0};

// Vetores para armazenar resultados
float adcMedio[NUM_PONTOS];
float tensaoMedia[NUM_PONTOS];
float logConcentracao[NUM_PONTOS];

// Coeficientes da reta
float a = 0.0;
float b = 0.0;

// ----------------------------------------------------------
// Aguarda ENTER no Monitor Serial
// ----------------------------------------------------------
void aguardarEnter() {
  while (Serial.available()) Serial.read();

  Serial.println("Pressione ENTER para iniciar a leitura deste ponto...");
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        break;
      }
    }
  }
}

// ----------------------------------------------------------
// Le a media de varias leituras ADC
// ----------------------------------------------------------
float lerADCMedio() {
  long soma = 0;

  for (int i = 0; i < NUM_LEITURAS; i++) {
    soma += analogRead(NH3_PIN);
    delay(TEMPO_ENTRE_LEITURAS);
  }

  return (float)soma / NUM_LEITURAS;
}

// ----------------------------------------------------------
// Converte ADC para tensao
// ----------------------------------------------------------
float adcParaTensao(float adc) {
  return (adc / ADC_MAX) * VREF;
}

// ----------------------------------------------------------
// Regressao linear: y = a*x + b
// y = tensao
// x = log10(concentracao)
// ----------------------------------------------------------
void calcularRegressaoLinear(float x[], float y[], int n, float &coefA, float &coefB) {
  float somaX = 0.0;
  float somaY = 0.0;
  float somaXY = 0.0;
  float somaX2 = 0.0;

  for (int i = 0; i < n; i++) {
    somaX += x[i];
    somaY += y[i];
    somaXY += x[i] * y[i];
    somaX2 += x[i] * x[i];
  }

  float denominador = (n * somaX2) - (somaX * somaX);

  if (denominador == 0.0) {
    coefA = 0.0;
    coefB = 0.0;
    return;
  }

  coefA = ((n * somaXY) - (somaX * somaY)) / denominador;
  coefB = (somaY - coefA * somaX) / n;
}

// ----------------------------------------------------------
// Calcula R²
// ----------------------------------------------------------
float calcularR2(float x[], float y[], int n, float coefA, float coefB) {
  float mediaY = 0.0;
  for (int i = 0; i < n; i++) {
    mediaY += y[i];
  }
  mediaY /= n;

  float ssRes = 0.0;
  float ssTot = 0.0;

  for (int i = 0; i < n; i++) {
    float yEst = coefA * x[i] + coefB;
    ssRes += pow(y[i] - yEst, 2);
    ssTot += pow(y[i] - mediaY, 2);
  }

  if (ssTot == 0.0) return 0.0;
  return 1.0 - (ssRes / ssTot);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  analogReadResolution(12);

  Serial.println("====================================================");
  Serial.println(" CALIBRACAO DO SENSOR ION-SELETIVO DE AMONIA ");
  Serial.println("====================================================");
  Serial.println("Pino do sensor: GPIO 35");
  Serial.println("Pontos: 10, 25, 50, 75 e 100 mg/L");
  Serial.println("Modelo: V = a * log10(C) + b");
  Serial.println("====================================================");
  Serial.println();

  for (int i = 0; i < NUM_PONTOS; i++) {
    Serial.println("----------------------------------------------------");
    Serial.print("Coloque o sensor na solucao padrao de ");
    Serial.print(concentracoes[i], 2);
    Serial.println(" mg/L.");
    Serial.println("Aguarde a estabilizacao da leitura e pressione ENTER.");
    aguardarEnter();

    adcMedio[i] = lerADCMedio();
    tensaoMedia[i] = adcParaTensao(adcMedio[i]);
    logConcentracao[i] = log10(concentracoes[i]);

    Serial.println("Leitura concluida.");
    Serial.print("Concentracao padrao         : ");
    Serial.print(concentracoes[i], 2);
    Serial.println(" mg/L");

    Serial.print("log10(concentracao)         : ");
    Serial.println(logConcentracao[i], 6);

    Serial.print("ADC medio                   : ");
    Serial.println(adcMedio[i], 2);

    Serial.print("Tensao media                : ");
    Serial.print(tensaoMedia[i], 4);
    Serial.println(" V");
    Serial.println();

    delay(1000);
  }

  calcularRegressaoLinear(logConcentracao, tensaoMedia, NUM_PONTOS, a, b);
  float r2 = calcularR2(logConcentracao, tensaoMedia, NUM_PONTOS, a, b);

  Serial.println("====================================================");
  Serial.println(" RESULTADO DA CALIBRACAO ");
  Serial.println("====================================================");

  for (int i = 0; i < NUM_PONTOS; i++) {
    Serial.print("Ponto ");
    Serial.print(i + 1);
    Serial.print(" -> ");
    Serial.print(concentracoes[i], 2);
    Serial.print(" mg/L | ADC medio = ");
    Serial.print(adcMedio[i], 2);
    Serial.print(" | Tensao = ");
    Serial.print(tensaoMedia[i], 4);
    Serial.println(" V");
  }

  Serial.println();
  Serial.println("Equacao de calibracao ajustada:");
  Serial.print("V = ");
  Serial.print(a, 8);
  Serial.print(" * log10(C) + ");
  Serial.println(b, 8);

  Serial.println();
  Serial.println("Equacao inversa para estimar concentracao:");
  Serial.println("C = 10^((V - b) / a)");

  Serial.print("Coeficiente angular (a)     : ");
  Serial.println(a, 8);

  Serial.print("Intercepto (b)              : ");
  Serial.println(b, 8);

  Serial.print("R²                          : ");
  Serial.println(r2, 6);

  Serial.println();
  Serial.println("Calibracao finalizada.");
  Serial.println("Copie os coeficientes para a programacao de leitura.");
}

void loop() {
  // Nada a executar continuamente
}