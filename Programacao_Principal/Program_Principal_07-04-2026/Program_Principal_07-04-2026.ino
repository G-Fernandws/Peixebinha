// ============================================================
//  Monitoramento de Qualidade da Água — ESP32
//  Versão com menu interativo no Monitor Serial
// ============================================================
//  Objetivo geral:
//  Este programa realiza a leitura sequencial de cinco parâmetros
//  relevantes para monitoramento de qualidade da água:
//    1) Temperatura
//    2) pH
//    3) Condutividade elétrica compensada para 25 °C (EC25)
//    4) TDS
//    5) Nitrato por eletrodo íon-seletivo (ISE)
//
//  Filosofia de funcionamento:
//  - primeiro o sistema lê a temperatura, pois ela influencia pH e EC;
//  - depois lê os sensores analógicos com múltiplas amostras;
//  - aplica média robusta com descarte de extremos para reduzir ruído;
//  - calcula os parâmetros finais com base nos coeficientes calibrados;
//  - exibe os resultados em modo resumido ou detalhado.
// ============================================================
//  Parâmetros:
//    - Temperatura (DS18B20)
//    - pH
//    - Condutividade compensada para 25 °C (EC25)
//    - TDS
//    - Nitrato (ISE)
// ============================================================
//  Recursos:
//    - média robusta com descarte de extremos
//    - desvio padrão
//    - estabilização automática do nitrato
//    - modo resumido / detalhado
//    - saída CSV opcional
//    - menu interativo no Serial
// ============================================================
//  Pinos:
//    Temperatura   -> D4
//    pH            -> D34
//    Condutividade -> D32
//    Nitrato       -> D33
// ============================================================

#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// ============================================================
//                    CONFIGURAÇÕES GERAIS
// ============================================================
const long SERIAL_BAUD = 115200;

// ---- Estados do sistema ----
// MODO_DETALHADO: quando true, imprime variáveis intermediárias,
// desvios, tensões e status de estabilização.
// SAIDA_CSV: quando true, imprime uma linha em formato CSV, útil
// para copiar e colar em planilha ou salvar no PC.
// MEDICAO_CONTINUA: controla se o sistema seguirá medindo após o
// ciclo atual. É alterado quando pede leitura única.
// SISTEMA_PAUSADO: interrompe novos ciclos até receber comando.
bool MODO_DETALHADO = false;
bool SAIDA_CSV = false;
bool MEDICAO_CONTINUA = true;   // true = mede continuamente
bool SISTEMA_PAUSADO = false;   // true = não mede até receber comando

// ---- Temporização ----
// Intervalo entre um ciclo completo e o próximo.
// Não representa o tempo total do ciclo, pois cada sensor já consome
// tempo próprio de leitura e estabilização.
const unsigned long DELAY_ENTRE_CICLOS_MS = 2000;

// ---- Amostragem analógica ----
// Número máximo de amostras usadas nas leituras dos sensores
// analógicos. O valor real utilizado por função não pode exceder
// este limite, pois os vetores foram dimensionados com base nele.
const int NUM_AMOSTRAS_ANALOGICAS = 100;

// ---- Temperatura ----
// Número de leituras do DS18B20 para cálculo da média final.
// Aqui foi adotada média simples das leituras válidas.
const int NUM_AMOSTRAS_TEMPERATURA = 100;

// ---- ADC ESP32 ----
// V_REF  : tensão de referência assumida para conversão ADC -> volts.
// ADC_RES: resolução máxima do ADC em 12 bits no ESP32 (0 a 4095).
const float V_REF   = 3.3;
const float ADC_RES = 4095.0;

// ---- Descarte de extremos na média robusta ----
// Fração das leituras descartada em cada ponta do vetor ordenado.
// Exemplo: 10% descarta os menores 10% e os maiores 10%, reduzindo
// a influência de ruídos impulsivos e leituras espúrias.
const float FRAC_DESCARTE = 0.10;

// ============================================================
//                      PINOS E OBJETOS
// ============================================================
#define TEMP_PIN 4
OneWire oneWire(TEMP_PIN);
DallasTemperature sensorTemp(&oneWire);

// Coeficientes de calibração
// pH:
// A equação final está na forma:
//    pH = slope * tensão + intercept
// Os coeficientes a 25 °C são ajustados pela temperatura medida,
// assumindo comportamento linear do slope com a temperatura absoluta.
//
// EC:
// A condutividade bruta é calculada a partir da equação calibrada do
// sensor. Em seguida, faz-se compensação para 25 °C usando ALPHA_EC.
// TDS é estimado por fator empírico aplicado sobre EC25.
//
// Nitrato:
// A concentração é obtida por relação exponencial inversa da curva
// ajustada em log10, compatível com comportamento típico do tipo de sensor, ISE.
#define PH_PIN 33
const float PH_SLOPE_25C     = -9.0909;
const float PH_INTERCEPT_25C = 21.86;
const float PH_TEMP_CAL      = 25.0;

#define EC_PIN 32
const float FATOR_TDS = 0.5;
const float TEMP_REF  = 25.0;
const float ALPHA_EC  = 0.02;

#define NITRATO_PIN 33
const float NO3_A = -0.120835;
const float NO3_B =  2.046916;

// ============================================================
//          CONFIGURAÇÕES DE ESTABILIZAÇÃO DO NITRATO
// ============================================================
const int NO3_LEITURAS_POR_BLOCO = 20;
const int NO3_MAX_BLOCOS = 12;
const float NO3_LIMIAR_ESTAB_V = 0.0015;
const int NO3_BLOCOS_ESTAVEIS_NECESSARIOS = 3;
const int NO3_DELAY_ENTRE_BLOCOS_MS = 250;

// ============================================================
//                         ESTRUTURAS
// ============================================================
struct EstatisticaAnalogica {
  float mediaADC;
  float desvioADC;
  float mediaTensao;
  float desvioTensao;
  int nUsado;
  bool valido;
};

struct ResultadoPH {
  EstatisticaAnalogica estat;
  float slopeComp;
  float interceptComp;
  float pH;
  bool valido;
};

struct ResultadoEC {
  EstatisticaAnalogica estat;
  float ecBruta;
  float ec25;
  float tds;
  bool valido;
};

struct ResultadoNO3 {
  EstatisticaAnalogica estat;
  float nitrato;
  bool estabilizou;
  bool valido;
};

// ============================================================
//                        CONTROLE GLOBAL
// ============================================================
unsigned long tempoInicio = 0;
unsigned long contadorCiclos = 0;

// ============================================================
//                        FUNÇÕES DE APOIO
// ============================================================

// Ordena o vetor em ordem crescente.
// Esta função é usada antes do descarte de extremos, para que o
// programa consiga remover sistematicamente os menores e maiores
// valores da série de leituras.
void ordenarVetor(float *v, int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (v[j] < v[i]) {
        float temp = v[i];
        v[i] = v[j];
        v[j] = temp;
      }
    }
  }
}

// Calcula a média aritmética simples de um vetor.
float calcularMedia(const float *v, int n) {
  if (n <= 0) return 0.0;
  float soma = 0.0;
  for (int i = 0; i < n; i++) soma += v[i];
  return soma / n;
}

// Calcula o desvio-padrão amostral (n - 1 no denominador).
// Esse indicador ajuda a avaliar a estabilidade das leituras.
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
// Realiza uma leitura analógica robusta:
// 1) coleta várias amostras do pino;
// 2) ordena os valores;
// 3) descarta extremos inferiores e superiores;
// 4) calcula média e desvio no miolo da distribuição;
// 5) converte ADC médio em tensão média.
// Essa estratégia reduz efeito de ruído, picos e oscilações rápidas.
EstatisticaAnalogica lerAnalogicoRobusto(int pino, int nAmostras, int delayMs) {
  EstatisticaAnalogica r;
  r.mediaADC = 0;
  r.desvioADC = 0;
  r.mediaTensao = 0;
  r.desvioTensao = 0;
  r.nUsado = 0;
  r.valido = false;

  if (nAmostras < 5 || nAmostras > NUM_AMOSTRAS_ANALOGICAS) return r;

  float leituras[NUM_AMOSTRAS_ANALOGICAS];

  for (int i = 0; i < nAmostras; i++) {
    leituras[i] = (float)analogRead(pino);
    delay(delayMs);
  }

  ordenarVetor(leituras, nAmostras);

  // Calcula quantas leituras serão descartadas em cada extremidade.
  int nDesc = (int)(nAmostras * FRAC_DESCARTE);
  if (nDesc < 1) nDesc = 1;

  int inicio = nDesc;
  int fim = nAmostras - nDesc;

  if (fim <= inicio) return r;

  int nUsado = fim - inicio;
  float mioloADC[NUM_AMOSTRAS_ANALOGICAS];

  for (int i = 0; i < nUsado; i++) {
    mioloADC[i] = leituras[inicio + i];
  }

  float mediaADC = calcularMedia(mioloADC, nUsado);
  float desvioADC = calcularDesvioPadrao(mioloADC, nUsado, mediaADC);

  float mioloV[NUM_AMOSTRAS_ANALOGICAS];
  for (int i = 0; i < nUsado; i++) {
    mioloV[i] = (mioloADC[i] / ADC_RES) * V_REF;
  }

  float mediaV = calcularMedia(mioloV, nUsado);
  float desvioV = calcularDesvioPadrao(mioloV, nUsado, mediaV);

  r.mediaADC = mediaADC;
  r.desvioADC = desvioADC;
  r.mediaTensao = mediaV;
  r.desvioTensao = desvioV;
  r.nUsado = nUsado;
  r.valido = true;

  return r;
}

// ============================================================
// TEMPERATURA
// ============================================================
// Lê repetidamente o DS18B20 e retorna a média apenas das leituras
// consideradas fisicamente válidas. Se nenhuma leitura for válida,
// retorna DEVICE_DISCONNECTED_C como indicador de falha.
float lerMediaTemperatura() {
  float somaTemp = 0.0;
  int leiturasValidas = 0;

  for (int i = 0; i < NUM_AMOSTRAS_TEMPERATURA; i++) {
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

// ============================================================
// pH
// ============================================================
// Faz a leitura do sensor de pH e aplica compensação térmica no slope.
// Etapas:
// 1) lê a tensão média robusta;
// 2) usa a temperatura medida para ajustar a sensibilidade;
// 3) recalcula o intercepto mantendo o ponto isoelétrico da calibração;
// 4) converte tensão em pH;
// 5) valida o resultado em faixa plausível.
ResultadoPH lerPHCompensado(float temperaturaMedia) {
  ResultadoPH r;
  r.slopeComp = 0;
  r.interceptComp = 0;
  r.pH = 0;
  r.valido = false;

  r.estat = lerAnalogicoRobusto(PH_PIN, NUM_AMOSTRAS_ANALOGICAS, 10);
  if (!r.estat.valido) return r;

  float tempUsada = temperaturaMedia;
  if (tempUsada == DEVICE_DISCONNECTED_C) tempUsada = PH_TEMP_CAL;

  float Tk_med = tempUsada + 273.15;
  float Tk_cal = PH_TEMP_CAL + 273.15;

  r.slopeComp = PH_SLOPE_25C * (Tk_cal / Tk_med);

  // Determina a tensão correspondente ao ponto de pH 7 da calibração.
  // Isso permite ajustar o intercepto sem perder o ponto de referência.
  float vIso = (7.0 - PH_INTERCEPT_25C) / PH_SLOPE_25C;
  r.interceptComp = 7.0 - (r.slopeComp * vIso);

  r.pH = (r.slopeComp * r.estat.mediaTensao) + r.interceptComp;

  if (isnan(r.pH) || isinf(r.pH) || r.pH < 0.0 || r.pH > 14.5) return r;

  r.valido = true;
  return r;
}

// ============================================================
// CONDUTIVIDADE / TDS
// ============================================================
// Faz a leitura da condutividade e do TDS.
// Etapas:
// 1) lê a tensão média robusta do sensor;
// 2) converte a tensão em condutividade bruta pela equação calibrada;
// 3) compensa o valor para 25 °C;
// 4) estima TDS por fator de conversão;
// 5) rejeita resultados não plausíveis.
ResultadoEC lerEC(float temperaturaMedia) {
  ResultadoEC r;
  r.ecBruta = 0;
  r.ec25 = 0;
  r.tds = 0;
  r.valido = false;

  r.estat = lerAnalogicoRobusto(EC_PIN, NUM_AMOSTRAS_ANALOGICAS, 10);
  if (!r.estat.valido) return r;

  // Equação calibrada do sensor para converter tensão em EC bruta.
  r.ecBruta = (r.estat.mediaTensao + 0.0572) / 0.0013;

  float tempUsada = temperaturaMedia;
  if (tempUsada == DEVICE_DISCONNECTED_C) tempUsada = TEMP_REF;

  // Fator de compensação térmica para referenciar a 25 °C.
  float fatorComp = 1.0 + ALPHA_EC * (tempUsada - TEMP_REF);
  if (fatorComp <= 0.0) fatorComp = 0.001;

  r.ec25 = r.ecBruta / fatorComp;
  r.tds  = r.ec25 * FATOR_TDS;

  if (isnan(r.ec25) || isinf(r.ec25) || r.ec25 < 0.0) return r;

  r.valido = true;
  return r;
}

// ============================================================
// NITRATO COM ESTABILIZAÇÃO
// ============================================================
// Faz a leitura do nitrato com verificação de estabilização.
// Em sensores ISE, a leitura costuma demorar mais para estabilizar.
// Por isso, o algoritmo trabalha em blocos:
// 1) mede um bloco de leituras;
// 2) compara a tensão média do bloco atual com a do bloco anterior;
// 3) se a diferença ficar abaixo do limiar, considera o bloco estável;
// 4) exige uma sequência mínima de blocos estáveis;
// 5) calcula a concentração final pela curva de calibração.
//
// Se a estabilização não for confirmada no limite de blocos,
// o programa ainda usa o último bloco válido, mas marca o resultado
// como não estabilizado para alertar o usuário.
ResultadoNO3 lerNitratoEstabilizado() {
  ResultadoNO3 r;
  r.nitrato = 0;
  r.estabilizou = false;
  r.valido = false;

  float tensaoAnterior = -999.0;
  int blocosEstaveis = 0;

  float tensoesEstaveis[NO3_MAX_BLOCOS];
  float adcsEstaveis[NO3_MAX_BLOCOS];
  int nEstaveis = 0;

  EstatisticaAnalogica ultimoBloco;
  ultimoBloco.valido = false;

  if (MODO_DETALHADO) {
    Serial.println("Acompanhando estabilizacao do nitrato...");
  }

  for (int bloco = 1; bloco <= NO3_MAX_BLOCOS; bloco++) {
    EstatisticaAnalogica estatBloco = lerAnalogicoRobusto(
      NITRATO_PIN,
      NO3_LEITURAS_POR_BLOCO,
      20
    );

    if (!estatBloco.valido) continue;

    ultimoBloco = estatBloco;

    if (MODO_DETALHADO) {
      Serial.print("  Bloco ");
      Serial.print(bloco);
      Serial.print(" | ADC=");
      Serial.print(estatBloco.mediaADC, 2);
      Serial.print(" | V=");
      Serial.print(estatBloco.mediaTensao, 6);
    }

    if (tensaoAnterior > 0) {
      float dV = estatBloco.mediaTensao - tensaoAnterior;

      if (MODO_DETALHADO) {
        Serial.print(" | dV=");
        Serial.print(dV, 6);
      }

      if (fabs(dV) <= NO3_LIMIAR_ESTAB_V) {
        blocosEstaveis++;

        if (nEstaveis < NO3_MAX_BLOCOS) {
          tensoesEstaveis[nEstaveis] = estatBloco.mediaTensao;
          adcsEstaveis[nEstaveis] = estatBloco.mediaADC;
          nEstaveis++;
        }

        if (MODO_DETALHADO) {
          Serial.print(" | estavel (");
          Serial.print(blocosEstaveis);
          Serial.print("/");
          Serial.print(NO3_BLOCOS_ESTAVEIS_NECESSARIOS);
          Serial.print(")");
        }
      } else {
        blocosEstaveis = 0;
        nEstaveis = 0;

        if (MODO_DETALHADO) {
          Serial.print(" | instavel");
        }
      }
    }

    if (MODO_DETALHADO) Serial.println();

    tensaoAnterior = estatBloco.mediaTensao;
    delay(NO3_DELAY_ENTRE_BLOCOS_MS);

    if (blocosEstaveis >= NO3_BLOCOS_ESTAVEIS_NECESSARIOS && nEstaveis > 0) {
      float mediaADC = calcularMedia(adcsEstaveis, nEstaveis);
      float mediaV = calcularMedia(tensoesEstaveis, nEstaveis);
      float desvioADC = calcularDesvioPadrao(adcsEstaveis, nEstaveis, mediaADC);
      float desvioV = calcularDesvioPadrao(tensoesEstaveis, nEstaveis, mediaV);

      r.estat.mediaADC = mediaADC;
      r.estat.desvioADC = desvioADC;
      r.estat.mediaTensao = mediaV;
      r.estat.desvioTensao = desvioV;
      r.estat.nUsado = nEstaveis;
      r.estat.valido = true;
      r.estabilizou = true;
      break;
    }
  }

  if (!r.estabilizou) {
    r.estat = ultimoBloco;
  }

  if (!r.estat.valido) return r;

  // Equação inversa da curva de calibração do ISE.
  // Como a calibração foi ajustada em log10, retorna-se à concentração
  // aplicando potência de base 10.
  r.nitrato = pow(10.0, (r.estat.mediaTensao - NO3_B) / NO3_A);

  if (isnan(r.nitrato) || isinf(r.nitrato) || r.nitrato < 0.0) return r;

  r.valido = true;
  return r;
}

// ============================================================
// MENU INTERATIVO
// ============================================================

// Imprime o menu de comandos disponíveis no Monitor Serial.
void imprimirMenu() {
  Serial.println("====================================================");
  Serial.println("MENU SERIAL");
  Serial.println("====================================================");
  Serial.println("[d] Alternar modo detalhado/resumido");
  Serial.println("[c] Ligar/desligar saida CSV");
  Serial.println("[p] Pausar/retomar medicoes");
  Serial.println("[1] Fazer uma unica leitura");
  Serial.println("[m] Mostrar configuracao atual");
  Serial.println("[h] Mostrar menu novamente");
  Serial.println("====================================================");
  Serial.println();
}

void imprimirConfiguracaoAtual() {
  Serial.println("Configuracao atual:");
  Serial.print("- Modo detalhado   : ");
  Serial.println(MODO_DETALHADO ? "SIM" : "NAO");
  Serial.print("- Saida CSV        : ");
  Serial.println(SAIDA_CSV ? "SIM" : "NAO");
  Serial.print("- Medicao continua : ");
  Serial.println(MEDICAO_CONTINUA ? "SIM" : "NAO");
  Serial.print("- Sistema pausado  : ");
  Serial.println(SISTEMA_PAUSADO ? "SIM" : "NAO");
  Serial.println();
}

// Lê os caracteres digitados no Monitor Serial e altera o estado
// do sistema em tempo real, sem necessidade de regravar o ESP32.
void processarComandoSerial() {
  while (Serial.available()) {
    char cmd = Serial.read();

    if (cmd == '\n' || cmd == '\r') continue;

    switch (cmd) {
      case 'd':
      case 'D':
        MODO_DETALHADO = !MODO_DETALHADO;
        Serial.print("Modo detalhado agora: ");
        Serial.println(MODO_DETALHADO ? "SIM" : "NAO");
        break;

      case 'c':
      case 'C':
        SAIDA_CSV = !SAIDA_CSV;
        Serial.print("Saida CSV agora: ");
        Serial.println(SAIDA_CSV ? "SIM" : "NAO");
        if (SAIDA_CSV) {
          Serial.println("CSV_HEADER:");
          Serial.println("ciclo,tempo_s,temperatura_C,pH,EC25_uScm,TDS_mgL,NO3_mgL");
        }
        break;

      case 'p':
      case 'P':
        SISTEMA_PAUSADO = !SISTEMA_PAUSADO;
        Serial.print("Sistema pausado: ");
        Serial.println(SISTEMA_PAUSADO ? "SIM" : "NAO");
        break;

      case '1':
        MEDICAO_CONTINUA = false;
        SISTEMA_PAUSADO = false;
        Serial.println("O sistema fara uma unica leitura no proximo ciclo.");
        break;

      case 'm':
      case 'M':
        imprimirConfiguracaoAtual();
        break;

      case 'h':
      case 'H':
        imprimirMenu();
        break;

      default:
        Serial.print("Comando nao reconhecido: ");
        Serial.println(cmd);
        Serial.println("Digite 'h' para ver o menu.");
        break;
    }

    Serial.println();
  }
}

// ============================================================
// IMPRESSÃO
// ============================================================

void imprimirCabecalho() {
  Serial.println("====================================================");
  Serial.println("       Monitoramento de Qualidade da Agua");
  Serial.println("====================================================");
  Serial.println("Versao com menu interativo no Monitor Serial");
  Serial.println("Parametros: Temperatura, pH, EC25, TDS e Nitrato");
  Serial.println("====================================================");
  Serial.println();
}

// Impressão expandida: mostra resultados finais e variáveis
// intermediárias úteis para diagnóstico, validação e calibração.
void imprimirResultadosDetalhados(float temperatura, ResultadoPH ph, ResultadoEC ec, ResultadoNO3 no3) {
  Serial.println("----------------------------------------------------");
  Serial.print("Ciclo                           : ");
  Serial.println(contadorCiclos);
  Serial.print("Tempo desde inicio              : ");
  Serial.print((millis() - tempoInicio) / 1000.0, 1);
  Serial.println(" s");
  Serial.println();

  Serial.print("Temperatura media               : ");
  if (temperatura == DEVICE_DISCONNECTED_C) Serial.println("ERRO");
  else {
    Serial.print(temperatura, 2);
    Serial.println(" C");
  }
  Serial.println();

  Serial.print("pH - ADC medio                  : ");
  Serial.println(ph.estat.mediaADC, 2);
  Serial.print("pH - ADC desvio                 : ");
  Serial.println(ph.estat.desvioADC, 2);
  Serial.print("pH - Tensao media               : ");
  Serial.print(ph.estat.mediaTensao, 6);
  Serial.println(" V");
  Serial.print("pH - Tensao desvio              : ");
  Serial.print(ph.estat.desvioTensao, 6);
  Serial.println(" V");
  Serial.print("pH - Slope compensado           : ");
  Serial.println(ph.slopeComp, 6);
  Serial.print("pH medio                        : ");
  if (ph.valido) Serial.println(ph.pH, 2);
  else Serial.println("ERRO");
  Serial.println();

  Serial.print("EC - ADC medio                  : ");
  Serial.println(ec.estat.mediaADC, 2);
  Serial.print("EC - ADC desvio                 : ");
  Serial.println(ec.estat.desvioADC, 2);
  Serial.print("EC - Tensao media               : ");
  Serial.print(ec.estat.mediaTensao, 6);
  Serial.println(" V");
  Serial.print("EC - Tensao desvio              : ");
  Serial.print(ec.estat.desvioTensao, 6);
  Serial.println(" V");
  Serial.print("Condutividade compensada (EC25) : ");
  if (ec.valido) {
    Serial.print(ec.ec25, 2);
    Serial.println(" uS/cm");
  } else Serial.println("ERRO");
  Serial.print("TDS medio                       : ");
  if (ec.valido) {
    Serial.print(ec.tds, 2);
    Serial.println(" mg/L");
  } else Serial.println("ERRO");
  Serial.println();

  Serial.print("NO3 - ADC medio                 : ");
  Serial.println(no3.estat.mediaADC, 2);
  Serial.print("NO3 - ADC desvio                : ");
  Serial.println(no3.estat.desvioADC, 2);
  Serial.print("NO3 - Tensao media              : ");
  Serial.print(no3.estat.mediaTensao, 6);
  Serial.println(" V");
  Serial.print("NO3 - Tensao desvio             : ");
  Serial.print(no3.estat.desvioTensao, 6);
  Serial.println(" V");
  Serial.print("NO3 - Estabilizacao             : ");
  Serial.println(no3.estabilizou ? "CONFIRMADA" : "NAO CONFIRMADA");
  Serial.print("Nitrato estimado                : ");
  if (no3.valido) {
    Serial.print(no3.nitrato, 2);
    Serial.println(" mg/L");
  } else Serial.println("ERRO");

  Serial.println("----------------------------------------------------");
  Serial.println();
}

// Impressão compacta: mostra apenas os valores principais.
void imprimirResultadosResumidos(float temperatura, ResultadoPH ph, ResultadoEC ec, ResultadoNO3 no3) {
  Serial.println("----------------------------------------------------");
  Serial.print("Ciclo ");
  Serial.print(contadorCiclos);
  Serial.print(" | t=");
  Serial.print((millis() - tempoInicio) / 1000.0, 1);
  Serial.println(" s");

  Serial.print("Temperatura : ");
  if (temperatura == DEVICE_DISCONNECTED_C) Serial.println("ERRO");
  else {
    Serial.print(temperatura, 2);
    Serial.println(" C");
  }

  Serial.print("pH          : ");
  if (ph.valido) Serial.println(ph.pH, 2);
  else Serial.println("ERRO");

  Serial.print("EC25        : ");
  if (ec.valido) {
    Serial.print(ec.ec25, 2);
    Serial.println(" uS/cm");
  } else Serial.println("ERRO");

  Serial.print("TDS         : ");
  if (ec.valido) {
    Serial.print(ec.tds, 2);
    Serial.println(" mg/L");
  } else Serial.println("ERRO");

  Serial.print("Nitrato     : ");
  if (no3.valido) {
    Serial.print(no3.nitrato, 2);
    Serial.println(" mg/L");
  } else Serial.println("ERRO");

  Serial.println("----------------------------------------------------");
  Serial.println();
}

// Saída em linha única CSV.
// Útil para registrar os dados no computador, importar em Excel,
// LibreOffice Calc, Python ou outro software de análise.
void imprimirCSV(float temperatura, ResultadoPH ph, ResultadoEC ec, ResultadoNO3 no3) {
  Serial.print(contadorCiclos);
  Serial.print(",");
  Serial.print((millis() - tempoInicio) / 1000.0, 1);
  Serial.print(",");

  if (temperatura == DEVICE_DISCONNECTED_C) Serial.print("nan");
  else Serial.print(temperatura, 2);
  Serial.print(",");

  if (ph.valido) Serial.print(ph.pH, 2);
  else Serial.print("nan");
  Serial.print(",");

  if (ec.valido) Serial.print(ec.ec25, 2);
  else Serial.print("nan");
  Serial.print(",");

  if (ec.valido) Serial.print(ec.tds, 2);
  else Serial.print("nan");
  Serial.print(",");

  if (no3.valido) Serial.print(no3.nitrato, 2);
  else Serial.print("nan");
  Serial.println();
}

// Gera alertas simples de plausibilidade.
// Não substitui validação analítica nem limites legais, mas ajuda a
// identificar leituras suspeitas durante ensaio de bancada ou campo.
void imprimirAlertas(float temperatura, ResultadoPH ph, ResultadoEC ec, ResultadoNO3 no3) {
  bool houveAlerta = false;

  if (temperatura == DEVICE_DISCONNECTED_C) {
    Serial.println("[ALERTA] Falha na leitura de temperatura.");
    houveAlerta = true;
  }

  if (ph.valido && (ph.pH < 5.0 || ph.pH > 9.0)) {
    Serial.println("[ALERTA] pH fora da faixa usual.");
    houveAlerta = true;
  }

  if (ec.valido && ec.ec25 > 2000.0) {
    Serial.println("[ALERTA] EC25 elevada.");
    houveAlerta = true;
  }

  if (no3.valido && no3.nitrato > 100.0) {
    Serial.println("[ALERTA] Nitrato elevado.");
    houveAlerta = true;
  }

  if (no3.valido && !no3.estabilizou) {
    Serial.println("[ALERTA] Nitrato sem estabilizacao confirmada.");
    houveAlerta = true;
  }

  if (!houveAlerta && MODO_DETALHADO) {
    Serial.println("Sem alertas de plausibilidade neste ciclo.");
  }

  if (MODO_DETALHADO) Serial.println();
}

// ============================================================
// EXECUÇÃO DE UM CICLO DE LEITURA
// ============================================================
// Executa um ciclo completo de aquisição.
// A ordem adotada é importante:
// 1) temperatura
// 2) pH
// 3) EC/TDS
// 4) nitrato
// Depois, o sistema imprime resultados, alertas e CSV opcional.
void executarCicloLeitura() {
  contadorCiclos++;

  if (MODO_DETALHADO) {
    Serial.println(">>> Iniciando novo ciclo de leitura...");
  }

  float temperatura = lerMediaTemperatura();
  if (MODO_DETALHADO) Serial.println("Temperatura concluida.");

  ResultadoPH ph = lerPHCompensado(temperatura);
  if (MODO_DETALHADO) Serial.println("pH concluido.");

  ResultadoEC ec = lerEC(temperatura);
  if (MODO_DETALHADO) Serial.println("Condutividade e TDS concluidos.");

  ResultadoNO3 no3 = lerNitratoEstabilizado();
  if (MODO_DETALHADO) Serial.println("Nitrato concluido.");

  if (MODO_DETALHADO) {
    imprimirResultadosDetalhados(temperatura, ph, ec, no3);
  } else {
    imprimirResultadosResumidos(temperatura, ph, ec, no3);
  }

  imprimirAlertas(temperatura, ph, ec, no3);

  if (SAIDA_CSV) {
    imprimirCSV(temperatura, ph, ec, no3);
  }

  if (MODO_DETALHADO) {
    Serial.println(">>> Ciclo finalizado.");
    Serial.println();
  }
}

// ============================================================
// SETUP
// ============================================================
// Setup: inicializa comunicação serial, ADC e sensor de temperatura,
// registra o instante inicial do programa e mostra a interface.
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1200);

  analogReadResolution(12);
  sensorTemp.begin();

  tempoInicio = millis();

  imprimirCabecalho();

  int nDisp = sensorTemp.getDeviceCount();
  Serial.print("Sensores DS18B20 encontrados     : ");
  Serial.println(nDisp);

  if (nDisp == 0) {
    Serial.println("[AVISO] Nenhum sensor DS18B20 detectado.");
    Serial.println("[AVISO] pH e EC usarao temperatura de referencia.");
  }

  Serial.println();
  imprimirConfiguracaoAtual();
  imprimirMenu();
}

// ============================================================
// LOOP
// ============================================================
// Loop principal:
// - processa comandos do usuário;
// - respeita o estado de pausa;
// - executa um ciclo de leitura;
// - se a opção era leitura única, pausa após o ciclo;
// - aguarda até o próximo ciclo, mantendo o menu responsivo.
void loop() {
  processarComandoSerial();

  if (SISTEMA_PAUSADO) {
    delay(200);
    return;
  }

  executarCicloLeitura();

  // Se foi solicitado apenas um ciclo, pausa após executá-lo
  if (!MEDICAO_CONTINUA) {
    SISTEMA_PAUSADO = true;
    MEDICAO_CONTINUA = true;
    Serial.println("Leitura unica concluida. Sistema pausado.");
    Serial.println("Use [p] para retomar medicoes continuas.");
    Serial.println();
  }

  unsigned long t0 = millis();
  while (millis() - t0 < DELAY_ENTRE_CICLOS_MS) {
    processarComandoSerial();

    if (SISTEMA_PAUSADO) break;

    delay(50);
  }
}
