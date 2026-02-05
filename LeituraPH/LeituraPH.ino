
#include <Wire.h>
//#include <LiquidCrystal_I2C.h>

//LiquidCrystal_I2C lcd(0x27, 16, 2);


float calibracao_ph7 = 2.57;   // Tensão obtida em solução de calibração pH 7 historico de valores 2.54,2.48
float calibracao_ph4 = 2.86;   // Tensão obtida em solução de calibração pH 4 historico de valores 3.05,2.99
float calibracao_ph10 = 0.0;  // Tensão obtida em solução de calibração pH 10

#define UTILIZAR_PH_10 false  // Habilita calibração entre pH 7 e 10, \
                                 caso contrário utiliza pH 7 e 4.
float m;
float b;
int buf[10];

void setup() {
  Serial.begin(9600);

  if (calibracao_ph7 == 0 && calibracao_ph4 == 0 && calibracao_ph10 == 0) {
    delay(500);
    Serial.println();
    Serial.println("Erro - Necessário colocar os valores de calibração no código!");
    while (1)
      ;
  }

  if (!UTILIZAR_PH_10 && calibracao_ph4 == 0 && calibracao_ph10 != 0 && calibracao_ph7 != 0) {
    delay(500);
    Serial.println();
    Serial.println("Erro - Você não marcou a opção UTILIZAR_PH_10 como true para utilizar pH 7 e 10.");
    while (1)
      ;
  }

  /*lcd.init();                  // Inicializa o display
  lcd.begin(16, 2);            // Inicializa todos os caracteres
  lcd.backlight();             // Inicializa o backlight
  lcd.setCursor(0, 0);         // Posiciona o cursor na posição
  lcd.print("LAQUA");      // Escreve no monitor serial
  lcd.setCursor(0, 1);         // Posiciona o cursor na posição
  lcd.print("Leitura de pH");  // Escreve no monitor serial
  delay(2000);
  lcd.clear();  // Limpa o conteúdo do display*/


  if (UTILIZAR_PH_10) {
    m = (7.0 - 10.0) / (calibracao_ph7 - calibracao_ph10);
    b = 10.0 - m * calibracao_ph10;
  } else {
    m = (4.0 - 7.0) / (calibracao_ph4 - calibracao_ph7);
    b = 7.0 - m * calibracao_ph7;
  }
}


void loop() {
  for (int i = 0; i < 10; i++) {  // 11 amostras
    buf[i] = analogRead(A0);      // Ler o sensor PH
    delay(10);
  }

  for (int i = 0; i < 9; i++) {  // Ordena em ordem crescente
    for (int j = i + 1; j < 10; j++) {
      if (buf[i] > buf[j]) {
        int temp = buf[i];
        buf[i] = buf[j];
        buf[j] = temp;
      }
    }
  }

  int valorMedio = 0;
  for (int i = 2; i < 8; i++) {  // Realiza o valor médio utilizando 6 amostras
    valorMedio += buf[i];
  }

  float tensao = (valorMedio * 5.0) / 1024.0 / 6;  // Realiza a média e \
                                                    transforma o valor \
                                                    analógico em volt
  float ph = m * tensao + b; // Converte para pH
  Serial.println(ph);
  delay(1000);

  /*lcd.setCursor(0, 0);      // Posiciona o cursor no display
  lcd.print("Valor pH: ");  // Escreve no display
  lcd.setCursor(11, 0);     // Posiciona o cursor no display
  lcd.print(ph, 1);         // Escreve o pH com uma casa decimal
  delay(1000);              // Aguarda para próxima leitura*/
}