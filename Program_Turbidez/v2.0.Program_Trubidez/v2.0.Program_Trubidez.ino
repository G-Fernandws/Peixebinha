/*
LÓGICA BÁSICA DA CALIBRAÇÃO:
* O sensor gera uma tensão analógica proporcional à turbidez.
* Água limpa → maior tensão
* Água turva → menor tensão

Registra 2 valores, depois, todas as leituras serão
normalizadasentre esses dois extremos.

1. Tensão em água limpa  → Vclear
2. Tensão em água turva → Vdirty
*/

const int turbidezPin = 35;

const float Vref = 3.3;     // Tensão máxima do ADC do ESP32
const int ADCmax = 4095;    // Resolução 12 bits

//variáveis de calibração
float Vclear = 0;   // tensão medida em água limpa
float Vdirty = 0;   // tensão medida em água muito turva

bool calibrado = false;

void setup()
{
Serial.begin(115200);
delay(2000);

Serial.println("=== SENSOR DE TURBIDEZ - ESP32 ===");
Serial.println("Calibracao em 2 etapas:");
Serial.println("1) Agua limpa");
Serial.println("2) Agua turva");
Serial.println("");
}

void loop()
{
/*Executa calibração apenas uma vez,
outra forma de assegurar que a calibração
não vai se repetir*/
  if (!calibrado) 
  {
  calibrarSensor();
  calibrado = true;
  }

int adc = analogRead(turbidezPin);

// Converte leitura ADC para tensão
float V = adc * (Vref / ADCmax);

// Normaliza o valor entre os extremos calibrados
float turbidezRelativa = (Vclear - V) / (Vclear - Vdirty);

// Limita entre 0 e 1 pois a sensibilidade
// do sensor é baixa demais
if (turbidezRelativa < 0) turbidezRelativa = 0;
if (turbidezRelativa > 1) turbidezRelativa = 1;

// Mostra dados
Serial.print("ADC: ");
Serial.print(adc);

Serial.print(" | Tensao: ");
Serial.print(V, 3);
Serial.print(" V");

Serial.print(" | Turbidez relativa: ");
Serial.print(turbidezRelativa * 100);
Serial.println(" %");

delay(1000);
}

// FUNÇÃO DE CALIBRAÇÃO
void calibrarSensor()
{

Serial.println("=== INICIANDO CALIBRACAO ===");


// AGUA LIMPA
Serial.println("\nColoque o sensor em AGUA LIMPA e pressione ENTER");
esperarEnter();

Vclear = mediaTensao();

Serial.print("Tensao agua limpa: ");
Serial.println(Vclear, 3);

// PASSO 2 - AGUA TURVA
Serial.println("\nAgora coloque o sensor em AGUA TURVA e pressione ENTER");
esperarEnter();

Vdirty = mediaTensao();

Serial.print("Tensao agua turva: ");
Serial.println(Vdirty, 3);

Serial.println("\nCalibracao finalizada!");
}

// FUNÇÃO: ESPERA ENTER NO MONITOR SERIAL
void esperarEnter()
{
while (Serial.available() == 0) {}
while (Serial.available() > 0) Serial.read();
}

// FUNÇÃO: CALCULA MÉDIA DA TENSÃO (reduz ruído nas leituras)
float mediaTensao()
{

float soma = 0;

//coloquei 50 pois as leituras são muito instáveis
for (int i = 0; i < 50; i++) 
{
int adc = analogRead(turbidezPin);
float V = adc * (Vref / ADCmax);
soma += V;
delay(40);
}

return soma / 50.0;
}
