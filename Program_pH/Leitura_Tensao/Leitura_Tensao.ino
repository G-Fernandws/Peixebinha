#define PINO_PH 33

float lerTensaoFiltrada() {
  const int N = 50;
  int leituras[N];

  // Coleta
  for (int i = 0; i < N; i++) {
    leituras[i] = analogRead(PINO_PH);
    delay(5);
  }

  // Média inicial
  float soma = 0;
  for (int i = 0; i < N; i++) {
    soma += leituras[i];
  }
  float media = soma / N;

  // Remoção de outliers
  float soma_filtrada = 0;
  int count = 0;

  for (int i = 0; i < N; i++) {
    if (abs(leituras[i] - media) < 50) { // tolerância
      soma_filtrada += leituras[i];
      count++;
    }
  }

  float media_final = soma_filtrada / count;

  // Conversão para tensão
  float tensao = media_final * (3.3 / 4095.0);

  return tensao;
}

void setup() {
  Serial.begin(115200);

  // Configuração do ADC (melhora estabilidade)
  analogReadResolution(12); // 0–4095
  analogSetAttenuation(ADC_11db); // até ~3.3V
}

void loop() {
  float tensao = lerTensaoFiltrada();

  Serial.print("Tensao: ");
  Serial.print(tensao, 4);
  Serial.println(" V");

  delay(1000);
}