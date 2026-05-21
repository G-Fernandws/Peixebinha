/*
  Leitura de tensão (mV) de sensor ISE de amônia
  ESP32 + entrada analógica GPIO 35

  O código realiza:
  - leitura ADC do ESP32
  - média de múltiplas amostras
  - conversão para tensão em mV
  - exibição no Monitor Serial

  Conexão:
  Saída analógica do circuito condicionador do ISE -> GPIO 35
  GND -> GND do ESP32
  VCC -> 3.3V ou 5V (dependendo do módulo utilizado)

  IMPORTANTE:
  O eletrodo ISE não deve ser ligado diretamente ao ESP32.
  Deve existir um circuito condicionador/amplificador de alta impedância.
*/

const int pinoISE = 33;

const int numAmostras = 25;

void setup() {
  Serial.begin(115200);

  // Configuração ADC ESP32
  analogReadResolution(12); // faixa 0 - 4095

  // Atenuação do ADC
  analogSetPinAttenuation(pinoISE, ADC_11db);

  Serial.println("Leitura sensor ISE de amonia");
}

void loop() {

  long soma = 0;

  // Média de leituras para reduzir ruído
  for (int i = 0; i < numAmostras; i++) {
    soma += analogRead(pinoISE);
    delay(10);
  }

  float mediaADC = soma / (float)numAmostras;

  // Conversão ADC -> tensão
  float tensao = (mediaADC / 4095.0) * 3.3;

  // Conversão para mV
  float tensao_mV = tensao * 1000.0;

  Serial.print("ADC: ");
  Serial.print(mediaADC);

  Serial.print(" | Tensao: ");
  Serial.print(tensao_mV, 2);
  Serial.println(" mV");

  delay(1000);
}