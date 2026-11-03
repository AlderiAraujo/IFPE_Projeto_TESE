//Importação das bibliotecas
#include <Wire.h>     //Comunicão I2C entre Arduinos
#include <Keypad.h>     //Teclado Matricial
#include <LiquidCrystal_AIP31068_I2C.h>     //Display LCD I2C

int buttonPin = 2;      //Pino do botão

int motorSelecionado = 0;     //Variável p/ seleção de motor (atuador)

LiquidCrystal_AIP31068_I2C lcd(0x3E, 20, 4);     //Configuração do Display LCD I2C 20x4

//Configuração do teclado matricial 4x3
const byte LINHAS = 4;
const byte COLUNAS = 3;

const char TECLAS_MATRIZ[LINHAS][COLUNAS] = { //Mapeamento de teclas do Keypad
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

const byte PINOS_LINHAS[LINHAS] = {12, 11, 10, 9};  //Pinos Arduino para Linhas
const byte PINOS_COLUNAS[COLUNAS] = {7, 6, 5};   //Pinos Arduino para Colunas

Keypad teclado = Keypad(        //Criação do objeto teclado, utilizando o mapeamento e pinagem
  makeKeymap(TECLAS_MATRIZ),
  PINOS_LINHAS,
  PINOS_COLUNAS,
  LINHAS,
  COLUNAS
);
void setup() {
  pinMode(buttonPin, INPUT_PULLUP);     //Inicialização do pino do Botão com resistor pull-up

  lcd.init();     //Inicialização do Display

}

void loop() {
  

}
