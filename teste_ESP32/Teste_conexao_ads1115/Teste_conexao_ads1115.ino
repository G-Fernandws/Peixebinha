#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// =======================================
// INSTÂNCIA DO ADS1115
// =======================================

Adafruit_ADS1115 ads;

// =======================================
// ENDEREÇO I2C
// ADDR ligado ao GND -> 0x48
// =======================================

#define ADS1115_ADDRESS 0x48

// =======================================
// SETUP
// =======================================

void setup() {

  Serial.begin(115200);

  // SDA = GPIO21
  // SCL = GPIO22

  Wire.begin(21, 22);

  delay(1000);

  Serial.println();
  Serial.println("=================================");
  Serial.println(" TESTE DE COMUNICACAO ESP32 + ADS1115 ");
  Serial.println("=================================");

  // =======================================
  // SCANNER I2C
  // =======================================

  Serial.println("\nEscaneando barramento I2C...\n");

  byte dispositivos = 0;

  for (byte endereco = 1; endereco < 127; endereco++) {

    Wire.beginTransmission(endereco);

    if (Wire.endTransmission() == 0) {

      Serial.print("Dispositivo encontrado em: 0x");

      if (endereco < 16)
        Serial.print("0");

      Serial.println(endereco, HEX);

      dispositivos++;
    }
  }

  if (dispositivos == 0) {

    Serial.println("\nNenhum dispositivo I2C encontrado");
    Serial.println("Verifique as conexoes:");

    Serial.println("- SDA -> GPIO21");
    Serial.println("- SCL -> GPIO22");
    Serial.println("- GND comum");
    Serial.println("- VCC -> 3.3V");
    Serial.println("- ADDR -> GND");

    while (1);
  }

  // =======================================
  // TESTE DE INICIALIZACAO DO ADS1115
  // =======================================

  Serial.println("\nTentando inicializar ADS1115...");

  if (!ads.begin(ADS1115_ADDRESS)) {

    Serial.println("ERRO: ADS1115 nao inicializado");

    Serial.print("Endereco utilizado: 0x");
    Serial.println(ADS1115_ADDRESS, HEX);

    Serial.println("\nPossiveis causas:");
    Serial.println("- Endereco incorreto");
    Serial.println("- Falha na alimentacao");
    Serial.println("- Problema no barramento I2C");
    Serial.println("- ADS1115 defeituoso");

    while (1);
  }

  // =======================================
  // SUCESSO
  // =======================================

  Serial.println("\nADS1115 inicializado com sucesso");

  Serial.print("Endereco confirmado: 0x");
  Serial.println(ADS1115_ADDRESS, HEX);

  Serial.println("\nComunicacao I2C funcionando corretamente");
}

// =======================================
// LOOP
// =======================================

void loop() {

  Serial.println("ADS1115 conectado e respondendo");

  delay(3000);
}