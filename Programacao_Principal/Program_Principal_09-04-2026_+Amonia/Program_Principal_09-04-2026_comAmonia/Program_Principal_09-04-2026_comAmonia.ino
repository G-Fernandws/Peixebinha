#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

// ================= PINOS =================
#define TEMP_PIN 4
#define PH_PIN   35
#define EC_PIN   34
#define NO3_PIN  32
#define NH3_PIN  33   // << AGORA AQUI

// ================= CONFIG =================
const float VREF = 3.3;
const int ADC_MAX = 4095;

const int N_AMOSTRAS = 100;

// ================= TEMPERATURA =================
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);

// ================= pH =================
const float PH_SLOPE = -9.0909;
const float PH_INTERCEPT = 21.86;

// ================= CONDUTIVIDADE =================
const float ALPHA = 0.02;
const float TEMP_REF = 25.0;
const float FATOR_TDS = 0.5;

// ================= NITRATO =================
const float NO3_A = -0.120835;
const float NO3_B =  2.046916;

// ================= AMÔNIA =================
const float NH3_A = 0.09660110;
const float NH3_B = 1.70914745;

// ------------------------------------------------
float lerADC(int pino) {
  long soma = 0;
  for (int i = 0; i < N_AMOSTRAS; i++) {
    soma += analogRead(pino);
    delay(10);
  }
  return soma / (float)N_AMOSTRAS;
}

float adcToV(float adc) {
  return (adc / ADC_MAX) * VREF;
}

float calcularPH(float v) {
  return PH_SLOPE * v + PH_INTERCEPT;
}

float calcularNH3(float v) {
  return pow(10.0, (v - NH3_B) / NH3_A);
}

float calcularNO3(float v) {
  return pow(10.0, (v - NO3_B) / NO3_A);
}

// =================================================
void setup() {
  Serial.begin(115200);
  delay(1500);

  analogReadResolution(12);
  sensors.begin();

  Serial.println("====================================");
  Serial.println(" SISTEMA MULTIPARAMETROS ");
  Serial.println("====================================");
}

// =================================================
void loop() {

  // -------- TEMPERATURA --------
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  // -------- LEITURAS --------
  float adcPH  = lerADC(PH_PIN);
  float adcEC  = lerADC(EC_PIN);
  float adcNO3 = lerADC(NO3_PIN);
  float adcNH3 = lerADC(NH3_PIN);

  // -------- TENSÕES --------
  float vPH  = adcToV(adcPH);
  float vEC  = adcToV(adcEC);
  float vNO3 = adcToV(adcNO3);
  float vNH3 = adcToV(adcNH3);

  // -------- CÁLCULOS --------
  float ph  = calcularPH(vPH);
  float nh3 = calcularNH3(vNH3);
  float no3 = calcularNO3(vNO3);

  // Condutividade
  float ecBruta = (vEC + 0.0572) / 0.0013;
  float fatorComp = 1.0 + ALPHA * (temp - TEMP_REF);
  float ec25 = ecBruta / fatorComp;
  float tds = ec25 * FATOR_TDS;

  // -------- SAÍDA --------
  Serial.println("------------------------------------");

  Serial.print("Temperatura (C)      : ");
  Serial.println(temp, 2);

  Serial.print("pH                   : ");
  Serial.println(ph, 2);

  Serial.print("Condutividade (uS/cm): ");
  Serial.println(ec25, 2);

  Serial.print("TDS (mg/L)           : ");
  Serial.println(tds, 2);

  Serial.print("Nitrato (mg/L)       : ");
  Serial.println(no3, 4);

  Serial.print("Amonia (mg/L)        : ");
  Serial.println(nh3, 4);

  Serial.println("------------------------------------");
  Serial.println();

  delay(3000);
}