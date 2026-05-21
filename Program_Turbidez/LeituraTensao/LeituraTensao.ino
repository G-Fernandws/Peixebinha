#define SENSOR_PIN 33

const float VREF = 3.3;
const int ADC_MAX = 4095;

void setup() {

  Serial.begin(115200);

  analogReadResolution(12);

  Serial.println("Leitura de tensao no GPIO33");
}

void loop() {

  int leitura = analogRead(SENSOR_PIN);

  float tensao = (leitura * VREF) / ADC_MAX;

  Serial.print("ADC: ");
  Serial.print(leitura);

  Serial.print(" | Tensao: ");
  Serial.print(tensao, 3);
  Serial.println(" V");

  delay(1000);
}