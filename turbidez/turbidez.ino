// Inicia as bibliotecas do Display LCD
#include <Wire.h>
//#include <LiquidCrystal_I2C.h>
 
// Inicia o Display LCD 16x2 através do endereço 0x27
//LiquidCrystal_I2C lcd(0x27, 16, 2);
 
// Define o pino de Leitura do Sensor
int SensorTurbidez = A0;
 
// Inicia as variáveis
int i;
float voltagem;
float NTU;
 
 
void setup() {

Serial.begin(9600);
Serial.println("Sensor TSW-30 - Medição de Turbidez");
// Inicia o display LCD
////lcd.init();
//lcd.backlight();
}
 
void loop() {
// Inicia a leitura da voltagem em 0
voltagem = 0;
 
// Realiza a soma dos "i" valores de voltagem
for (i = 0; i < 50; i++) {
voltagem += ((float)analogRead(SensorTurbidez) / 1023) * 5;
}
 
// Realiza a média entre os valores lidos na função for acima
voltagem = voltagem / 50;
voltagem = ArredondarPara(voltagem, 3);
 
// Se Voltagem menor que 2.5 fixa o valor de NTU
if (voltagem < 2.5) {
NTU = 3000;
}
 
else if (voltagem > 3.78) {
NTU = 0;
}
 
// Senão calcula o valor de NTU através da fórmula
else {
NTU = 499.059 * voltagem * voltagem+ -5161.337 * voltagem + 12367.676;
}


Serial.print("Analógico: ");
Serial.print(analogRead(SensorTurbidez));
Serial.print(" | ");
Serial.print("Tensão: ");
Serial.print(voltagem, 3);
Serial.print("V | NTU: ");
Serial.print(NTU, 1);
Serial.print(" | Nível: ");
Serial.println(getTurbidityLevel(voltagem));
 
// Imprime as informações na tela do LCD
//lcd.clear();
//lcd.setCursor(0, 0);
//lcd.print("Leitura: ");
//lcd.print(voltagem);
//lcd.print(" V");
 
// Imprime as informações na tela do LCD
//lcd.setCursor(3, 1);
//lcd.print(NTU);
//lcd.print(" NTU");
delay(10);

delay(2000);
}
 
// Sistema de arredendamento para Leitura
float ArredondarPara( float ValorEntrada, int CasaDecimal ) {
float multiplicador = powf( 10.0f, CasaDecimal );
ValorEntrada = roundf( ValorEntrada * multiplicador ) / multiplicador;
return ValorEntrada;
}

// Classificação por níveis (conforme datasheet)
String getTurbidityLevel(float voltage) {
  if (voltage >= 2.96) return "Nível 1 - Muito Limpa";
  if (voltage >= 2.64) return "Nível 2 - Limpa"; 
  if (voltage >= 1.84) return "Nível 3 - Moderada";
  return "Nível 4 - Turva";
}