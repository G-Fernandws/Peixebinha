// ============================================================
//  Monitoramento de Qualidade da Água — ESP32
//  Parâmetros: Temperatura, Condutividade (EC25), TDS, pH e Nitrato
//  Valores finais = média de 100 leituras de cada sensor
//  Envio de dados: ThingSpeak via Wi-Fi
// ============================================================
//  Pinos:
//    Temperatura   → D4  (DS18B20, protocolo OneWire)
//    pH            → D34 (analógico)
//    Condutividade → D32 (analógico)
//    Nitrato       → D33 (analógico)
// ============================================================
//  ThingSpeak — campos:
//    Field 1 → Temperatura (°C)
//    Field 2 → EC25 (uS/cm)
//    Field 3 → TDS (mg/L)
//    Field 4 → pH
//    Field 5 → Nitrato (mg/L) — calibração pendente
// ============================================================

#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ── Credenciais Wi-Fi ────────────────────────────────────────
const char* WIFI_SSID     = "NOME_DA_SUA_REDE";
const char* WIFI_PASSWORD = "SENHA_DA_SUA_REDE";

// ── ThingSpeak ───────────────────────────────────────────────
const char* TS_API_KEY = WJE8WPA03PDP58KI;
const char* TS_SERVER  = "http://api.thingspeak.com/update";

// ── Temperatura ──────────────────────────────────────────────
#define TEMP_PIN 4
OneWire oneWire(TEMP_PIN);
DallasTemperature sensorTemp(&oneWire);

// ── pH ───────────────────────────────────────────────────────
#define PH_PIN 34
const float PH_SLOPE     = -9.0909;
const float PH_INTERCEPT = 21.86;

// ── Condutividade / TDS ──────────────────────────────────────
#define EC_PIN 32
const float FATOR_TDS = 0.5;

// ── Compensação de temperatura (EC) ─────────────────────────
const float TEMP_REF = 25.0;
const float ALPHA_EC = 0.02;

// ── Nitrato ──────────────────────────────────────────────────
#define NITRATO_PIN 33
// Coeficientes PROVISÓRIOS — substituir após calibração
const float NITRATO_SLOPE     = 1.0;  // ← substituir após calibração
const float NITRATO_INTERCEPT = 0.0;  // ← substituir após calibração

// ── Parâmetros comuns ────────────────────────────────────────
const int   NUM_AMOSTRAS = 100;
const float V_REF        = 3.3;
const float ADC_RES      = 4095.0;

// ─────────────────────────────────────────────────────────────
//  Funções auxiliares — leitura de sensores
// ─────────────────────────────────────────────────────────────

float lerMediaAnalogica(int pino)
{
  long soma = 0;
  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    soma += analogRead(pino);
    delay(10);
  }
  return soma / (float)NUM_AMOSTRAS;
}

float lerMediaTemperatura()
{
  float somaTemp = 0.0;
  int leiturasValidas = 0;

  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    sensorTemp.requestTemperatures();
    float temp = sensorTemp.getTempCByIndex(0);

    if (temp != DEVICE_DISCONNECTED_C && temp > -55.0 && temp < 125.0) {
      somaTemp += temp;
      leiturasValidas++;
    }
    delay(50);
  }

  if (leiturasValidas == 0) return DEVICE_DISCONNECTED_C;
  return somaTemp / leiturasValidas;
}

float lerPHMedio()
{
  float mediaADC = lerMediaAnalogica(PH_PIN);
  float tensao   = (mediaADC / ADC_RES) * V_REF;
  return (PH_SLOPE * tensao) + PH_INTERCEPT;
}

float lerCondutividadeMedia(float temperaturaMedia, float &tds)
{
  float mediaADC  = lerMediaAnalogica(EC_PIN);
  float tensao    = mediaADC * (V_REF / ADC_RES);
  float ec_bruta  = (tensao + 0.0572) / 0.0013;

  float fatorComp = 1.0 + ALPHA_EC * (temperaturaMedia - TEMP_REF);
  if (fatorComp <= 0) fatorComp = 0.001;

  float ec25 = ec_bruta / fatorComp;
  tds = ec25 * FATOR_TDS;
  return ec25;
}

float lerNitratoMedio()
{
  float mediaADC = lerMediaAnalogica(NITRATO_PIN);
  float tensao   = (mediaADC / ADC_RES) * V_REF;
  return (NITRATO_SLOPE * tensao) + NITRATO_INTERCEPT;
}

// ─────────────────────────────────────────────────────────────
//  Função de envio ao ThingSpeak
// ─────────────────────────────────────────────────────────────

void enviarThingSpeak(float temperatura, float ec25, float tds, float pH, float nitrato)
{
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Wi-Fi] Desconectado. Tentando reconectar...");
    WiFi.reconnect();
    delay(3000);
    return;
  }

  HTTPClient http;

  String url = String(TS_SERVER) + "?api_key=" + TS_API_KEY;

  if (temperatura != DEVICE_DISCONNECTED_C) {
    url += "&field1=" + String(temperatura, 2);
    url += "&field2=" + String(ec25,        2);
    url += "&field3=" + String(tds,         2);
  }
  url += "&field4=" + String(pH,      2);
  url += "&field5=" + String(nitrato, 2);

  Serial.println("[ThingSpeak] Enviando dados...");
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.print("[ThingSpeak] Resposta HTTP: ");
    Serial.println(httpCode); // 200 = sucesso
  } else {
    Serial.print("[ThingSpeak] Erro: ");
    Serial.println(http.errorToString(httpCode));
  }

  http.end();
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

  Serial.print("Conectando ao Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado!");
    Serial.print("IP do ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[AVISO] Wi-Fi nao conectado. Dados nao serao enviados.");
  }

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
  float pH          = lerPHMedio();
  float nitrato     = lerNitratoMedio();
  float tds         = 0.0;
  float ec25        = 0.0;

  if (temperatura != DEVICE_DISCONNECTED_C)
    ec25 = lerCondutividadeMedia(temperatura, tds);

  // ── Monitor serial ────────────────────────────────────────
  Serial.println("-------------------------------------------");

  Serial.print("Temperatura media (100 leituras)  : ");
  if (temperatura == DEVICE_DISCONNECTED_C)
    Serial.println("ERRO");
  else { Serial.print(temperatura, 2); Serial.println(" C"); }

  Serial.print("Condutividade media EC25          : ");
  if (temperatura == DEVICE_DISCONNECTED_C)
    Serial.println("ERRO");
  else { Serial.print(ec25, 2); Serial.println(" uS/cm"); }

  Serial.print("TDS medio                         : ");
  if (temperatura == DEVICE_DISCONNECTED_C)
    Serial.println("ERRO");
  else { Serial.print(tds, 2); Serial.println(" mg/L"); }

  Serial.print("pH medio                          : ");
  Serial.println(pH, 2);

  Serial.print("Nitrato medio                     : ");
  Serial.print(nitrato, 2);
  Serial.println(" mg/L (calibracao pendente)");

  // ── Envio ao ThingSpeak ───────────────────────────────────
  enviarThingSpeak(temperatura, ec25, tds, pH, nitrato);

  delay(8000);
}