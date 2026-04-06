#define PH_PIN 34

const int NUM_LEITURAS = 30;
const int INTERVALO_LEITURA_MS = 100;
const float VREF = 3.3;
const int ADC_RESOLUCAO = 4095;

// ------------------------
// Funções auxiliares
// ------------------------

void limparBufferSerial() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}

void esperarEnter() {
  while (true) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        break;
      }
    }
  }
  delay(50);
  limparBufferSerial();
}

void aguardarComContagem(int segundos) {
  for (int i = segundos; i > 0; i--) {
    Serial.print("Iniciando leitura em ");
    Serial.print(i);
    Serial.println(" s...");
    delay(1000);
  }
}

float converterParaTensao(float mediaRaw) {
  return mediaRaw * (VREF / ADC_RESOLUCAO);
}

float lerMediaTensao() {
  long soma = 0;

  Serial.println("Coletando leituras:");
  Serial.print("[");

  for (int i = 0; i < NUM_LEITURAS; i++) {
    int raw = analogRead(PH_PIN);
    soma += raw;

    Serial.print("#");
    delay(INTERVALO_LEITURA_MS);
  }

  Serial.println("]");

  float mediaRaw = soma / (float)NUM_LEITURAS;
  return converterParaTensao(mediaRaw);
}

float lerDesvioAproximado(float media) {
  float somaQuadrados = 0.0;

  for (int i = 0; i < NUM_LEITURAS; i++) {
    float raw = analogRead(PH_PIN);
    float tensao = converterParaTensao(raw);
    somaQuadrados += (tensao - media) * (tensao - media);
    delay(INTERVALO_LEITURA_MS);
  }

  float variancia = somaQuadrados / NUM_LEITURAS;
  return sqrt(variancia);
}

float realizarLeituraCalibracao(float phReferencia) {
  Serial.println("========================================");
  Serial.print("Prepare a solução tampão pH ");
  Serial.println(phReferencia, 1);
  Serial.println("1) Enxágue a sonda");
  Serial.println("2) Seque suavemente sem friccionar");
  Serial.println("3) Mergulhe a sonda na solução");
  Serial.println("4) Aguarde estabilização");
  Serial.println("5) Pressione ENTER no Monitor Serial para iniciar");
  Serial.println("========================================");

  esperarEnter();
  aguardarComContagem(3);

  float media = lerMediaTensao();

  Serial.print("Tensão média medida em pH ");
  Serial.print(phReferencia, 1);
  Serial.print(": ");
  Serial.print(media, 4);
  Serial.println(" V");

  float desvio = lerDesvioAproximado(media);

  Serial.print("Variação aproximada das leituras: ");
  Serial.print(desvio, 5);
  Serial.println(" V");

  if (desvio > 0.02) {
    Serial.println("ATENCAO: leitura com variacao relativamente alta.");
    Serial.println("Verifique agitacao da amostra, bolhas, mau contato ou tempo insuficiente de estabilizacao.");
  } else {
    Serial.println("Leitura considerada estavel.");
  }

  Serial.println();
  return media;
}

void mostrarResumoFinal(float V4, float V7, float m, float b) {
  Serial.println("========================================");
  Serial.println("        RESULTADO DA CALIBRACAO");
  Serial.println("========================================");

  Serial.print("Tensao em pH 4,0: ");
  Serial.print(V4, 4);
  Serial.println(" V");

  Serial.print("Tensao em pH 7,0: ");
  Serial.print(V7, 4);
  Serial.println(" V");

  Serial.println();
  Serial.println("Coeficientes da equacao linear:");
  Serial.print("m = ");
  Serial.println(m, 6);

  Serial.print("b = ");
  Serial.println(b, 6);

  Serial.println();
  Serial.println("Equacao final de conversao:");
  Serial.print("pH = ");
  Serial.print(m, 6);
  Serial.print(" * V + ");
  Serial.println(b, 6);

  Serial.println();
  Serial.println("Copie os valores abaixo para o codigo principal:");
  Serial.print("float m = ");
  Serial.print(m, 6);
  Serial.println(";");

  Serial.print("float b = ");
  Serial.print(b, 6);
  Serial.println(";");

  Serial.println("========================================");
}

// ------------------------
// Setup principal
// ------------------------

void setup() {
  Serial.begin(115200);
  delay(2000);

  analogReadResolution(12); // ESP32 com ADC de 12 bits

  Serial.println();
  Serial.println("========================================");
  Serial.println("      ROTINA DE CALIBRACAO DO pH");
  Serial.println("========================================");
  Serial.println("Esta rotina realiza calibracao em 2 pontos:");
  Serial.println("- tampao pH 4,0");
  Serial.println("- tampao pH 7,0");
  Serial.println();
  Serial.println("Importante:");
  Serial.println("- use solucoes tampao confiaveis");
  Serial.println("- enxague a sonda entre as etapas");
  Serial.println("- evite bolhas na regiao sensora");
  Serial.println("- mantenha a sonda estabilizada");
  Serial.println();
  Serial.println("Pressione ENTER para comecar.");
  Serial.println("========================================");

  esperarEnter();

  float V4 = realizarLeituraCalibracao(4.0);
  float V7 = realizarLeituraCalibracao(7.0);

  if (abs(V4 - V7) < 0.001) {
    Serial.println("ERRO: as tensoes de pH 4 e pH 7 ficaram muito proximas.");
    Serial.println("Nao foi possivel calcular a reta de calibracao com seguranca.");
    Serial.println("Revise conexoes, sonda, condicionamento de sinal e solucoes tampao.");
    return;
  }

  float m = (4.0 - 7.0) / (V4 - V7);
  float b = 7.0 - (m * V7);

  mostrarResumoFinal(V4, V7, m, b);

  Serial.println("Calibracao concluida.");
  Serial.println("Reinicie o equipamento para repetir o processo.");
}

void loop() {
  // Rotina executada apenas uma vez
}