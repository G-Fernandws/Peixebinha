/*
                    OBSERVAÇÃO 
  ==========================================================
  LIGAÇÃO ELÉTRICA (COM DIVISOR DE TENSÃO)
  ==========================================================
  Fórmula do divisor:
  Vout = Vin * (R2 / (R1 + R2))

  Com:
  R1 = 10kΩ
  R2 = 20kΩ

  Vout ≈ 0.66 * Vin
*/

const int turbidityPin = 35;   // GPIO 35 (ADC1)
const float Vref = 3.3;        // Tensão máxima do ESP32

// Parâmetros do divisor de tensão
const float R1 = 10000.0;      // 10kΩ (entre sensor e ESP32)
const float R2 = 20000.0;      // 20kΩ (entre ESP32 e GND)

void setup()
{
  Serial.begin(9600);
  delay(1000);

  Serial.println("==================================");
  Serial.println("  LEITURA SENSOR DE TURBIDEZ");
  Serial.println("  ESP32 + DIVISOR DE TENSAO");
  Serial.println("==================================");
}


void loop()
{

  /*
    ESP32 possui ADC de 12 bits:
    Faixa: 0 a 4095
  */
  int adcValue = analogRead(turbidityPin);

  /*
    Conversão real após o ESP32: essa é a tensão 
    REAL que chega ao ESP32.
  */
  float Vout = adcValue * (Vref / 4095.0);

  /*
    Recuperar a tensão real, para estimar qual tensão
    o sensor está realmente lendo
    Fórmula do divisor:
    Vout = Vin * (R2 / (R1 + R2))

    Vin = Vout * ((R1 + R2) / R2)
  */
  float Vin_sensor = Vout * ((R1 + R2) / R2);

  /*
   Filtragem simples pois o sensor é pouco sensível, logo, pequenas
   oscilações podem ser importantes.
  */
  float soma = 0;

  for (int i = 0; i < 50; i++) {
    soma += analogRead(turbidityPin);
    delay(10);
  }

  float adcMedio = soma / 50.0;
  float VoutMedio = adcMedio * (Vref / 4095.0);
  float VinMedio = VoutMedio * ((R1 + R2) / R2);

  /*
    Mostra tudo o que vamos precisar para analisar
    as leituras e fazer uma possível calibração.
  */
  Serial.print("ADC bruto: ");
  Serial.print(adcValue);

  Serial.print(" | ADC medio: ");
  Serial.print(adcMedio, 1);

  Serial.print(" | Vout ESP32: ");
  Serial.print(VoutMedio, 3);
  Serial.print(" V");

  Serial.print(" | Vin sensor (estimado): ");
  Serial.print(VinMedio, 3);
  Serial.println(" V");

  delay(1000);
}
