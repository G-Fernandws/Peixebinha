// ============================================================
//  Monitoramento de Qualidade da Água — ESP32
//  Parâmetros: Temperatura, Condutividade (EC25), TDS e pH
//  Valores finais = média de 100 leituras de cada sensor
// ============================================================
//  Pinos:
//    Temperatura   → D4  (DS18B20, protocolo OneWire)
//    pH            → D34 (analógico)
//    Condutividade → D32 (analógico)
// ============================================================

#include <OneWire.h>
#include <DallasTemperature.h>

// ── Temperatura ──────────────────────────────────────────────
#define TEMP_PIN 4
OneWire oneWire(TEMP_PIN);
DallasTemperature sensorTemp(&oneWire);

// ── pH ───────────────────────────────────────────────────────
#define PH_PIN 34
const float PH_SLOPE     = -9.0909;
const float PH_INTERCEPT = 21.86;

// ── Condutividade / TDS ─────────────────────────────────────
#define EC_PIN 32
const float FATOR_TDS = 0.5;   // fator de conversão EC → TDS

// ── Compensação de temperatura (EC) ─────────────────────────
const float TEMP_REF = 25.0;   // °C
const float ALPHA_EC = 0.02;   // 2 %/°C

// ── Parâmetros comuns ───────────────────────────────────────
const int NUM_AMOSTRAS = 100;  // média final de 100 leituras
const float V_REF      = 3.3;
const float ADC_RES    = 4095.0;

// ─────────────────────────────────────────────────────────────
//  Funções auxiliares
// ─────────────────────────────────────────────────────────────

/** Retorna a média de NUM_AMOSTRAS leituras analógicas */
float lerMediaAnalogica(int pino)
{
  long soma = 0;

  for (int i = 0; i < NUM_AMOSTRAS; i++)
  {
    soma += analogRead(pino);
    delay(10);
  }

  return soma / (float)NUM_AMOSTRAS;
}

/** Retorna a média de 100 leituras de temperatura */
float lerMediaTemperatura()
{
  float somaTemp = 0.0;
  int leiturasValidas = 0;

  for (int i = 0; i < NUM_AMOSTRAS; i++)
  {
    sensorTemp.requestTemperatures();
    float temp = sensorTemp.getTempCByIndex(0);

    // DS18B20 retorna -127 °C em caso de erro de leitura
    if (temp != DEVICE_DISCONNECTED_C && temp > -55.0 && temp < 125.0)
    {
      somaTemp += temp;
      leiturasValidas++;
    }

    delay(50);
  }

  if (leiturasValidas == 0)
    return DEVICE_DISCONNECTED_C;

  return somaTemp / leiturasValidas;
}

/** Retorna o pH médio com base na média de 100 leituras analógicas */
float lerPHMedio()
{
  float mediaADC = lerMediaAnalogica(PH_PIN);
  float tensao = (mediaADC / ADC_RES) * V_REF;
  float pH = (PH_SLOPE * tensao) + PH_INTERCEPT;
  return pH;
}

/**
 * Retorna a EC25 média a partir da média de 100 leituras analógicas
 * e calcula o TDS correspondente.
 */
float lerCondutividadeMedia(float temperaturaMedia, float &tds)
{
  float mediaADC = lerMediaAnalogica(EC_PIN);
  float tensao = mediaADC * (V_REF / ADC_RES);

  // Equação de calibração da condutividade
  float ec_bruta = (tensao + 0.0572) / 0.0013;

  // Compensação para 25 °C
  float fatorComp = 1.0 + ALPHA_EC * (temperaturaMedia - TEMP_REF);
  if (fatorComp <= 0) fatorComp = 0.001;

  float ec25 = ec_bruta / fatorComp;
  tds = ec25 * FATOR_TDS;

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
  Serial.println("  Monitoramento de Qualidade da Agua");
  Serial.println("===========================================");
  Serial.print("Sensores DS18B20 encontrados: ");
  Serial.println(nDisp);

  if (nDisp == 0)
    Serial.println("[AVISO] Nenhum sensor de temperatura detectado!");

  Serial.println("-------------------------------------------");
  Serial.println("Cada valor exibido corresponde a media de 100 leituras.");
  delay(1000);
}

// ─────────────────────────────────────────────────────────────
//  Loop principal
// ─────────────────────────────────────────────────────────────
void loop()
{
  float temperatura = lerMediaTemperatura();
  float pH = lerPHMedio();
  float tds = 0.0;
  float ec25 = 0.0;

  if (temperatura != DEVICE_DISCONNECTED_C)
  {
    ec25 = lerCondutividadeMedia(temperatura, tds);
  }

  Serial.println("-------------------------------------------");

  Serial.print("Temperatura media (100 leituras)  : ");
  if (temperatura == DEVICE_DISCONNECTED_C)
    Serial.println("ERRO");
  else
  {
    Serial.print(temperatura, 2);
    Serial.println(" °C");
  }

  Serial.print("Condutividade media EC25          : ");
  if (temperatura == DEVICE_DISCONNECTED_C)
    Serial.println("ERRO");
  else
  {
    Serial.print(ec25, 2);
    Serial.println(" uS/cm");
  }

  Serial.print("TDS medio                         : ");
  if (temperatura == DEVICE_DISCONNECTED_C)
    Serial.println("ERRO");
  else
  {
    Serial.print(tds, 2);
    Serial.println(" mg/L");
  }

  Serial.print("pH medio                          : ");
  Serial.println(pH, 2);

  delay(2000);
}