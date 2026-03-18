#define PH_PIN 34

const int numLeituras = 30;

// Função para média
float lerMediaTensao() {
  long soma = 0;

  for (int i = 0; i < numLeituras; i++) {
    int raw = analogRead(PH_PIN);
    soma += raw;
    delay(100);
  }

  float mediaRaw = soma / (float)numLeituras;

  // Converter para tensão (ESP32 12 bits)
  float voltage = mediaRaw * (3.3 / 4095.0);

  return voltage;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=== CALIBRACAO pH ===");
  Serial.println("Coloque a sonda na solucao pH 4");
  Serial.println("Pressione ENTER para iniciar...");
  while (!Serial.available());
  Serial.read();

  delay(3000); // tempo para estabilizar

  float V4 = lerMediaTensao();

  Serial.print("Media pH 4: ");
  Serial.print(V4, 4);
  Serial.println(" V");

  Serial.println("\nLave a sonda e coloque na solucao pH 7");
  Serial.println("Pressione ENTER para continuar...");
  while (!Serial.available());
  Serial.read();

  delay(3000);

  float V7 = lerMediaTensao();

  Serial.print("Media pH 7: ");
  Serial.print(V7, 4);
  Serial.println(" V");

  // Calculo dos coeficientes
  float m = (4.0 - 7.0) / (V4 - V7);
  float b = 7.0 - (m * V7);

  Serial.println("\n=== RESULTADO ===");
  Serial.print("m = ");
  Serial.println(m, 6);

  Serial.print("b = ");
  Serial.println(b, 6);

  Serial.println("\nEquacao final:");
  Serial.print("pH = ");
  Serial.print(m, 6);
  Serial.print(" * V + ");
  Serial.println(b, 6);

  Serial.println("\nCopie esses valores para o codigo principal.");
}

void loop() {
  // Nada aqui (calibração roda uma vez)
}