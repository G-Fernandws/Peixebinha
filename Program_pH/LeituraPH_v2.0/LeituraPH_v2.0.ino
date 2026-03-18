#define PH_PIN 34

float slope = -9.0909;
float intercept = 21.86;

void setup()
{
  Serial.begin(115200);
}

void loop() 
{

  int amostras = 30;
  long soma = 0;

  for (int i = 0; i < amostras; i++)
  {
    soma += analogRead(PH_PIN);
    delay(10);
  }

  float media = soma / (float)amostras;

  // ESP32 ADC (12 bits)
  float voltage = (media / 4095.0) * 3.3;

  // Equação calibrada
  float pH = (slope * voltage) + intercept;

  Serial.print("RAW: ");
  Serial.print(media);

  Serial.print(" | Tensao: ");
  Serial.print(voltage, 4);

  Serial.print(" V | pH: ");
  Serial.println(pH, 2);

  delay(1000);
}