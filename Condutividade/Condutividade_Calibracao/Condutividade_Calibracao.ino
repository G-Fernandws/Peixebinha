const int sensorPin = A0;
const float Vref = 5.0;
const float Rfix = 10000.0;

// Quantidade de padrões de calibração
const int N = 5;

// Arrays com calibração
float R_values[N];      // Resistências lidas
float EC_values[N];     // Condutividades conhecidas (µS/cm)

// Coeficientes da curva EC = aR² + bR + c
float a = 0, b = 0, c = 0;

bool calibrated = false;

void setup() {
  Serial.begin(9600);
  delay(1000);

  Serial.println("=== Calibracao de Condutividade ===");
  Serial.println("Coloque a solucao padrao quando solicitado.");
  Serial.println("As unidades sao em microsiemens (uS/cm).");
}

void loop() {

  if (!calibrated) {
    realizarCalibracao();
    calcularCoeficientes();
    calibrated = true;

    Serial.println("\n=== Calibracao concluida ===");
    Serial.print("a = "); Serial.println(a, 10);
    Serial.print("b = "); Serial.println(b, 10);
    Serial.print("c = "); Serial.println(c, 10);
    Serial.println("Agora medindo condutividade...");
    delay(2000);
  }

  // ------------------------
  // Medição usando curva
  // ------------------------

  int adc = analogRead(sensorPin);
  float V = adc * (Vref / 1023.0);
  float R = Rfix * ((Vref / V) - 1.0);

  // Usa a curva polinomial
  float EC = a * R * R + b * R + c;

  Serial.print("R = ");
  Serial.print(R, 1);
  Serial.print(" ohms | EC = ");
  Serial.print(EC, 1);
  Serial.println(" uS/cm");

  delay(500);
}


// ===========================================================
// FUNÇÃO DE CALIBRAÇÃO — Lê resistência para cada solução padrão
// ===========================================================
void realizarCalibracao() {
  for (int i = 0; i < N; i++) {

    Serial.println("\nDigite no monitor serial o valor da solucao padrao (uS/cm):");
    while (Serial.available() == 0) {}
    EC_values[i] = Serial.parseFloat();
    Serial.print("Valor recebido: ");
    Serial.println(EC_values[i]);

    Serial.println("Coloque o sensor na solucao e pressione ENTER...");
    while (Serial.available() == 0) {}
    Serial.read(); // limpa buffer

    // faz uma média da resistência
    float soma = 0;
    for (int j = 0; j < 30; j++) {
      int adc = analogRead(sensorPin);
      float V = adc * (Vref / 1023.0);
      float R = Rfix * ((Vref / V) - 1.0);
      soma += R;
      delay(50);
    }

    R_values[i] = soma / 30.0;

    Serial.print("Resistencia medida: ");
    Serial.println(R_values[i], 1);
  }
}


// ===========================================================
// FUNÇÃO: AJUSTE POLINOMIAL DE 2º GRAU
// Resolve os coeficientes a, b, c da curva EC = aR² + bR + c
// ===========================================================
void calcularCoeficientes() {

  // Matriz para soma dos termos
  float Sx = 0, Sx2 = 0, Sx3 = 0, Sx4 = 0;
  float Sy = 0, Sxy = 0, Sx2y = 0;

  for (int i = 0; i < N; i++) {
    float x = R_values[i];
    float y = EC_values[i];

    Sx += x;
    Sx2 += x * x;
    Sx3 += x * x * x;
    Sx4 += x * x * x * x;

    Sy += y;
    Sxy += x * y;
    Sx2y += x * x * y;
  }

  // Resolver o sistema linear
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
