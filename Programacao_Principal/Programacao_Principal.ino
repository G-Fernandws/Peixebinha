/********************* BIBLIOTECAS *************************/

// --- Temperatura ---
#include <OneWire.h>
#include <DallasTemperature.h>

// --- Matemática ---
#include <math.h>

/********************* DEFINIÇÃO DOS PINOS *****************/

// Turbidez
#define PIN_TURBIDEZ A0

// pH
#define PIN_PH A1

// Condutividade
#define PIN_EC A2

// Temperatura DS18B20
#define PIN_DS18B20 2

/********************* OBJETOS TEMPERATURA *****************/

OneWire oneWire(PIN_DS18B20);
DallasTemperature sensortemp(&oneWire);

/********************* VARIÁVEIS TURBIDEZ *****************/

int i;
float voltagemTurbidez;
float NTU;

/********************* VARIÁVEIS TEMPERATURA ***************/

int ndispositivos = 0;
float grausC;

/********************* VARIÁVEIS PH ************************/

float calibracao_ph7 = 2.57;
float calibracao_ph4 = 2.86;
float calibracao_ph10 = 0.0;

#define UTILIZAR_PH_10 false

float m;
float b;
int bufPH[10];

/********************* VARIÁVEIS CONDUTIVIDADE *************/

const float Vref = 5.0;
const float Rfix = 10000.0;

// Modelo matemático calibrado
const double A = -2.564676647e11;
const double B =  3.870610649e07;
const double C = -1.846756580e+01;

// Média móvel
const int window = 10;
double bufferEC[window];
int indexEC = 0;
bool filled = false;

/***********************************************************
                        SETUP
***********************************************************/
void setup() {

  Serial.begin(9600);
  Serial.println("=== Sistema Integrado de Monitoramento ===");

  /******** Temperatura ********/
  sensortemp.begin();

  Serial.print("Sensores DS18B20 encontrados: ");
  ndispositivos = sensortemp.getDeviceCount();
  Serial.println(ndispositivos);

  /******** Configuração pH ********/

  if (UTILIZAR_PH_10) {
    m = (7.0 - 10.0) / (calibracao_ph7 - calibracao_ph10);
    b = 10.0 - m * calibracao_ph10;
  }
  else {
    m = (4.0 - 7.0) / (calibracao_ph4 - calibracao_ph7);
    b = 7.0 - m * calibracao_ph7;
  }

  /******** Inicialização buffer EC ********/
  for (int i = 0; i < window; i++) {
    bufferEC[i] = 0;
  }
}

/***********************************************************
                        LOOP PRINCIPAL
***********************************************************/
void loop() {

  Serial.println("\n===== NOVA LEITURA =====");

  leituraTurbidez();
  leituraTemperatura();
  leituraPH();
  leituraCondutividade();

  delay(2000);
}

/***********************************************************
                    FUNÇÃO TURBIDEZ
***********************************************************/
void leituraTurbidez() {

  voltagemTurbidez = 0;

  // Média de 50 leituras
  for (i = 0; i < 50; i++) {
    voltagemTurbidez += ((float)analogRead(PIN_TURBIDEZ) / 1023.0) * 5.0;
  }

  voltagemTurbidez /= 50;
  voltagemTurbidez = ArredondarPara(voltagemTurbidez, 3);

  // Conversão para NTU
  if (voltagemTurbidez < 2.5) {
    NTU = 3000;
  }
  else if (voltagemTurbidez > 3.78) {
    NTU = 0;
  }
  else {
    NTU = 499.059 * voltagemTurbidez * voltagemTurbidez
          - 5161.337 * voltagemTurbidez + 12367.676;
  }

  Serial.println("\n--- TURBIDEZ ---");
  Serial.print("Tensão: ");
  Serial.print(voltagemTurbidez);
  Serial.print(" V | NTU: ");
  Serial.print(NTU);
  Serial.print(" | ");
  Serial.println(getTurbidityLevel(voltagemTurbidez));
}

/***********************************************************
                    FUNÇÃO TEMPERATURA
***********************************************************/
void leituraTemperatura() {

  sensortemp.requestTemperatures();

  Serial.println("\n--- TEMPERATURA ---");

  for (int i = 0; i < ndispositivos; i++) {

    grausC = sensortemp.getTempCByIndex(i);

    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(grausC);
    Serial.println(" °C");
  }
}

/***********************************************************
                    FUNÇÃO PH
***********************************************************/
void leituraPH() {

  // Coleta 10 amostras
  for (int i = 0; i < 10; i++) {
    bufPH[i] = analogRead(PIN_PH);
    delay(10);
  }

  // Ordena amostras
  for (int i = 0; i < 9; i++) {
    for (int j = i + 1; j < 10; j++) {
      if (bufPH[i] > bufPH[j]) {
        int temp = bufPH[i];
        bufPH[i] = bufPH[j];
        bufPH[j] = temp;
      }
    }
  }

  // Média filtrada
  int valorMedio = 0;
  for (int i = 2; i < 8; i++) {
    valorMedio += bufPH[i];
  }

  float tensao = (valorMedio * 5.0) / 1024.0 / 6;
  float ph = m * tensao + b;

  Serial.println("\n--- PH ---");
  Serial.print("Tensão: ");
  Serial.print(tensao);
  Serial.print(" V | pH: ");
  Serial.println(ph);
}

/***********************************************************
                FUNÇÃO CONDUTIVIDADE
***********************************************************/
void leituraCondutividade() {

  int adc = analogRead(PIN_EC);
  float V = adc * (Vref / 1023.0);

  if (V <= 0.01) return;

  double R = Rfix * ((Vref / V) - 1.0);
  if (R <= 0) return;

  double u = 1.0 / R;
  double EC = A * (u * u) + B * u + C;

  if (EC < 0) EC = 0;

  // Média móvel
  bufferEC[indexEC] = EC;
  indexEC++;

  if (indexEC >= window) {
    indexEC = 0;
    filled = true;
  }

  double soma = 0;
  int count = filled ? window : indexEC;

  for (int i = 0; i < count; i++) {
    soma += bufferEC[i];
  }

  double EC_filtrada = soma / (count > 0 ? count : 1);

  Serial.println("\n--- CONDUTIVIDADE ---");
  Serial.print("Resistência: ");
  Serial.print(R);
  Serial.print(" ohms | EC: ");
  Serial.print(EC_filtrada);
  Serial.println(" uS/cm");
}

/***********************************************************
                FUNÇÕES AUXILIARES
***********************************************************/
float ArredondarPara(float valor, int casas) {
  float mult = powf(10.0f, casas);
  return roundf(valor * mult) / mult;
}

String getTurbidityLevel(float voltage) {
  if (voltage >= 2.96) return "Muito Limpa";
  if (voltage >= 2.64) return "Limpa";
  if (voltage >= 1.84) return "Moderada";
  return "Turva";
}
