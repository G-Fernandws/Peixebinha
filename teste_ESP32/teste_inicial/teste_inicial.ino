void setup() 
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("Teste ESP32 iniciado!");
}

void loop()
{
  Serial.println("ESP32 funcionando corretamente.");
  delay(2000);
}