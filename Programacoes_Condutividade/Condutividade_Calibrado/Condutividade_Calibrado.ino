/****************************************************
LEITURA DE CONDUTIVIDADE (uS/cm) - ESP32 GPIO32

* Apenas Monitor Serial
* Média móvel (10)
* Verificação de sensor conectado
  *****************************************************/

const int sensorPin = 32;   // GPIO32 (ADC1 - recomendado)
const float Vref = 3.3;     // Referência do ESP32
const float Rfix = 10000.0;

// Coeficientes do ajuste em 1/R
const double A = -2.564676647e11;
const double B =  3.870610649e07;
const double C = -1.846756580e+01;

// ------- Filtro Média Móvel -------
const int window = 10;
double bufferEC[window];
int indexEC = 0;
bool filled = false;
// ----------------------------------

void setup() {
Serial.begin(115200);
delay(1000);

Serial.println("=== Leitura de Condutividade - ESP32 ===");
Serial.println("Inicializando ADC GPIO32...");

for (int i = 0; i < window; i++) bufferEC[i] = 0.0;
}

void loop() {

int adc = analogRead(sensorPin);

// -------- Verificação de sensor conectado --------
if (adc <= 5) {
Serial.println("ERRO: Sensor desconectado ou sem sinal (ADC ~0)");
delay(1000);
return;
}

if (adc >= 4090) {
Serial.println("ERRO: Leitura saturada (ADC max). Verifique ligacao.");
delay(1000);
return;
}
// -------------------------------------------------

float V = adc * (Vref / 4095.0);

if (V <= 0.01) {
Serial.println("Leitura de tensao invalida");
delay(1000);
return;
}

// Calcula resistência
double R = Rfix * ((Vref / V) - 1.0);

if (R <= 0.0) {
Serial.println("Resistencia calculada invalida");
delay(1000);
return;
}

double u = 1.0 / R;
double EC = A * (u * u) + B * u + C;

if (EC < 0.0) EC = 0.0;

// -------- Média móvel --------
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
// -----------------------------

// -------- Confirmação de funcionamento --------
Serial.print("Sensor OK | ADC: ");
Serial.print(adc);

Serial.print(" | V: ");
Serial.print(V, 3);

Serial.print(" V | R: ");
Serial.print(R, 1);

Serial.print(" ohms | EC: ");
Serial.print(EC_filtrada, 2);
Serial.println(" uS/cm");
// ---------------------------------------------

delay(1000);
}
