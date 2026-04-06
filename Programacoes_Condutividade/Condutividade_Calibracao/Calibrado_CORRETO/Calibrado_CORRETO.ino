const int pinoSensor = 32;
const int numLeituras = 30;

float fatorTDS = 0.5;   // fator de conversão EC → TDS

void setup() 
{
  Serial.begin(115200);
}

float lerSensorFiltrado()
{

  long soma = 0;

  for(int i = 0; i < numLeituras; i++)
  {
    soma += analogRead(pinoSensor);
    delay(10);
  }

  float mediaADC = soma / (float)numLeituras;

  return mediaADC;
}

void loop()
{

  float leituraADC = lerSensorFiltrado();

  float tensao = leituraADC * (3.3 / 4095.0);

  // Equação de calibração obtida no gráfico
  float condutividade = (tensao + 0.0572) / 0.0013;

  // Conversão EC → TDS
  float TDS = condutividade * fatorTDS;

  Serial.print("Tensao: ");
  Serial.print(tensao);
  Serial.print(" V");

  Serial.print(" | Condutividade: ");
  Serial.print(condutividade);
  Serial.print(" uS/cm");

  Serial.print(" | TDS: ");
  Serial.print(TDS);
  Serial.println(" mg/L");

  delay(2000);
}