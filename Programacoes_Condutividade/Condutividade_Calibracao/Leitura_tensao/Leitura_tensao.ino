#define TDS_PIN 32          // GPIO ADC (32–39)
#define NUM_AMOSTRAS 100    // número de leituras para média

float lerTensaoMedia() {

  // Descarta leituras iniciais (estabilização do ADC)
  for (int i = 0; i < 10; i++) {
    analogRead(TDS_PIN);
    delay(5);
  }

  uint32_t soma = 0;

  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    soma += analogRead(TDS_PIN);
    delay(5);
  }

  float mediaADC = soma / (float)NUM_AMOSTRAS;

  float tensao = mediaADC * (3.3 / 4095.0);

  return tensao;
}

void setup() {
  Serial.begin(115200);

  analogReadResolution(12);  
  analogSetPinAttenuation(TDS_PIN, ADC_11db);  
  // ADC_11db permite leitura até ~3.3–3.6V

  Serial.println("Sistema pronto.");
  Serial.println("Coloque a sonda na solução padrão e pressione ENTER.");
}

void loop() {

  if (Serial.available()) { //“Chegou algum dado? Então medir , ao precionar enter ativa essa linha

    Serial.read();  // limpa buffer

    Serial.println("Aguardando estabilização...");
    delay(5000);   // tempo para estabilizar na solução

    float tensao = lerTensaoMedia();

    Serial.print("Tensão média medida: ");
    Serial.print(tensao, 5);
    Serial.println(" V");

    Serial.println("----------------------------------");
    Serial.println("Troque a solução e pressione ENTER novamente.");
  }
}