#define NITRATO_PIN 33

const int NUM_LEITURAS = 100;
const float VREF = 3.3;
const int ADC_RES = 4095;

// 5 pontos medidos
const int NUM_PONTOS_TOTAL = 5;
float concentracoes[NUM_PONTOS_TOTAL] = {0, 25, 50, 75, 100};

// Apenas 4 pontos entram na regressão logarítmica
const int NUM_PONTOS_REG = 4;
float concReg[NUM_PONTOS_REG];
float logConcReg[NUM_PONTOS_REG];
float tensaoReg[NUM_PONTOS_REG];

// Armazenamento geral
float adcMedio[NUM_PONTOS_TOTAL];
float tensaoMedia[NUM_PONTOS_TOTAL];

bool calibracaoConcluida = false;
float coefA = 0.0;
float coefB = 0.0;
float coefR2 = 0.0;

float lerMediaADCInterativa(int pin, int nLeituras) {
  long soma = 0;

  Serial.println("Iniciando aquisicao...");
  for (int i = 0; i < nLeituras; i++) {
    soma += analogRead(pin);

    // Feedback a cada 10 leituras
    if ((i + 1) % 10 == 0 || i == nLeituras - 1) {
      Serial.print("Leituras coletadas: ");
      Serial.print(i + 1);
      Serial.print("/");
      Serial.println(nLeituras);
    }

    delay(20);
  }

  return (float)soma / nLeituras;
}

float adcParaTensao(float adc) {
  return (adc / ADC_RES) * VREF;
}

void limparBufferSerial() {
  while (Serial.available()) {
    Serial.read();
  }
}

void esperarEnter(const char* mensagem) {
  limparBufferSerial();
  Serial.println(mensagem);
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        break;
      }
    }
  }
}

char esperarOpcaoValida() {
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();

      if (c >= 'a' && c <= 'z') c = c - 32; // converte para maiúscula

      if (c == 'C' || c == 'R' || c == 'M') {
        return c;
      }
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
    a = 0;
    b = 0;
    r2 = 0;
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

void mostrarCabecalho() {
  Serial.println("==================================================");
  Serial.println("      CALIBRACAO INTERATIVA - ISE DE NITRATO      ");
  Serial.println("==================================================");
  Serial.println("Modelo de ajuste: Tensao = a * log10(C) + b");
  Serial.println("Pontos medidos: 0, 25, 50, 75 e 100 mg/L");
  Serial.println("Obs.: o ponto 0 mg/L sera lido, mas NAO entra");
  Serial.println("na regressao logaritmica porque log10(0) nao existe.");
  Serial.println("==================================================");
  Serial.println();
}

void mostrarResumoPonto(int i) {
  Serial.println("-------------------------------");
  Serial.print("Ponto ");
  Serial.print(i + 1);
  Serial.print(" de ");
  Serial.println(NUM_PONTOS_TOTAL);

  Serial.print("Concentracao: ");
  Serial.print(concentracoes[i], 1);
  Serial.println(" mg/L");

  Serial.print("ADC medio: ");
  Serial.println(adcMedio[i], 2);

  Serial.print("Tensao media: ");
  Serial.print(tensaoMedia[i], 6);
  Serial.println(" V");

  if (concentracoes[i] > 0) {
    Serial.print("log10(C): ");
    Serial.println(log10(concentracoes[i]), 6);
    Serial.println("Status: ponto incluido na regressao.");
  } else {
    Serial.println("Status: ponto 0 mg/L usado apenas como branco/controle.");
  }

  Serial.println("-------------------------------");
  Serial.println();
}

void mostrarTabelaFinal() {
  Serial.println();
  Serial.println("==================================================");
  Serial.println("             RESUMO DOS PONTOS MEDIDOS            ");
  Serial.println("==================================================");

  for (int i = 0; i < NUM_PONTOS_TOTAL; i++) {
    Serial.print("C = ");
    Serial.print(concentracoes[i], 1);
    Serial.print(" mg/L | ADC = ");
    Serial.print(adcMedio[i], 2);
    Serial.print(" | V = ");
    Serial.print(tensaoMedia[i], 6);
    Serial.println(" V");
  }

  Serial.println("==================================================");
  Serial.println();
}

void mostrarResultadoCalibracao() {
  Serial.println("==================================================");
  Serial.println("           RESULTADO DA CALIBRACAO FINAL          ");
  Serial.println("==================================================");

  Serial.print("Equacao ajustada: Tensao (V) = ");
  Serial.print(coefA, 6);
  Serial.print(" * log10(C mg/L) + ");
  Serial.println(coefB, 6);

  Serial.print("R^2 = ");
  Serial.println(coefR2, 6);

  Serial.println();
  Serial.println("Equacao inversa para estimar concentracao:");
  Serial.println("C = 10^((Tensao - b) / a)");

  Serial.println();
  Serial.println("Coeficientes para uso posterior:");
  Serial.print("a = ");
  Serial.println(coefA, 6);
  Serial.print("b = ");
  Serial.println(coefB, 6);

  Serial.println("==================================================");
  Serial.println();
}

void executarCalibracao() {
  int idxReg = 0;

  Serial.println();
  Serial.println("Preparando nova calibracao...");
  Serial.println();

  for (int i = 0; i < NUM_PONTOS_TOTAL; i++) {
    Serial.println("==================================================");
    Serial.print("Prepare a solucao de ");
    Serial.print(concentracoes[i], 1);
    Serial.println(" mg/L");
    Serial.println("Lave o sensor, aguarde estabilizacao e pressione ENTER.");
    Serial.println("==================================================");

    esperarEnter("Pressione ENTER para iniciar a leitura deste ponto.");

    float adc = lerMediaADCInterativa(NITRATO_PIN, NUM_LEITURAS);
    float tensao = adcParaTensao(adc);

    adcMedio[i] = adc;
    tensaoMedia[i] = tensao;

    if (concentracoes[i] > 0 && idxReg < NUM_PONTOS_REG) {
      concReg[idxReg] = concentracoes[i];
      logConcReg[idxReg] = log10(concentracoes[i]);
      tensaoReg[idxReg] = tensao;
      idxReg++;
    }

    mostrarResumoPonto(i);
  }

  calcularRegressaoLinear(logConcReg, tensaoReg, NUM_PONTOS_REG, coefA, coefB, coefR2);
  calibracaoConcluida = true;

  mostrarTabelaFinal();
  mostrarResultadoCalibracao();

  Serial.println("Digite:");
  Serial.println("[R] para recalibrar");
  Serial.println("[M] para mostrar novamente os resultados");
  Serial.println();
}

void mostrarMenu() {
  Serial.println("Escolha uma opcao:");
  Serial.println("[C] iniciar calibracao");
  if (calibracaoConcluida) {
    Serial.println("[M] mostrar ultimo resultado");
    Serial.println("[R] recalibrar");
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  analogReadResolution(12);

  mostrarCabecalho();
  mostrarMenu();
}

void loop() {
  if (Serial.available()) {
    char opcao = esperarOpcaoValida();

    if (opcao == 'C') {
      executarCalibracao();
    }
    else if (opcao == 'R') {
      executarCalibracao();
    }
    else if (opcao == 'M') {
      if (calibracaoConcluida) {
        mostrarTabelaFinal();
        mostrarResultadoCalibracao();
      } else {
        Serial.println("Nenhuma calibracao foi realizada ainda.");
        Serial.println();
      }
    }

    mostrarMenu();
  }
}