#include <OneWire.h>
#include <DallasTemperature.h>
 
#define DS18B20_OneWire 2
 
OneWire oneWire(DS18B20_OneWire);
 
DallasTemperature sensortemp(&oneWire);
 
int ndispositivos = 0;
float grausC;
 
void setup()  {
  sensortemp.begin(); 
  Serial.begin(9600);
 
  Serial.println("Localizando Dispositivos ...");
  Serial.print("Encontrados ");
  ndispositivos = sensortemp.getDeviceCount();
  Serial.print(ndispositivos, DEC);
  Serial.println(" dispositivos.");
  Serial.println("");
}
 
void loop() { 
  sensortemp.requestTemperatures(); 
 
  for (int i = 0;  i < ndispositivos;  i++) {
    Serial.println("Graus Celsius:");
    Serial.print("Sensor ");
    Serial.print(i+1);
    Serial.print(": ");
    grausC = sensortemp.getTempCByIndex(i);
    Serial.print(grausC);
    Serial.println("ºC");
  }
  
  Serial.println("");
  delay(1000);
}