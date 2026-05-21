/*
  ==========================================================
  LEITURA DE NITRATO (NO3-) COM ISE + ESP32
  ==========================================================

  CONFIGURAÇÃO UTILIZADA:
  - Sensor ISE de nitrato
  - Módulo HW-828
  - ESP32
  - GPIO 33

  CALIBRAÇÃO:
  Solução padrão preparada:
    1 mL de solução 1000 ppm
    diluída em 100 mL

  Concentração final:
    10 ppm (10 mg/L)

  Média experimental obtida:
    E0 = 2902.8 mV

  Slope experimental:
    59 mV/década

  ==========================================================
*/

const int pinoNitrato = 33;

// ===== CALIBRAÇÃO =====

// potencial da solução padrão
float E0 = 2902.8; // mV

// concentração da solução padrão
float C0 = 10.0; // ppm ou mg/L

// slope experimental
float slope = 59.0; // mV/década

// ======================

void setup() {

  Serial.begin(115200);

  // resolução ADC ESP32
  analogReadResolution(12);

  // faixa até ~3.3V
  analogSetPinAttenuation(pinoNitrato, ADC_11db);

  Serial.println("================================");
  Serial.println("ISE NITRATO - ESP32");
  Serial.println("================================");
}

void loop() {

  long somaADC = 0;

  // média de leituras para reduzir ruído
  for (int i = 0; i < 20; i++) {

    somaADC += analogRead(pinoNitrato);

    delay(10);
  }

  float adcMedio = somaADC / 20.0;

  // conversão ADC -> tensão
  float tensao = (adcMedio / 4095.0) * 3.3;

  // conversão para mV
  float mV = tensao * 1000.0;

  /*
    EQUAÇÃO DE NERNST

    E = E0 + S*log(C/C0)

    rearranjando:

    C = C0 * 10^((E-E0)/S)
  */

  float nitrato = C0 * pow(10, ((mV - E0) / slope));

  // ===== SAÍDA SERIAL =====

  Serial.print("ADC: ");
  Serial.print(adcMedio);

  Serial.print(" | Tensao: ");
  Serial.print(mV, 2);
  Serial.print(" mV");

  Serial.print(" | Nitrato: ");
  Serial.print(nitrato, 3);
  Serial.println(" mg/L");

  delay(1000);
}