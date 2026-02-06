/****************************************************
   LEITURA DE CONDUTIVIDADE (uS/cm) - MELHOR PRECISÃO
   Modelo: EC = A*(1/R)^2 + B*(1/R) + C
   - Média móvel (10)
   - LCD I2C 16x2 + Serial
*****************************************************/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // Troque para 0x3F se seu módulo for esse

const int sensorPin = A0;
const float Vref = 5.0;
const float Rfix = 10000.0;

// Coeficientes do ajuste em 1/R (obtidos a partir dos seus 5 pontos)
const double A = -2.564676647e11;
const double B =  3.870610649e07;
const double C = -1.846756580e+01;

// ------- Filtro Média Móvel -------
const int window = 10;   // 10 amostras
double bufferEC[window];
int indexEC = 0;
bool filled = false;
// ----------------------------------

void setup() {
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Condutiv. - Ajuste");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(1200);

  for (int i = 0; i < window; i++) bufferEC[i] = 0.0;
}

void loop() {

  // --- leitura ADC ---
  int adc = analogRead(sensorPin);
  float V = adc * (Vref / 1023.0);

  // evita divisão por zero ou leituras inválidas
  if (V <= 0.01) {
    Serial.println("Leitura V invalida");
    delay(500);
    return;
  }

  // --- calcula resistência do divisor ---
  double R = Rfix * ((Vref / V) - 1.0);

  // evita R <= 0 (proteção extra)
  if (R <= 0.0) {
    Serial.println("R invalida");
    delay(500);
    return;
  }

  // --- calcula u = 1/R ---
  double u = 1.0 / R;

  // --- aplica modelo EC = A*u^2 + B*u + C ---
  double EC = A * (u * u) + B * u + C;

  // --- limite inferior a zero (não faz sentido negativo) ---
  if (EC < 0.0) EC = 0.0;

  // --- média móvel ---
  bufferEC[indexEC] = EC;
  indexEC++;
  if (indexEC >= window) {
    indexEC = 0;
    filled = true;
  }

  double soma = 0.0;
  int count = filled ? window : indexEC;
  for (int i = 0; i < count; i++) soma += bufferEC[i];
  double EC_filtrada = soma / (count > 0 ? count : 1);

  // --- Saída Serial ---
  Serial.print("R = ");
  Serial.print(R, 1);
  Serial.print(" ohms | Condutividade: ");
  Serial.print(EC_filtrada, 2);
  Serial.println(" uS/cm");

  // --- LCD ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cond: ");
  lcd.print(EC_filtrada, 1);
  lcd.print(" uS");

  lcd.setCursor(0, 1);
  lcd.print("R=");
  if (R >= 100000.0) {
    // para caber na tela, mostra em kOhm com 1 casa
    lcd.print(R / 1000.0, 1);
    lcd.print("k");
  } else {
    lcd.print(R, 0);
  }
  lcd.print(" ohm");

  delay(500);
}
