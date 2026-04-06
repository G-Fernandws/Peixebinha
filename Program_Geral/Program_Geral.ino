// ============================================================
//  Monitoramento de Qualidade da Água — ESP32
//  Parâmetros: Temperatura, Condutividade (EC25), TDS e pH
//  EC normalizada a 25 °C por compensação de temperatura
// ============================================================
//  Pinos:
//    Temperatura  → D4  (DS18B20, protocolo OneWire)
//    pH           → D34 (analógico)
//    Condutividade→ D32 (analógico)
// ============================================================

#include <OneWire.h>
#include <DallasTemperature.h>

// ── Temperatura ──────────────────────────────────────────────
#define TEMP_PIN        4      
OneWire oneWire(TEMP_PIN);
DallasTemperature sensorTemp(&oneWire);

// ── pH ───────────────────────────────────────────────────────
#define PH_PIN          34      
const float PH_SLOPE     = -9.0909;
const float PH_INTERCEPT =  21.86;

// ── Condutividade / TDS ───────────────────────────────────────
#define EC_PIN          32     
const float FATOR_TDS   = 0.5;  // fator de conversão EC → TDS

// ── Compensação de temperatura (EC) ──────────────────────────
// Modelo linear padrão: EC25 = EC_bruta / (1 + α·(T - 25))
// α = 0.02 → coeficiente típico para águas naturais (2 %/°C)
// Referência: 25 °C (padrão internacional ABNT/ISO)
const float TEMP_REF    = 25.0; // °C de referência
const float ALPHA_EC    = 0.02; // coeficiente de compensação (2 %/°C)

// ── Parâmetros comuns ─────────────────────────────────────────
const int  NUM_AMOSTRAS = 30;   // média de leituras analógicas
const float V_REF       = 3.3;
const float ADC_RES     = 4095.0;

// ─────────────────────────────────────────────────────────────
//  Funções auxiliares
// ─────────────────────────────────────────────────────────────

/** Retorna a média de NUM_AMOSTRAS leituras de um pino analógico */
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

/** Lê e retorna a temperatura em °C (primeiro sensor encontrado) */
float lerTemperatura()
{
  sensorTemp.requestTemperatures();
  return sensorTemp.getTempCByIndex(0);
}

/** Lê e retorna o pH calculado */
float lerPH()
{
  float mediaADC = lerAnalogico(PH_PIN);
  float tensao   = (mediaADC / ADC_RES) * V_REF;
  return (PH_SLOPE * tensao) + PH_INTERCEPT;
}

/**
 * Lê a condutividade bruta, aplica compensação de temperatura e
 * retorna EC25 (uS/cm a 25 °C). Atualiza TDS por referência.
 *
 * Modelo de compensação linear (padrão IEC 60746-3 / ABNT):
 *   EC25 = EC_bruta / (1 + α · (T - T_ref))
 *
 * @param temperatura  Leitura atual do sensor DS18B20 (°C)
 * @param tds          Variável de saída: TDS compensado (mg/L)
 * @return             EC compensada a 25 °C (uS/cm)
 */
float lerCondutividade(float temperatura, float &tds)
{
  float mediaADC      = lerAnalogico(EC_PIN);
  float tensao        = mediaADC * (V_REF / ADC_RES);
  float ec_bruta      = (tensao + 0.0572) / 0.0013;   // equação de calibração

  // Fator de compensação; protege divisão por zero caso α·ΔT = -1
  float fatorComp     = 1.0 + ALPHA_EC * (temperatura - TEMP_REF);
  if (fatorComp <= 0) fatorComp = 0.001;               // segurança numérica

  float ec25          = ec_bruta / fatorComp;          // EC normalizada a 25 °C
  tds                 = ec25 * FATOR_TDS;
  return ec25;
}

// ─────────────────────────────────────────────────────────────
//  Setup
// ─────────────────────────────────────────────────────────────
void setup()
{
  Serial.begin(115200);
  delay(500);

  sensorTemp.begin();
  int nDisp = sensorTemp.getDeviceCount();

  Serial.println("===========================================");
  Serial.println("  Monitoramento de Qualidade da Água");
  Serial.println("===========================================");
  Serial.print("Sensores DS18B20 encontrados: ");
  Serial.println(nDisp);
  if (nDisp == 0)
    Serial.println("[AVISO] Nenhum sensor de temperatura detectado!");
  Serial.println("-------------------------------------------");
  delay(1000);
}

// ─────────────────────────────────────────────────────────────
//  Loop principal
// ─────────────────────────────────────────────────────────────
void loop()
{
  float temperatura   = lerTemperatura();
  float pH            = lerPH();
  float tds           = 0;
  float ec25          = lerCondutividade(temperatura, tds); // EC compensada a 25 °C

  // ── Exibição no Monitor Serial ────────────────────────────
  Serial.println("-------------------------------------------");

  Serial.print("Temperatura           : ");
  Serial.print(temperatura, 2);
  Serial.println(" °C");

  Serial.print("Condutividade (EC25)  : ");
  Serial.print(ec25, 2);
  Serial.println(" uS/cm  [compensada]");

  Serial.print("TDS                   : ");
  Serial.print(tds, 2);
  Serial.println(" mg/L");

  Serial.print("pH                    : ");
  Serial.println(pH, 2);

  delay(2000); // aguarda 2 s antes da próxima leitura
}
