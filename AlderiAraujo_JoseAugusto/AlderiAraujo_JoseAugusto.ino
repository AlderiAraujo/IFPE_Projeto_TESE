/*
 * PROJETO: Controle de 3 Atuadores via I2C (ARDUINO MASTER)
 * EQUIPE: Alderi Araújo e José Augusto
 * HARDWARE: Arduino UNO + Display AIP31068 (0x3E) + Teclado 4x3 + Push Button
 */

#include <Wire.h>
#include <Keypad.h>


// CONFIGURAÇÕES DE ENDEREÇOS I2C
#define LCD_ADDR   0x3E  // Endereço do Display AIP31068 (62 em decimal)
#define SLAVE_ADDR 0x08  // Endereço do Arduino Slave


// CONFIGURAÇÃO DO PUSH BUTTON E ESTADOS
const byte PINO_BOTAO = 2;

enum EstadoMotor {
  PARADO = 0,
  MOTOR_DC = 1,
  SERVO_MOTOR = 2,
  MOTOR_PASSO = 3
};

EstadoMotor motorAtual = PARADO;
bool estadoAnteriorBotao = HIGH;
unsigned long ultimoDebounce = 0;


// CONFIGURAÇÃO DO TECLADO MATRICIAL 4x3 (Conforme XML do SimulIDE)
const byte LINHAS = 4;
const byte COLUNAS = 3;

char teclas[LINHAS][COLUNAS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

// Pin0..Pin3 do KeyPad ligados em D12, D11, D10, D9
byte pinosLinhas[LINHAS] = {12, 11, 10, 9};
// Pin4..Pin6 do KeyPad ligados em D7, D6, D5
byte pinosColunas[COLUNAS] = {7, 6, 5};

Keypad teclado = Keypad(makeKeymap(teclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);


// VARIÁVEIS DE CONTROLE DOS MOTORES

// Motor DC
byte dcVelocidade = 0;       // 0 a 255 (PWM)
byte dcSentido = 1;          // 1 = Horário, 2 = Anti-horário

// Servo Motor
int servoAnguloAtual = 0;    // 0 a 180 graus
int servoAnguloAnterior = 0; // Para calcular o sentido de giro do servo
String servoSentidoStr = "-";

// Motor de Passo
byte passoSentido = 1;       // 1 = Horário, 2 = Anti-horário
int passoQtd = 0;            // Quantidade de passos a dar

// Variável auxiliar para digitação numérica (Servo e Passo)
String bufferDigitacao = "";


// FUNÇÕES NATIVAS DO DISPLAY AIP31068 (I2C)
void lcdComando(byte cmd) {
  Wire.beginTransmission(LCD_ADDR);
  Wire.write(0x00); // Control byte: Co=0, RS=0 (Comando)
  Wire.write(cmd);
  Wire.endTransmission();
}

void lcdEscreverChar(char c) {
  Wire.beginTransmission(LCD_ADDR);
  Wire.write(0x40); // Control byte: Co=0, RS=1 (Dado)
  Wire.write(c);
  Wire.endTransmission();
}

void lcdLimpar() {
  lcdComando(0x01);
  delay(2);
}

void lcdCursor(byte col, byte lin) {
  byte endereco = (lin == 0) ? (0x80 + col) : (0xC0 + col);
  lcdComando(endereco);
}

void lcdPrint(String texto) {
  for (unsigned int i = 0; i < texto.length(); i++) {
    lcdEscreverChar(texto[i]);
  }
}

void lcdIniciar() {
  delay(50);
  lcdComando(0x38); // Modo 8 bits, 2 linhas, fonte 5x8
  delay(5);
  lcdComando(0x0C); // Display ligado, cursor desligado
  delay(5);
  lcdLimpar();
  lcdComando(0x06); // Incremento automático do cursor
  delay(5);
}


// COMUNICAÇÃO I2C COM O ARDUINO SLAVE
void enviarDadosSlave(byte executarPasso) {
  Wire.beginTransmission(SLAVE_ADDR);
  Wire.write((byte)motorAtual);       // Byte 0: Motor selecionado
  Wire.write(dcVelocidade);           // Byte 1: Velocidade DC (0-255)
  Wire.write(dcSentido);              // Byte 2: Sentido DC (1=Hor, 2=Anti)
  Wire.write((byte)servoAnguloAtual); // Byte 3: Ângulo Servo (0-180)
  Wire.write(executarPasso);          // Byte 4: 1 = Dispara passos agora
  Wire.write(passoSentido);           // Byte 5: Sentido Passo (1=Hor, 2=Anti)
  Wire.write((byte)passoQtd);         // Byte 6: Qtd de passos (0-255)
  Wire.endTransmission();
}


// ATUALIZAÇÃO DA TELA DO DISPLAY
void atualizarDisplay() {
  lcdLimpar();
  
  switch (motorAtual) {
    case PARADO:
      lcdCursor(0, 0);
      lcdPrint("Alderi Araujo");
      lcdCursor(0, 1);
      lcdPrint("Jose Augusto");
      break;

    case MOTOR_DC: {
      int pct = map(dcVelocidade, 0, 255, 0, 100);
      lcdCursor(0, 0);
      lcdPrint("1.MOTOR DC");
      lcdCursor(0, 1);
      lcdPrint("Vel:" + String(pct) + "% ");
      lcdPrint((dcSentido == 1) ? "Hor" : "Anti");
      break;
    }

    case SERVO_MOTOR:
      lcdCursor(0, 0);
      lcdPrint("2.SERVO Ang:" + String(servoAnguloAtual) + (char)223);
      lcdCursor(0, 1);
      lcdPrint("Dir:" + servoSentidoStr + " Dig:" + bufferDigitacao);
      break;

    case MOTOR_PASSO:
      lcdCursor(0, 0);
      lcdPrint("3.PASSO ");
      lcdPrint((passoSentido == 1) ? "Dir:Hor" : "Dir:Anti");
      lcdCursor(0, 1);
      lcdPrint("Qtd:" + String(passoQtd) + " Dig:" + bufferDigitacao);
      break;
  }
}


// SETUP
void setup() {
  Wire.begin(); // Inicia barramento I2C como Master
  pinMode(PINO_BOTAO, INPUT_PULLUP);

  lcdIniciar();
  atualizarDisplay(); // Mostra os nomes da equipe inicialmente
  enviarDadosSlave(0); // Garante todos os motores parados no início
}


// LOOP PRINCIPAL
void loop() {
  // 1. LEITURA DO PUSH BUTTON (Seleção do Motor)
  bool leituraBotao = digitalRead(PINO_BOTAO);
  
  if (leituraBotao == LOW && estadoAnteriorBotao == HIGH && (millis() - ultimoDebounce > 250)) {
    ultimoDebounce = millis();
    bufferDigitacao = ""; // Limpa digitação pendente ao trocar de motor
    
    // Avança ciclicamente: PARADO -> DC -> SERVO -> PASSO -> PARADO
    motorAtual = (EstadoMotor)((motorAtual + 1) % 4);
    
    atualizarDisplay();
    enviarDadosSlave(0); // Avisa o Slave imediatamente (para o motor anterior)
  }
  estadoAnteriorBotao = leituraBotao;

  // 2. LEITURA DO TECLADO MATRICIAL
  char tecla = teclado.getKey();
  if (!tecla) return; // Se nenhuma tecla foi pressionada, encerra o ciclo

  // Ignora teclado se estiver no modo PARADO
  if (motorAtual == PARADO) return;

  // --- CONTROLE DO MOTOR DC ---
  if (motorAtual == MOTOR_DC) {
    if (tecla >= '0' && tecla <= '9') {
      int nivel = tecla - '0'; // Converte char '0'..'9' para int 0..9
      dcVelocidade = map(nivel, 0, 9, 0, 255);
    } 
    else if (tecla == '*') {
      dcSentido = 1; // Horário
    } 
    else if (tecla == '#') {
      dcSentido = 2; // Anti-horário
    }
    atualizarDisplay();
    enviarDadosSlave(0);
  }

  // --- CONTROLE DO SERVO MOTOR ---
  else if (motorAtual == SERVO_MOTOR) {
    if (tecla >= '0' && tecla <= '9') {
      if (bufferDigitacao.length() < 3) {
        bufferDigitacao += tecla;
      }
    } 
    else if (tecla == '*') {
      bufferDigitacao = ""; // Limpa digitação
    } 
    else if (tecla == '#') {
      if (bufferDigitacao.length() > 0) {
        int novoAngulo = bufferDigitacao.toInt();
        if (novoAngulo > 180) novoAngulo = 180; // Trava de segurança do servo
        
        // Define sentido baseado no movimento
        if (novoAngulo > servoAnguloAtual) servoSentidoStr = "Hor";
        else if (novoAngulo < servoAnguloAtual) servoSentidoStr = "Anti";
        else servoSentidoStr = "Par";

        servoAnguloAnterior = servoAnguloAtual;
        servoAnguloAtual = novoAngulo;
        bufferDigitacao = "";
        enviarDadosSlave(0);
      }
    }
    atualizarDisplay();
  }

  // --- CONTROLE DO MOTOR DE PASSO ---
  else if (motorAtual == MOTOR_PASSO) {
    if (tecla >= '0' && tecla <= '9') {
      if (bufferDigitacao.length() < 3) {
        bufferDigitacao += tecla;
      }
    } 
    else if (tecla == '*') {
      // Alterna sentido Horário <-> Anti-horário
      passoSentido = (passoSentido == 1) ? 2 : 1;
      bufferDigitacao = "";
    } 
    else if (tecla == '#') {
      if (bufferDigitacao.length() > 0) {
        int qtd = bufferDigitacao.toInt();
        if (qtd > 255) qtd = 255;
        passoQtd = qtd;
        bufferDigitacao = "";
        atualizarDisplay();
        enviarDadosSlave(1); // Envia comando '1' para executar os passos
        return;
      }
    }
    atualizarDisplay();
  }
}
