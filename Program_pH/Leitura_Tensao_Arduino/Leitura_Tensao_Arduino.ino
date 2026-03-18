#define PH_PIN A0

void setup() 
{
  Serial.begin(9600);
}

void loop() 
{
  int raw = analogRead(PH_PIN);

  // Converte para tensão (ADC 10 bits)
  float voltage = raw * (5.0 / 1023.0);

  Serial.print("RAW: ");
  Serial.print(raw);
  Serial.print(" | Tensao: ");
  Serial.print(voltage, 3);
  Serial.println(" V");

  delay(500);
}