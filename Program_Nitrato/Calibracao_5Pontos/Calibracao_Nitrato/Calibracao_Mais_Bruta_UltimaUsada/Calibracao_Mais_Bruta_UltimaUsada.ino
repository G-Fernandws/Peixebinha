#define NITRATO_PIN 33

// ======================================================
// CONFIGURACOES GERAIS
// ======================================================
const float VREF = 3.3;
const int ADC_RES = 4095;

// Pontos da calibracao
const int NUM_PONTOS = 4;
float concentracoes[NUM_PONTOS] = {25.0, 50.0, 75.0, 100.0};

// Leitura por blocos
const int LEITURAS_POR_BLOCO = 20;
const int MAX_BLOCOS = 20;

// Critério de estabilizacao
const float LIMIAR_ESTAB_V = 0.0015;   // V
const int BLOCOS_ESTAVEIS_NECESSARIOS = 3;

// Intervalos
const int DELAY_ENTRE_LEITURAS_MS = 20;
const int DELAY_ENTRE_BLOCOS_MS = 300;

// Armazenamento dos resultados
float adcFinal[NUM_PONTOS];
float tensaoFinal[NUM_PONTOS];
float logConc[NUM_PONTOS];

// ======================================================
// FUNCOES AUXILIARES
// ======================================================
float lerMediaADC(int pin, int nLeituras) {
  long soma = 0;

  for (int i = 0; i < nLeituras; i++) {
    soma += analogRead(pin);
    delay(DELAY_ENTRE_LEITURAS_MS);
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
  float ssTot = 0;
  float ssRes = 0;

  for (int i = 0; i < n; i++) {
    float yPrev = a * x[i] + b;
    ssTot += pow(y[i] - mediaY, 2);
    ssRes += pow(y[i] - yPrev, 2);
  }

  r2 = (ssTot == 0) ? 0 : 1.0 - (ssRes / ssTot);
}

char lerOpcaoMenu() {
  while (true) {
    if (Serial.available()) {
      char c = Serial.read();

      if (c >= 'a' && c <= 'z') c = c - 32;

      if (c == 'C' || c == 'R' || c == 'M') {
        return c;
      }
    }
  }
}

// ======================================================
// ESTABILIZACAO
// ======================================================
bool capturarPontoEstabilizado(float &adcEstavel, float &tensaoEstavel) {
  float tensaoAnterior = -999.0;
  int blocosEstaveis = 0;

  float somaADCValidos = 0.0;
  float somaTensaoValidas = 0.0;
  int numBlocosValidos = 0;

  Serial.println("Iniciando acompanhamento da estabilizacao...");
  Serial.println("A leitura sera aceita automaticamente quando estabilizar.");
  Serial.println();

  for (int bloco = 1; bloco <= MAX_BLOCOS; bloco++) {
    float adcBloco = lerMediaADC(NITRATO_PIN, LEITURAS_POR_BLOCO);
    float tensaoBloco = adcParaTensao(adcBloco);

    Serial.print("Bloco ");
    Serial.print(bloco);
    Serial.print(" | ADC: ");
    Serial.print(adcBloco, 2);
    Serial.print(" | V: ");
    Serial.print(tensaoBloco, 6);

    if (tensaoAnterior > 0) {
      float dV = tensaoBloco - tensaoAnterior;

      Serial.print(" | dV: ");
      Serial.print(dV, 6);
      Serial.print(" V");

      if (fabs(dV) <= LIMIAR_ESTAB_V) {
        blocosEstaveis++;
        Serial.print(" | estavel ");
        Serial.print("(");
        Serial.print(blocosEstaveis);
        Serial.print("/");
        Serial.print(BLOCOS_ESTAVEIS_NECESSARIOS);
        Serial.print(")");

        somaADCValidos += adcBloco;
        somaTensaoValidas += tensaoBloco;
        numBlocosValidos++;
      } else {
        blocosEstaveis = 0;
        somaADCValidos = 0.0;
        somaTensaoValidas = 0.0;
        numBlocosValidos = 0;
        Serial.print(" | instavel");
      }
    }

    Serial.println();

    tensaoAnterior = tensaoBloco;
    delay(DELAY_ENTRE_BLOCOS_MS);

    if (blocosEstaveis >= BLOCOS_ESTAVEIS_NECESSARIOS && numBlocosValidos > 0) {
      adcEstavel = somaADCValidos / numBlocosValidos;
      tensaoEstavel = somaTensaoValidas / numBlocosValidos;

      Serial.println();
      Serial.println(">>> Ponto considerado estabilizado.");
      Serial.print("ADC medio estabilizado   : ");
      Serial.println(adcEstavel, 2);
      Serial.print("Tensao estabilizada (V)  : ");
      Serial.println(tensaoEstavel, 6);
      Serial.println();

      return true;
    }
  }

  Serial.println();
  Serial.println(">>> Nao foi possivel confirmar estabilizacao no limite definido.");
  Serial.println(">>> Sera usado o ultimo bloco medido.");
  Serial.println();

  adcEstavel = lerMediaADC(NITRATO_PIN, LEITURAS_POR_BLOCO);
  tensaoEstavel = adcParaTensao(adcEstavel);

  return false;
}

// ======================================================
// IMPRESSAO
// ======================================================
void mostrarCabecalho() {
  Serial.println("==================================================");
  Serial.println("     CALIBRACAO AUTOMATICA - ISE DE NITRATO       ");
  Serial.println("==================================================");
  Serial.println("Modelo: Tensao = a * log10(C) + b");
  Serial.println("Pontos de calibracao: 25, 50, 75 e 100 mg/L");
  Serial.println("A leitura de cada ponto sera aceita apos");
  Serial.println("estabilizacao automatica da tensao.");
  Serial.println("==================================================");
  Serial.println();
}

void mostrarMenu(bool calibFeita) {
  Serial.println("Escolha uma opcao:");
  Serial.println("[C] iniciar calibracao");

  if (calibFeita) {
    Serial.println("[M] mostrar ultimo resultado");
    Serial.println("[R] recalibrar");
  }

  Serial.println();
}

void mostrarResumoPonto(int i, bool estabilizou) {
  Serial.println("----------------------------------------");
  Serial.print("Ponto ");
  Serial.print(i + 1);
  Serial.print(" de ");
  Serial.println(NUM_PONTOS);

  Serial.print("Concentracao          : ");
  Serial.print(concentracoes[i], 1);
  Serial.println(" mg/L");

  Serial.print("ADC final             : ");
  Serial.println(adcFinal[i], 2);

  Serial.print("Tensao final          : ");
  Serial.print(tensaoFinal[i], 6);
  Serial.println(" V");

  Serial.print("log10(C)              : ");
  Serial.println(logConc[i], 6);

  Serial.print("Status estabilizacao  : ");
  if (estabilizou) Serial.println("CONFIRMADA");
  else Serial.println("NAO CONFIRMADA (ultimo bloco usado)");

  Serial.println("----------------------------------------");
  Serial.println();
}

void mostrarTabelaFinal() {
  Serial.println("==================================================");
  Serial.println("           RESUMO DOS PONTOS MEDIDOS              ");
  Serial.println("==================================================");

  for (int i = 0; i < NUM_PONTOS; i++) {
    Serial.print("C = ");
    Serial.print(concentracoes[i], 1);
    Serial.print(" mg/L | ADC = ");
    Serial.print(adcFinal[i], 2);
    Serial.print(" | V = ");
    Serial.print(tensaoFinal[i], 6);
    Serial.print(" | log10(C) = ");
    Serial.println(logConc[i], 6);
  }

  Serial.println("==================================================");
  Serial.println();
}

void mostrarResultadoFinal(float a, float b, float r2) {
  Serial.println("==================================================");
  Serial.println("           RESULTADO DA CALIBRACAO FINAL          ");
  Serial.println("==================================================");

  Serial.print("Equacao ajustada: Tensao (V) = ");
  Serial.print(a, 6);
  Serial.print(" * log10(C mg/L) + ");
  Serial.println(b, 6);

  Serial.print("R^2 = ");
  Serial.println(r2, 6);

  Serial.println();
  Serial.println("Equacao inversa para uso posterior:");
  Serial.println("C = 10^((Tensao - b) / a)");

  Serial.println();
  Serial.println("Coeficientes para a programacao de leitura:");
  Serial.print("a = ");
  Serial.println(a, 6);
  Serial.print("b = ");
  Serial.println(b, 6);

  Serial.println();

  if (r2 >= 0.995) {
    Serial.println("Qualidade da curva: MUITO BOA");
  } else if (r2 >= 0.990) {
    Serial.println("Qualidade da curva: BOA");
  } else if (r2 >= 0.980) {
    Serial.println("Qualidade da curva: ACEITAVEL");
  } else {
    Serial.println("Qualidade da curva: RUIM - recomendavel recalibrar");
  }

  Serial.println("==================================================");
  Serial.println();
}

// ======================================================
// ROTINA DE CALIBRACAO
// ======================================================
bool calibracaoRealizada = false;
float ultimoA = 0.0;
float ultimoB = 0.0;
float ultimoR2 = 0.0;

void executarCalibracao() {
  Serial.println();
  Serial.println("Preparando nova calibracao...");
  Serial.println("Ordem recomendada: 25 -> 50 -> 75 -> 100 mg/L");
  Serial.println();

  for (int i = 0; i < NUM_PONTOS; i++) {
    Serial.println("==================================================");
    Serial.print("Prepare a solucao de ");
    Serial.print(concentracoes[i], 1);
    Serial.println(" mg/L");
    Serial.println("Procedimento recomendado:");
    Serial.println("1. Lavar com agua deionizada");
    Serial.println("2. Secar levemente sem esfregar");
    Serial.println("3. Enxaguar com a propria solucao");
    Serial.println("4. Mergulhar o sensor");
    Serial.println("5. Pressionar ENTER para iniciar");
    Serial.println("==================================================");

    esperarEnter("Pressione ENTER para iniciar a leitura deste ponto.");

    float adcEstavel = 0.0;
    float tensaoEstavel = 0.0;

    bool estabilizou = capturarPontoEstabilizado(adcEstavel, tensaoEstavel);

    adcFinal[i] = adcEstavel;
    tensaoFinal[i] = tensaoEstavel;
    logConc[i] = log10(concentracoes[i]);

    mostrarResumoPonto(i, estabilizou);
  }

  calcularRegressaoLinear(logConc, tensaoFinal, NUM_PONTOS, ultimoA, ultimoB, ultimoR2);
  calibracaoRealizada = true;

  mostrarTabelaFinal();
  mostrarResultadoFinal(ultimoA, ultimoB, ultimoR2);
}

// ======================================================
// SETUP E LOOP
// ======================================================
void setup() {
  Serial.begin(115200);
  delay(1500);
  analogReadResolution(12);

  mostrarCabecalho();
  mostrarMenu(calibracaoRealizada);
}

void loop() {
  if (Serial.available()) {
    char opcao = lerOpcaoMenu();

    if (opcao == 'C') {
      executarCalibracao();
    }
    else if (opcao == 'R') {
      executarCalibracao();
    }
    else if (opcao == 'M') {
      if (calibracaoRealizada) {
        mostrarTabelaFinal();
        mostrarResultadoFinal(ultimoA, ultimoB, ultimoR2);
      } else {
        Serial.println("Nenhuma calibracao foi realizada ainda.");
        Serial.println();
      }
    }

    mostrarMenu(calibracaoRealizada);
  }
}