//inclui as bibliotecas do sensor 
#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20_OneWire 4   // Pino digital do ESP32

//chama as bibliotecas do sensor 
OneWire oneWire(DS18B20_OneWire);
DallasTemperature sensortemp(&oneWire);

int ndispositivos = 0; //para detectar se o sensor foi detectado 
float grausC;

void setup()  
{
  Serial.begin(9600);

  //Inicializa o barramento OneWire e procura sensores conectados.

  sensortemp.begin(); 

  Serial.println("Localizando Dispositivos DS18B20...");
  //chama a função de detectar dispositivos
  ndispositivos = sensortemp.getDeviceCount(); 
  Serial.print("Encontrados ");
  Serial.print(ndispositivos);
  Serial.println(" dispositivos.");
  Serial.println("");
}

void loop() {

  /*
    Solicita ao sensor que faça a leitura da temperatura.
    O DS18B20 mede e armazena o valor internamente.
  */
  sensortemp.requestTemperatures(); 

  //Percorre todos os sensores conectados no barramento, apenas por segurança
  for (int i = 0; i < ndispositivos; i++) {

    Serial.println("Graus Celsius:");
    Serial.print("Sensor ");
    Serial.print(i + 1);
    Serial.print(": ");

    grausC = sensortemp.getTempCByIndex(i);

    Serial.print(grausC);
    Serial.println(" ºC");
  }

  Serial.println("");
  delay(1000);
}
