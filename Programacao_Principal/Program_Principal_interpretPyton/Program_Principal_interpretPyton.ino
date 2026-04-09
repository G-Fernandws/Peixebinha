// ============================================================
//  Program_Principal_interpretPyton
//  Monitoramento de qualidade da água com saída serial limpa
//  para captura por Python e posterior exportação para planilha.
// ============================================================
//  Objetivo desta versão:
//  - manter a lógica principal de leitura dos sensores;
//  - remover a interface interativa do Monitor Serial;
//  - emitir uma saída serial previsível e fácil de interpretar;
//  - imprimir apenas:
//      1) um cabeçalho CSV no setup;
//      2) uma linha CSV por ciclo de medição.
//
//  Isso facilita a captura automática por um script Python,
//  evitando mistura de mensagens de menu, alertas e textos
//  descritivos no meio dos dados.
// ============================================================
//  Parâmetros medidos:
//    1) Temperatura (DS18B20)
//    2) pH
//    3) Condutividade corrigida para 25 °C (EC25)
//    4) TDS
//    5) Nitrato (ISE)
// ============================================================
//  Saída CSV:
//    ciclo,tempo_s,temperatura_C,pH,EC25_uScm,TDS_mgL,NO3_mgL


#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include <algorithm>

// ============================================================
//                    CONFIGURAÇÕES GERAIS
// ============================================================
const long SERIAL_BAUD = 115200;

// Intervalo de espera entre o fim de um ciclo e o início do próximo.
// O tempo total entre linhas CSV será maior do que este valor, porque
// as próprias leituras dos sensores já consomem tempo.
const unsigned long DELAY_ENTRE_CICLOS_MS = 2000;

// Define se as medições serão contínuas.
// Nesta versão o padrão é contínuo, porque ela foi feita para logging.
const bool MEDICAO_CONTINUA = true;

// ============================================================
//                   CONFIGURAÇÕES DE AMOSTRAGEM
// ============================================================
const int NUM_AMOSTRAS_ANALOGICAS  = 100;
const int NUM_AMOSTRAS_TEMPERATURA = 35;
const uint8_t TEMP_RESOLUTION_BITS = 12;

// ADC do ESP32.
const float V_REF   = 3.3;
const float ADC_RES = 4095.0;

// Fração descartada em cada extremidade do vetor de leituras.
const float FRAC_DESCARTE = 0.10;

// ============================================================
//                      PINOS E OBJETOS
// ============================================================
#define TEMP_PIN 4
#define PH_PIN 34
#define EC_PIN 32
#define NITRATO_PIN 33

OneWire oneWire(TEMP_PIN);
DallasTemperature sensorTemp(&oneWire);

// ============================================================
//              COEFICIENTES DE CALIBRAÇÃO — pH
// ============================================================
const float PH_SLOPE_25C     = -9.0909;
const float PH_INTERCEPT_25C = 21.86;
const float PH_TEMP_CAL      = 25.0;

// ============================================================
//            COEFICIENTES DE CALIBRAÇÃO — EC / TDS
// ============================================================
// Relação calibrada adotada na forma:
//   tensão = EC_SLOPE * EC + EC_INTERCEPT
// Portanto:
//   EC = (tensão - EC_INTERCEPT) / EC_SLOPE
const float EC_SLOPE     = 0.0013;
const float EC_INTERCEPT = -0.0572;
const float FATOR_TDS    = 0.5;
const float TEMP_REF     = 25.0;
const float ALPHA_EC     = 0.02;

// ============================================================
//             COEFICIENTES DE CALIBRAÇÃO — Nitrato
// ============================================================
const float NO3_A = -0.120835;
const float NO3_B = 2.046916;

// ============================================================
//          CONFIGURAÇÕES DE ESTABILIZAÇÃO DO NITRATO
// ============================================================
const int   NO3_LEITURAS_POR_BLOCO          = 20;
const int   NO3_MAX_BLOCOS                  = 12;
const float NO3_LIMIAR_ESTAB_V              = 0.0015;
const int   NO3_BLOCOS_ESTAVEIS_NECESSARIOS = 5; //verificaaaaaaaaaaaaaaaaaaaaaaaaaaaar
const int   NO3_DELAY_ENTRE_BLOCOS_MS       = 250;

// ============================================================
//                         ESTRUTURAS
// ============================================================
struct EstatisticaAnalogica {
  float mediaADC;
  float desvioADC;
  float mediaTensao;
  float desvioTensao;
  int   nUsado;
  bool  valido;
};

struct ResultadoPH {
  EstatisticaAnalogica estat;
  float slopeComp;
  float interceptComp;
  float pH;
  bool  valido;
};

struct ResultadoEC {
  EstatisticaAnalogica estat;
  float ecBruta;
  float ec25;
  float tds;
  bool  valido;
};

struct ResultadoNO3 {
  EstatisticaAnalogica estat;
  float nitrato;
  bool  estabilizou;
  bool  valido;
};

// ============================================================
//                        CONTROLE GLOBAL
// ============================================================
unsigned long tempoInicio    = 0;
unsigned long contadorCiclos = 0;

// ============================================================
//                        FUNÇÕES DE APOIO
// ============================================================
float calcularMedia(const float *v, int n) {
  if (n <= 0) return 0.0;
  float soma = 0.0;
  for (int i = 0; i < n; i++) soma += v[i];
  return soma / n;
}

float calcularDesvioPadrao(const float *v, int n, float media) {
  if (n < 2) return 0.0;
  float soma = 0.0;
  for (int i = 0; i < n; i++) {
    float d = v[i] - media;
    soma += d * d;
  }
  return sqrt(soma / (n - 1));
}

// ============================================================
//                   LEITURA ROBUSTA ANALÓGICA
// ============================================================
// Estratégia:
// 1) coleta nAmostras do ADC;
// 2) ordena os valores;
// 3) descarta extremos;
// 4) calcula média e desvio no miolo;
// 5) converte estatísticas ADC -> tensão.
EstatisticaAnalogica lerAnalogicoRobusto(int pino, int nAmostras, int delayMs) {
  EstatisticaAnalogica r = {0, 0, 0, 0, 0, false};

  if (nAmostras < 5 || nAmostras > NUM_AMOSTRAS_ANALOGICAS) return r;

  float leituras[NUM_AMOSTRAS_ANALOGICAS];

  for (int i = 0; i < nAmostras; i++) {
    leituras[i] = (float)analogRead(pino);
    delay(delayMs);
  }

  std::sort(leituras, leituras + nAmostras);

  int nDesc = (int)(nAmostras * FRAC_DESCARTE);
  if (nDesc < 1) nDesc = 1;

  int inicio = nDesc;
  int fim    = nAmostras - nDesc;
  if (fim <= inicio) return r;

  int nUsado = fim - inicio;
  const float escala = V_REF / ADC_RES;

  float mediaADC  = calcularMedia(leituras + inicio, nUsado);
  float desvioADC = calcularDesvioPadrao(leituras + inicio, nUsado, mediaADC);

  r.mediaADC      = mediaADC;
  r.desvioADC     = desvioADC;
  r.mediaTensao   = mediaADC * escala;
  r.desvioTensao  = desvioADC * escala;
  r.nUsado        = nUsado;
  r.valido        = true;

  return r;
}

// ============================================================
//                          TEMPERATURA
// ============================================================
// Cada chamada de requestTemperatures() roda em modo bloqueante,
// portanto cada amostra usada na média corresponde a uma nova
// conversão do DS18B20.
float lerMediaTemperatura() {
  float somaTemp      = 0.0;
  int leiturasValidas = 0;

  sensorTemp.setWaitForConversion(true);

  for (int i = 0; i < NUM_AMOSTRAS_TEMPERATURA; i++) {
    sensorTemp.requestTemperatures();
    float temp = sensorTemp.getTempCByIndex(0);

    if (temp != DEVICE_DISCONNECTED_C && temp > -55.0 && temp < 125.0) {
      somaTemp += temp;
      leiturasValidas++;
    }
  }

  if (leiturasValidas == 0) return DEVICE_DISCONNECTED_C;
  return somaTemp / leiturasValidas;
}

// ============================================================
//                              pH
// ============================================================
ResultadoPH lerPHCompensado(float temperaturaMedia) {
  ResultadoPH r = {{0, 0, 0, 0, 0, false}, 0, 0, 0, false};

  r.estat = lerAnalogicoRobusto(PH_PIN, NUM_AMOSTRAS_ANALOGICAS, 10);
  if (!r.estat.valido) return r;

  float tempUsada = (temperaturaMedia == DEVICE_DISCONNECTED_C)
                    ? PH_TEMP_CAL
                    : temperaturaMedia;

  float Tk_med = tempUsada + 273.15;
  float Tk_cal = PH_TEMP_CAL + 273.15;

  r.slopeComp = PH_SLOPE_25C * (Tk_cal / Tk_med);

  // Mantém o ponto de referência da calibração no reajuste térmico.
  float vIso      = (7.0 - PH_INTERCEPT_25C) / PH_SLOPE_25C;
  r.interceptComp = 7.0 - (r.slopeComp * vIso);

  r.pH = (r.slopeComp * r.estat.mediaTensao) + r.interceptComp;

  if (isnan(r.pH) || isinf(r.pH) || r.pH < 0.0 || r.pH > 14.5) return r;

  r.valido = true;
  return r;
}

// ============================================================
//                       CONDUTIVIDADE / TDS
// ============================================================
ResultadoEC lerEC(float temperaturaMedia) {
  ResultadoEC r = {{0, 0, 0, 0, 0, false}, 0, 0, 0, false};

  r.estat = lerAnalogicoRobusto(EC_PIN, NUM_AMOSTRAS_ANALOGICAS, 10);
  if (!r.estat.valido) return r;

  // Conversão da tensão para condutividade bruta usando a reta de calibração.
  if (fabs(EC_SLOPE) < 1e-9) return r;
  r.ecBruta = (r.estat.mediaTensao - EC_INTERCEPT) / EC_SLOPE;

  float tempUsada = (temperaturaMedia == DEVICE_DISCONNECTED_C)
                    ? TEMP_REF
                    : temperaturaMedia;

  float fatorComp = 1.0 + ALPHA_EC * (tempUsada - TEMP_REF);
  if (fatorComp <= 0.0) fatorComp = 0.001;

  r.ec25 = r.ecBruta / fatorComp;
  r.tds  = r.ec25 * FATOR_TDS;

  if (isnan(r.ec25) || isinf(r.ec25) || r.ec25 < 0.0) return r;

  r.valido = true;
  return r;
}

// ============================================================
//                    NITRATO COM ESTABILIZAÇÃO
// ============================================================
ResultadoNO3 lerNitratoEstabilizado() {
  ResultadoNO3 r = {{0, 0, 0, 0, 0, false}, 0, false, false};

  float tensaoAnterior = 0.0;
  bool primeiroBloco   = true;
  int blocosEstaveis   = 0;

  float tensoesEstaveis[NO3_MAX_BLOCOS];
  float adcsEstaveis[NO3_MAX_BLOCOS];
  int nEstaveis = 0;

  EstatisticaAnalogica ultimoBloco = {0, 0, 0, 0, 0, false};

  for (int bloco = 1; bloco <= NO3_MAX_BLOCOS; bloco++) {
    EstatisticaAnalogica estatBloco = lerAnalogicoRobusto(
      NITRATO_PIN, NO3_LEITURAS_POR_BLOCO, 20
    );

    if (!estatBloco.valido) continue;

    ultimoBloco = estatBloco;

    if (!primeiroBloco) {
      float dV = estatBloco.mediaTensao - tensaoAnterior;

      if (fabs(dV) <= NO3_LIMIAR_ESTAB_V) {
        blocosEstaveis++;

        if (nEstaveis < NO3_MAX_BLOCOS) {
          tensoesEstaveis[nEstaveis] = estatBloco.mediaTensao;
          adcsEstaveis[nEstaveis]    = estatBloco.mediaADC;
          nEstaveis++;
        }
      } else {
        blocosEstaveis = 0;
        nEstaveis      = 0;
      }
    }

    tensaoAnterior = estatBloco.mediaTensao;
    primeiroBloco  = false;
    delay(NO3_DELAY_ENTRE_BLOCOS_MS);

    if (blocosEstaveis >= NO3_BLOCOS_ESTAVEIS_NECESSARIOS && nEstaveis > 0) {
      float mediaADC  = calcularMedia(adcsEstaveis, nEstaveis);
      float mediaV    = calcularMedia(tensoesEstaveis, nEstaveis);
      float desvioADC = calcularDesvioPadrao(adcsEstaveis, nEstaveis, mediaADC);
      float desvioV   = calcularDesvioPadrao(tensoesEstaveis, nEstaveis, mediaV);

      r.estat.mediaADC     = mediaADC;
      r.estat.desvioADC    = desvioADC;
      r.estat.mediaTensao  = mediaV;
      r.estat.desvioTensao = desvioV;
      r.estat.nUsado       = nEstaveis;
      r.estat.valido       = true;
      r.estabilizou        = true;
      break;
    }
  }

  if (!r.estabilizou) {
    r.estat = ultimoBloco;
  }

  if (!r.estat.valido) return r;

  r.nitrato = pow(10.0, (r.estat.mediaTensao - NO3_B) / NO3_A);

  if (isnan(r.nitrato) || isinf(r.nitrato) || r.nitrato < 0.0) return r;

  r.valido = true;
  return r;
}

// ============================================================
//                         IMPRESSÃO CSV
// ============================================================
// Imprime o cabeçalho exatamente uma vez no setup.
// O script Python usará essa linha para reconhecer a estrutura.
void imprimirCabecalhoCSV() {
  Serial.println(F("ciclo,tempo_s,temperatura_C,pH,EC25_uScm,TDS_mgL,NO3_mgL"));
}

// Imprime uma única linha CSV por ciclo.
// Regras:
// - ordem fixa das colunas;
// - se um valor não for válido, imprime "nan".
void imprimirLinhaCSV(float temperatura, ResultadoPH ph, ResultadoEC ec, ResultadoNO3 no3) {
  Serial.print(contadorCiclos);
  Serial.print(',');
  Serial.print((millis() - tempoInicio) / 1000.0, 1);
  Serial.print(',');

  if (temperatura == DEVICE_DISCONNECTED_C) Serial.print(F("nan"));
  else Serial.print(temperatura, 2);
  Serial.print(',');

  if (ph.valido) Serial.print(ph.pH, 2);
  else Serial.print(F("nan"));
  Serial.print(',');

  if (ec.valido) Serial.print(ec.ec25, 2);
  else Serial.print(F("nan"));
  Serial.print(',');

  if (ec.valido) Serial.print(ec.tds, 2);
  else Serial.print(F("nan"));
  Serial.print(',');

  if (no3.valido) Serial.println(no3.nitrato, 2);
  else Serial.println(F("nan"));
}

// ============================================================
//                 EXECUÇÃO DE UM CICLO DE LEITURA
// ============================================================
// Ordem adotada:
// 1) temperatura;
// 2) pH;
// 3) EC/TDS;
// 4) nitrato;
// 5) impressão da linha CSV.
void executarCicloLeitura() {
  contadorCiclos++;

  float temperatura = lerMediaTemperatura();
  ResultadoPH  ph   = lerPHCompensado(temperatura);
  ResultadoEC  ec   = lerEC(temperatura);
  ResultadoNO3 no3  = lerNitratoEstabilizado();

  imprimirLinhaCSV(temperatura, ph, ec, no3);
}

// ============================================================
//                            SETUP
// ============================================================
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1200);

  analogReadResolution(12);
  sensorTemp.begin();
  sensorTemp.setResolution(TEMP_RESOLUTION_BITS);
  sensorTemp.setWaitForConversion(true);

  tempoInicio = millis();

  // Imprime apenas o cabeçalho CSV.
  // Não há menu nem mensagens textuais extras nesta versão.
  imprimirCabecalhoCSV();
}

// ============================================================
//                             LOOP
// ============================================================
void loop() {
  executarCicloLeitura();

  if (!MEDICAO_CONTINUA) {
    while (true) {
      delay(1000);
    }
  }

  delay(DELAY_ENTRE_CICLOS_MS);
}
