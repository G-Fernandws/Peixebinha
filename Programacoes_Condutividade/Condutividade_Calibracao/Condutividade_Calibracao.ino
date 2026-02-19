// ============================================
// SENSOR DE CONDUTIVIDADE - ESP32 DOIT DEVKIT V1
// GPIO 32 (ADC1_CH4)
// ============================================

const int sensorPin = 32;      // GPIO 32
const float Vref = 3.3;        // Tensão de referência do ESP32
const int N = 5;               // Número de soluções padrão

float V_values[N];             // Tensões medidas
float EC_values[N];            // Condutividades conhecidas

// Coeficientes do polinômio: EC = a·V² + b·V + c
float a = 0, b = 0, c = 0;

bool calibrated = false;

void setup()
{
  Serial.begin(115200);

  // Configuração recomendada para ADC do ESP32
  analogReadResolution(12);                   // 0–4095
  analogSetPinAttenuation(sensorPin, ADC_11db); // Permite leitura até ~3.3V

  delay(1000);

  Serial.println("=== CALIBRACAO DE CONDUTIVIDADE ===");
  Serial.println("Unidades: uS/cm");
}

void loop()
{
  if (!calibrated)
  {
    realizarCalibracao();
    calcularCoeficientes();
    calibrated = true;

    Serial.println("\n=== CALIBRACAO CONCLUIDA ===");
    Serial.print("a = "); Serial.println(a, 10);
    Serial.print("b = "); Serial.println(b, 10);
    Serial.print("c = "); Serial.println(c, 10);
    Serial.println("Iniciando medicoes...");
    delay(2000);
  }

  // ==========================
  // MEDIÇÃO NORMAL
  // ==========================

  int adc = analogRead(sensorPin);

  float V = adc * (Vref / 4095.0);   // Converte ADC em tensão
  float EC = a * V * V + b * V + c;  // Aplica polinômio

  Serial.print("ADC = ");
  Serial.print(adc);

  Serial.print(" | V = ");
  Serial.print(V, 3);
  Serial.print(" V");

  Serial.print(" | EC = ");
  Serial.print(EC, 1);
  Serial.println(" uS/cm");

  delay(500);
}

// =====================================================
// CALIBRAÇÃO
// =====================================================

void realizarCalibracao()
{
  for (int i = 0; i < N; i++)
  {
    Serial.println("\nDigite o valor da solucao padrao (uS/cm):");

    while (Serial.available() == 0) {}
    EC_values[i] = Serial.parseFloat();

    Serial.print("Valor recebido: ");
    Serial.println(EC_values[i]);

    Serial.println("Coloque o sensor na solucao e pressione ENTER...");
    while (Serial.available() == 0) {}
    Serial.read();

    float somaV = 0;

    for (int j = 0; j < 30; j++)
    {
      int adc = analogRead(sensorPin);
      float V = adc * (Vref / 4095.0);
      somaV += V;
      delay(50);
    }

    V_values[i] = somaV / 30.0;

    Serial.print("Tensao media medida: ");
    Serial.print(V_values[i], 4);
    Serial.println(" V");
  }
}

// =====================================================
// AJUSTE POLINOMIAL 2º GRAU (MÍNIMOS QUADRADOS)
// =====================================================

void calcularCoeficientes()
{
  float Sx = 0, Sx2 = 0, Sx3 = 0, Sx4 = 0;
  float Sy = 0, Sxy = 0, Sx2y = 0;

  for (int i = 0; i < N; i++)
  {
    float x = V_values[i];
    float y = EC_values[i];

    Sx += x;
    Sx2 += x * x;
    Sx3 += x * x * x;
    Sx4 += x * x * x * x;

    Sy += y;
    Sxy += x * y;
    Sx2y += x * x * y;
  }

  float D  = N * (Sx2 * Sx4 - Sx3 * Sx3)
           - Sx * (Sx * Sx4 - Sx2 * Sx3)
           + Sx2 * (Sx * Sx3 - Sx2 * Sx2);

  float Da = Sy * (Sx2 * Sx4 - Sx3 * Sx3)
           - Sx * (Sxy * Sx4 - Sx3 * Sx2y)
           + Sx2 * (Sxy * Sx3 - Sx2 * Sx2y);

  float Db = N * (Sxy * Sx4 - Sx3 * Sx2y)
           - Sy * (Sx * Sx4 - Sx2 * Sx3)
           + Sx2 * (Sx * Sx2y - Sxy * Sx2);

  float Dc = N * (Sx2 * Sx2y - Sxy * Sx3)
           - Sx * (Sx * Sx2y - Sxy * Sx2)
           + Sy * (Sx * Sx3 - Sx2 * Sx2);

  a = Da / D;
  b = Db / D;
  c = Dc / D;
}
