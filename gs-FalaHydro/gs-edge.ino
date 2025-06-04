#include <EEPROM.h>  // Inclusão da biblioteca necessária
#include <Wire.h>  // Inclusão da biblioteca necessária
#include <LiquidCrystal_I2C.h>  // Inclusão da biblioteca necessária
#include <RTClib.h>  // Inclusão da biblioteca necessária

#define ECHO_PIN A2
#define TRIG_PIN A3

#define LOG_OPTION 1     // Opção para ativar a leitura do log
#define SERIAL_OPTION 0  // Opção de comunicação serial: 0 para desligado, 1 para ligado
#define UTC_OFFSET -3    // Ajuste de fuso horário para UTC-3

LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço de acesso: 0x3F ou 0x27
RTC_DS1307 RTC;

// LEDs de sinalização (verde = ok, amarelo = alerta, vermelho = perigo)
const short int ledVermelho = 6;
const short int ledAmarelo  = 5;
const short int ledVerde    = 4;

// Buzzer que emite som de alerta
const short int pinoBuzzer = 3;

// Botão físico usado para Iniciar setup
const short int botaoIniciar = 2;

// Tempo de espera entre leituras (em milissegundos)
const int tempoPrecisao = 100;

// Configurações da EEPROM
const int maxRecords = 100;
const int recordSize = 6; // Tamanho de cada registro em bytes
int startAddress = 0;
int endAddress = maxRecords * recordSize;
int currentAddress = 0;
 
int lastLoggedMinute = -1;

// Animação logo FalaHydro
int i;
// Definição do caractere customizado "gota deitada" (8x5 pixels)
// Parte superior da gota
byte gotaTopo[8] = {
  B00000,
  B00001,
  B00110,
  B11000,
  B00110,
  B00001,
  B00000,
  B00000
};

// Parte inferior da gota
byte gotaBaixo[8] = {
  B01100,
  B10010,
  B00001,
  B00001,
  B00001,
  B10010,
  B01100,
  B00000
};

// Parte superior da nuvem
byte nuvemTopo[8] = {
  B00110,
  B01001,
  B01001,
  B10010,
  B10010,
  B10001,
  B10001,
  B10010
};

// Parte inferior da nuvem
byte nuvemBaixo[8] = {
  B10010,
  B10001,
  B10001,
  B10010,
  B10001,
  B10001,
  B01001,
  B00110
};

float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  int duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

void setup() {
  // Configura os pinos como entrada ou saída
  pinMode(ledVermelho, OUTPUT);
  pinMode(ledAmarelo, OUTPUT);
  pinMode(ledVerde, OUTPUT);
  pinMode(pinoBuzzer, OUTPUT);
  pinMode(botaoIniciar, INPUT_PULLUP); // Usa resistor interno de pull-up
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.begin(115200);
  lcd.init();   // Inicialização do LCD
  lcd.backlight();  // Ligando o backlight do LCD

  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0xFF);  // Escreve 0xFF na posição i
  }
  Serial.println("EEPROM limpa!");

  RTC.begin();    // Inicialização do Relógio em Tempo Real
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // RTC.adjust(DateTime(2024, 5, 6, 08, 15, 00));  // Ajustar a data e hora apropriadas uma vez inicialmente, depois comentar

  EEPROM.begin();

  //Posiciona o cursor na primeira coluna(0) e na primeira linha(0) do LCD   
  lcd.setCursor(16, 0);   
  lcd.print("     Fala "); //Escreve no LCD "Fala"   
  lcd.setCursor(16, 1); //Posiciona o cursor na primeira coluna(0) e na segunda linha(1) do LCD   
  lcd.print("      Hydro "); //Escreve no LCD "Hydro"   
  delay(3000);

  //efeito de deslocamento para esquerda   
  for (i =0; i <16; i++){     
  lcd.scrollDisplayLeft();     
  delay(20);   
  }   
  delay(1000); //Aguarda 1 segundo   

  //para esquerda   
  for (i =0; i <16; i++){     
  lcd.scrollDisplayLeft();     
  delay(40);   
  }

  lcd.clear(); 

  // Criar caracteres customizados
  lcd.createChar(0, gotaTopo);
  lcd.createChar(1, gotaBaixo);
  lcd.createChar(2, nuvemTopo);
  lcd.createChar(3, nuvemBaixo);
  lcd.clear();

  int linhaTopo = 0;
  int linhaBaixo = 1;

  // Desenha a nuvem fixa (colunas 0 e 1 nas linhas 0 e 1)
  for (int col = 0; col <= 1; col++) {
    lcd.setCursor(col, 0);
    lcd.write(byte(2)); // nuvem topo
    lcd.setCursor(col, 1);
    lcd.write(byte(3)); // nuvem baixo
  }

  // Animação das gotas saindo da nuvem
  // Vamos animar 3 gotas alternando entre linha 0 e 1
  for (int gota = 0; gota < 3; gota++) {
    int linha = gota % 2; // 0 ou 1 para alternar linhas

    // Gota se move da coluna 2 até a 13 (para caber 2 caracteres lado a lado)
    for (int col = 2; col <= 13; col++) {
      // Desenha a gota (2 caracteres lado a lado)
      lcd.setCursor(col, linha);
      lcd.write(byte(0)); // gota topo (esquerda)
      lcd.setCursor(col + 1, linha);
      lcd.write(byte(1)); // gota baixo (direita)

      delay(120);

      // Limpa a gota anterior para simular movimento (exceto na primeira posição)
      if (col > 2) {
        lcd.setCursor(col - 1, linha);
        lcd.print("  "); // apaga os 2 caracteres da gota anterior
      }

      // Redesenha a nuvem para garantir que não seja apagada
      for (int c = 0; c <= 1; c++) {
        lcd.setCursor(c, 0);
        lcd.write(byte(2));
        lcd.setCursor(c, 1);
        lcd.write(byte(3));
      }
    }

    // Limpa a última gota após sair do loop
    lcd.setCursor(13, linha);
    lcd.print("  ");
  }

  lcd.clear();
  lcd.print("      BEM"); // Mensagem inicial
  lcd.setCursor(0, 1); // Vai para a segunda linha
  lcd.print("     VINDO!"); 
  delay(3000); // Exibe a mensagem por 3 segundos
  lcd.clear(); // Limpa o display

  setupInit();
}

void loop() {
  DateTime now = RTC.now();
  // Calculando o deslocamento do fuso horário
  int offsetSeconds = UTC_OFFSET * 3600; // Convertendo horas para segundos
  now = now.unixtime() + offsetSeconds; // Adicionando o deslocamento ao tempo atual
  // Convertendo o novo tempo para DateTime
  DateTime adjustedTime = DateTime(now);

  if (LOG_OPTION) get_log();

  // Verifica se o minuto atual é diferente do minuto do último registro
  if (adjustedTime.minute() != lastLoggedMinute) {
    lastLoggedMinute = adjustedTime.minute();

    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);                       // wait for a second

    // Ler os valores de distancia
    float distance = readDistanceCM();

    if (!isnan(distance)) {
      if (distance < 20) {
        // Converter valores para int para armazenamento
        int distInt = (int)(distance * 100);

        // Escrever dados na EEPROM
        EEPROM.put(currentAddress, now.unixtime());
        EEPROM.put(currentAddress + 4, distInt);
        getNextAddress();
      }
    }
  }   

  if (SERIAL_OPTION) {
    Serial.print(adjustedTime.day());
    Serial.print("/");
    Serial.print(adjustedTime.month());
    Serial.print("/");
    Serial.print(adjustedTime.year());
    Serial.print(" ");
    Serial.print(adjustedTime.hour() < 10 ? "0" : ""); // Adiciona zero à esquerda se hora for menor que 10
    Serial.print(adjustedTime.hour());
    Serial.print(":");
    Serial.print(adjustedTime.minute() < 10 ? "0" : ""); // Adiciona zero à esquerda se minuto for menor que 10
    Serial.print(adjustedTime.minute());
    Serial.print(":");
    Serial.print(adjustedTime.second() < 10 ? "0" : ""); // Adiciona zero à esquerda se segundo for menor que 10
    Serial.print(adjustedTime.second());
    Serial.print("\n");
  }

  // // Exibe a distancia no LCD
  lcd.clear();
  float distance = readDistanceCM();
  lcd.setCursor(0, 0);
  lcd.print("Dist: "); 
  lcd.print(distance); 
  lcd.print(" cm");

  // Verifica os níveis de distancia
 if (distance > 10) {
    // Nível ideal: LED verde ligado
    digitalWrite(ledVerde, HIGH);
    digitalWrite(ledAmarelo, LOW);
    digitalWrite(ledVermelho, LOW);
    digitalWrite(pinoBuzzer, LOW);
    lcd.setCursor(0, 1); 
    lcd.print("Status: OK!             ");
    noTone(pinoBuzzer);
 } else if (distance > 5 && distance <= 10){
    // Nível de alerta: LED amarelo ligado
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarelo, HIGH);
    digitalWrite(ledVermelho, LOW);
    lcd.setCursor(0, 1); 
    lcd.print("Status: ALERTA!         ");
    // Ativa o buzzer por 3s
    digitalWrite(pinoBuzzer, HIGH);
    tone(pinoBuzzer, 1000); // Emite som em 1000 Hz
    delay(3000);
    noTone(pinoBuzzer);
 }
  else{
    // Nível problemático
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarelo, LOW);
    digitalWrite(ledVermelho, HIGH);
    lcd.setCursor(0, 1); 
    lcd.print("Status: PERIGO!           ");
    // Ativa o buzzer por 1s
    digitalWrite(pinoBuzzer, HIGH);
    tone(pinoBuzzer, 500); // Emite som em 500 Hz
    delay(1000);
    noTone(pinoBuzzer);  
  }
 delay(1000);
}

void getNextAddress() {
  currentAddress += recordSize;
  if (currentAddress >= endAddress) {
      currentAddress = 0; // Volta para o começo se atingir o limite
  }
}

void get_log() {
  Serial.println("Data stored in EEPROM:");
  Serial.println("Timestamp\t\tDistance");
 
  for (int address = startAddress; address < endAddress; address += recordSize) {
    long timeStamp;
    int distInt;

    // Ler dados da EEPROM
    EEPROM.get(address, timeStamp);
    EEPROM.get(address + 4, distInt);

    // Converter valores
    float distance = distInt / 100.0;

    // Verificar se os dados são válidos antes de imprimir
    if (timeStamp != 0xFFFFFFFF) { // 0xFFFFFFFF é o valor padrão de uma EEPROM não inicializada
        //Serial.print(timeStamp);
        DateTime dt = DateTime(timeStamp);
        Serial.print(dt.timestamp(DateTime::TIMESTAMP_FULL));
        Serial.print("\t");
        Serial.print(distance);
        Serial.println(" cm");
    }
  }
}

// Inicialização do setup, guiado por mensagens no display
void setupInit() {
  lcd.clear();
  lcd.setCursor(0, 0);

  // Mensagem de início do setup
  char mensagem[] = "Iniciando SETUP";
  animacaoPrint(mensagem, 150);
  lcd.setCursor(0, 1);
  lcd.print("Pressione");

  // Pressionar para continuar
  while (digitalRead(botaoIniciar) == HIGH);
  delay(100);
  while (digitalRead(botaoIniciar) == LOW);
}

// Texto no LCD com efeito de digitação
// Aparece a logo
void animacaoPrint(char mensagem[], int tempo) {
  lcd.clear();

  for (int i = 0; i < strlen(mensagem); i++) {
    lcd.print(mensagem[i]);
    delay(tempo);

    if (i > 14) {
      lcd.setCursor(i - 15, 1);
    }
  }
}
