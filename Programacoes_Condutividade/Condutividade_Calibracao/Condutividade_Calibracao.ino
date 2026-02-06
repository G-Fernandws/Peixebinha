//Atualizado para uso no ESP32
const int sensorPin = 32;
const float Vref = 3.3;

// Quantidade de padrões de calibração
const int N = 5;

// Vetores para armazenar os dados experimentais, usados para calibrar a curva
float V_values[N];   // Tensões medidas para cada soluçã
 /*Condutividades conhecidas (µS/cm) dessas 
 soluções, digitadas no monitor serial*/
float EC_values[N];    

// Coeficientes do polinômio: EC = a·V² + b·V + c
float a = 0, b = 0, c = 0;

bool calibrated = false; //para evitar repetição do processo

void setup() 
{
  Serial.begin(9600);
  delay(1000);

  Serial.println("=== Calibracao de Condutividade ===");
  Serial.println("Coloque a solucao padrao quando solicitado.");
  Serial.println("As unidades sao em microsiemens (uS/cm).");
}

/* - Primeiro executa a calibração (uma única vez)
   - Depois passa a medir continuamente*/
void loop() 
{

  if (!calibrated) 
  {
    realizarCalibracao();
    calcularCoeficientes();
    calibrated = true;

    Serial.println("\n=== Calibracao concluida ===");
    Serial.print("a = "); Serial.println(a, 10);
    Serial.print("b = "); Serial.println(b, 10);
    Serial.print("c = "); Serial.println(c, 10);
    Serial.println("Iniciando medicoes...");
    delay(2000);
  }

// Medição usando curva
 /* 1) Lê o valor do ADC (0–4095)
    2) Converte para tensão real (0–3.3V)
    3) Aplica a equação da calibração*/
  int adc = analogRead(sensorPin);
  float V = adc * (Vref / 4095.0); //converte o valor digital em tensão real
  float EC = a * V * V + b * V + c;  // Aplica o polinômio ajustado na calibração

  /* Usa a curva polinomial para ajustar melhor, 
  se fosse equação de reta não ia ser um ajuste adequado*/
  float EC = a * R * R + b * R + c;

 // Mostra os resultados
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
/*
  ============================================================
  FUNÇÃO: REALIZAR CALIBRAÇÃO
  ============================================================
  Para cada solução padrão:
  1) Digita a condutividade conhecida
  2) Coloca o sensor na solução
  3) O sistema mede a tensão média (30 leituras)
  4) Armazena o par:
       (Tensão medida, EC conhecida)
  Esses dados serão usados para ajustar a curva.
*/

void realizarCalibracao() 
{
  for (int i = 0; i < N; i++) 
  {

    Serial.println("\nDigite no monitor serial o valor da solucao padrao (uS/cm):");
    while (Serial.available() == 0) {} //recebe a EC conhecida
    EC_values[i] = Serial.parseFloat();
    Serial.print("Valor recebido: ");
    Serial.println(EC_values[i]);

    Serial.println("Coloque o sensor na solucao e pressione ENTER...");
    while (Serial.available() == 0) {}
    Serial.read(); // limpa buffer

    // faz uma média da resistência, 30 leituras
    float somaV = 0;
    for (int j = 0; j < 30; j++) 
    {
       int adc = analogRead(sensorPin);
      float V = adc * (Vref / 4095.0);
      somaV += V;
      delay(50);
    }

    // Tensão média medida nessa solução
    V_values[i] = somaV / 30.0;

    Serial.print("Tensao media medida: ");
    Serial.print(V_values[i], 4);
    Serial.println(" V");
  }
}
/*
  ============================================================
  FUNÇÃO: AJUSTE POLINOMIAL DE 2º GRAU
  ============================================================
  Curva a ser ajustada usando os N pontos coletados:
    EC = a·V² + b·V + c
  O código vai resolver o sistema por mínimos quadrados,
  montando as somas estatísticas e aplicando o método
  dos determinantes (Cramer).
*/
void calcularCoeficientes()
{

  // Matriz para soma dos termos, resolve por mínimos quadrados
  /*Sx = soma de R    | Sx2 = soma de R^2 | Sx3 = soma de R^3
    Sx4 = soma de R^4 | Sy =  soma de EC  | Sxy = soma de R*EC 
    Sx2y = soma de R^2 *EC */
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

  // Resolver o sistema linear com 3 icógnitas por meio de determinantes
  //basicamente o método de Cramer
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
