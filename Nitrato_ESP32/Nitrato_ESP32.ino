// ============================================================
//  Leitura e Calibração — Sensor de Nitrato — ESP32
//  Princípio: eletrodo íon-seletivo (ISE) — potenciometria
//  Divisor de tensão já aplicado externamente
// ============================================================
//  Modo de uso:
//    1. Mergulhe o sensor na solução padrão
//    2. Aguarde estabilizar (~30 s)
//    3. Observe a tensão exibida no monitor serial
//    4. Anote: concentração (mg/L) × tensão (V)
//    5. Após coletar todos os pontos, calcule a regressão
//       linear (ex: Excel, Python, Planilha) e insira os
//       coeficientes nas constantes SLOPE e INTERCEPT abaixo
// ============================================================

#define NO3_PIN         33      // altere conforme seu pino ADC

const int   NUM_AMOSTRAS  = 50;    // mais amostras = menos ruído
const float V_REF         = 3.3;
const float ADC_RES       = 4095.0;
const float FATOR_DIVISOR = 0.6667; // R2/(R1+R2) — ajuste se usou outros valores

// ── Coeficientes de calibração (preencher após regressão) ────
// Equação: Concentracao (mg/L) = SLOPE * tensao_real + INTERCEPT
// Deixe os valores abaixo como 0 até ter os dados de calibração
const float SLOPE_CAL     = 0.0;   // inclinação  (a)
const float INTERCEPT_CAL = 0.0;   // intercepto  (b)

bool calibrado = (SLOPE_CAL != 0.0); // true quando coeficientes forem inseridos

// ─────────────────────────────────────────────────────────────
//  Função auxiliar — média filtrada de leituras analógicas
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
  Serial.println("  Sensor de Nitrato — Modo Calibracao");
  Serial.println("===========================================");
  Serial.println("Mergulhe o sensor na solucao padrao,");
  Serial.println("aguarde ~30 s e anote a tensao exibida.");
  Serial.println("-------------------------------------------");
  if (calibrado)
  {
    Serial.println("[OK] Coeficientes de calibracao inseridos.");
    Serial.print("     Slope     : "); Serial.println(SLOPE_CAL, 4);
    Serial.print("     Intercept : "); Serial.println(INTERCEPT_CAL, 4);
  }
  else
  {
    Serial.println("[!] Coeficientes ainda nao inseridos.");
    Serial.println("    Concentracao nao sera calculada.");
  }
  Serial.println("-------------------------------------------");
  delay(1000);
}

// ─────────────────────────────────────────────────────────────
//  Loop principal
// ─────────────────────────────────────────────────────────────
void loop()
{
  float mediaADC    = lerAnalogico(NO3_PIN);
  float tensao_esp  = (mediaADC / ADC_RES) * V_REF;
  float tensao_real = tensao_esp / FATOR_DIVISOR; // tensao real do sensor

  Serial.println("-------------------------------------------");

  Serial.print("ADC bruto        : ");
  Serial.println(mediaADC, 1);

  Serial.print("Tensao no pino   : ");
  Serial.print(tensao_esp, 4);
  Serial.println(" V");

  Serial.print("Tensao do sensor : ");
  Serial.print(tensao_real, 4);
  Serial.println(" V");

  // Exibe concentração apenas se os coeficientes já foram inseridos
  if (calibrado)
  {
    float concentracao = (SLOPE_CAL * tensao_real) + INTERCEPT_CAL;
    Serial.print("Nitrato (NO3-)   : ");
    Serial.print(concentracao, 2);
    Serial.println(" mg/L");
  }
  else
  {
    Serial.println("Nitrato (NO3-)   : --- (aguardando calibracao)");
  }

  delay(2000);
}
