#include <Wire.h>
#include <Servo.h>
#include <Stepper.h>

#define SLAVE_ADDR 0x08

#define PIN_DC_EN  3
#define PIN_DC_1A  2
#define PIN_DC_2A  4
#define PIN_SERVO  11

#define PASSOS_POR_VOLTA 32
Stepper motorPasso(PASSOS_POR_VOLTA, 6, 8, 7, 9);
Servo meuServo;

volatile byte modoRecebido = 0;  // 0=Parado, 1=DC, 2=Servo, 3=Passo
volatile byte dcVelocidade = 0;  // 0 a 255
volatile byte dcSentido = 1;     // 1=Horário, 2=Anti-horário
volatile byte servoAngulo = 0;   // 0 a 180
volatile byte passoExecutar = 0; // 1=Dispara passos
volatile byte passoSentido = 1;  // 1=Horário, 2=Anti-horário
volatile byte passoQtd = 0;      // 0 a 255

int passosRestantes = 0;
unsigned long tempoUltimoPasso = 0;
const unsigned long intervaloPasso = 15;

void receberDadosI2C(int quantBytes) {
  if (quantBytes >= 7) {
    modoRecebido  = Wire.read();
    dcVelocidade  = Wire.read();
    dcSentido     = Wire.read();
    servoAngulo   = Wire.read();
    byte cmdPasso = Wire.read();
    passoSentido  = Wire.read();
    passoQtd      = Wire.read();

    if (cmdPasso == 1) {
      passosRestantes = passoQtd;
    }
  }
  while (Wire.available()) Wire.read();
}

void pararMotorDC() {
  analogWrite(PIN_DC_EN, 0);
  digitalWrite(PIN_DC_1A, LOW);
  digitalWrite(PIN_DC_2A, LOW);
}

void desligarBobinasPasso() {
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
}

void setup() {
  pinMode(PIN_DC_EN, OUTPUT);
  pinMode(PIN_DC_1A, OUTPUT);
  pinMode(PIN_DC_2A, OUTPUT);
  pararMotorDC();

  meuServo.attach(PIN_SERVO);
  meuServo.write(0);

  motorPasso.setSpeed(60);
  desligarBobinasPasso();

  Wire.begin(SLAVE_ADDR);
  Wire.onReceive(receberDadosI2C);
}

void loop() {
  byte modo = modoRecebido;

  if (modo == 0) { // PARADO
    pararMotorDC();
    passosRestantes = 0;
    desligarBobinasPasso();
  } 
  else if (modo == 1) { // MOTOR DC
    passosRestantes = 0;
    desligarBobinasPasso();
    if (dcVelocidade == 0) {
      pararMotorDC();
    } else {
      digitalWrite(PIN_DC_1A, (dcSentido == 1) ? HIGH : LOW);
      digitalWrite(PIN_DC_2A, (dcSentido == 1) ? LOW : HIGH);
      analogWrite(PIN_DC_EN, dcVelocidade);
    }
  } 
  else if (modo == 2) { // SERVO MOTOR
    pararMotorDC();
    passosRestantes = 0;
    desligarBobinasPasso();
    meuServo.write(servoAngulo);
  } 
  else if (modo == 3) { // MOTOR DE PASSO
    pararMotorDC();
    if (passosRestantes > 0 && (millis() - tempoUltimoPasso >= intervaloPasso)) {
      tempoUltimoPasso = millis();
      motorPasso.step((passoSentido == 1) ? -1 : 1);
      passosRestantes--;
      if (passosRestantes == 0) desligarBobinasPasso();
    }
  }
}
