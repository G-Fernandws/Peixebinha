// ============================================================
//  Monitoramento de Turbidez — ESP32
//  Sensor de turbidez com divisor de tensão (R1=10kΩ, R2=20kΩ)
//  Pino: D35 (somente entrada — ideal para ADC)
// ============================================================
//  Divisor de tensão:
//    Sinal sensor ── R1 (10kΩ) ──┬── D35 (ESP32)
//                                R2 (20kΩ)
//                                │
//                               GND
//  Fator do divisor: R2/(R1+R2) = 20k/30k = 0.6667
//  Tensão máx. no pino: 4.5V × 0.6667 = 3.0V ✓
// ============================================================

#define TURB_PIN        35      // D35 — somente entrada (input-only)

const int   NUM_AMOSTRAS   = 30;
const float V_REF          = 3.3;
const float ADC_RES        = 4095.0;
const float FATOR_DIVISOR  = 0.6667;  // R2 / (R1 + R2)

// ─────────────────────────────────────────────────────────────
//  Função auxiliar — média de leituras analógicas
// ─────────────────────────────────────────────────────────────
float lerAnalogico(int pino)
{
  long soma = 0;
  for (int i = 0; i < NUM_AMOSTRAS; i++)
  {
    soma += analogRead(pino);
    delay(10);
  }
  return soma / (float)NUM_AMOSTRAS;
}

// ─────────────────────────────────────────────────────────────
//  Setup
// ─────────────────────────────────────────────────────────────
void setup()
{
  Serial.begin(115200);
  delay(500);

  Serial.println("===========================================");
  Serial.println("  Monitoramento de Turbidez");
  Serial.println("===========================================");
  Serial.println("Pino     : D35");
  Serial.println("Divisor  : R1=10kΩ / R2=20kΩ");
  Serial.println("-------------------------------------------");
  delay(1000);
}

// ─────────────────────────────────────────────────────────────
//  Loop principal
// ─────────────────────────────────────────────────────────────
void loop()
{
  float mediaADC = lerAnalogico(TURB_PIN);

  // Tensão lida no pino do ESP32 (após divisor)
  float tensao_esp = (mediaADC / ADC_RES) * V_REF;

  // Tensão real na saída do sensor (reverte o divisor)
  float tensao_real = tensao_esp / FATOR_DIVISOR;

  // ── Exibição no Monitor Serial ────────────────────────────
  Serial.println("-------------------------------------------");

  Serial.print("ADC bruto       : ");
  Serial.println(mediaADC, 1);

  Serial.print("Tensao no pino  : ");
  Serial.print(tensao_esp, 4);
  Serial.println(" V");

  Serial.print("Tensao do sensor: ");
  Serial.print(tensao_real, 4);
  Serial.println(" V  [apos reverter divisor]");

  delay(2000);
}
