#include <Wire.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <Stepper.h>

Adafruit_INA219 ina219(0x40);

#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define LDR_PIN 34
const int minLdrValue = 2500;
const int maxLdrValue = 4000;

const int SERVO_PIN = 14;
Servo servo;

const int STEPS_PER_REV = 2048;
const int PIN_IN1 = 25;
const int PIN_IN2 = 26;
const int PIN_IN3 = 27;
const int PIN_IN4 = 33;
Stepper stepperMotor(STEPS_PER_REV, PIN_IN1, PIN_IN3, PIN_IN2, PIN_IN4);

// =================== CONTROLE ===================
bool inaOK = false;
int ciclo = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("==========================================");
  Serial.println("   TESTE COMPLETO - PLACA MOVEL ESP32");
  Serial.println("==========================================");
  Serial.println();

  Wire.begin(21, 22);
  if (!ina219.begin()) {
    Serial.println("[INA219]  ERRO: nao encontrado no endereco 0x40!");
  } else {
    Serial.println("[INA219]  OK: encontrado no endereco 0x40.");
    ina219.setCalibration_16V_400mA();
    inaOK = true;
  }

  sensors.begin();
  int count = sensors.getDeviceCount();
  if (count == 0) {
    Serial.println("[DS18B20] ERRO: nenhum sensor detectado!");
  } else {
    Serial.printf("[DS18B20] OK: %d sensor(es) encontrado(s).\n", count);
  }

  Serial.printf("[LDR]     Pino: GPIO%d\n", LDR_PIN);

  servo.attach(SERVO_PIN);
  servo.write(20);
  Serial.printf("[SERVO]   Pino: GPIO%d | Posicao inicial: 20 graus\n", SERVO_PIN);

  stepperMotor.setSpeed(10);
  Serial.printf("[STEPPER] Pinos: IN1=%d, IN2=%d, IN3=%d, IN4=%d\n", PIN_IN1, PIN_IN2, PIN_IN3, PIN_IN4);

  Serial.println();
  Serial.println("Iniciando ciclo de testes em 3 segundos...");
  Serial.println();
  delay(3000);
}

void loop() {
  ++;ciclo
  Serial.println("==========================================");
  Serial.printf("            CICLO DE TESTE #%d\n", ciclo);
  Serial.println("==========================================");

  // -------- LEITURA DOS SENSORES --------
  Serial.println();
  Serial.println("--- SENSORES ---");

  if (inaOK) {
    float tensao = ina219.getBusVoltage_V();
    float corrente = ina219.getCurrent_mA();
    float potencia = tensao * corrente;
    Serial.printf("[INA219]  Tensao: %.2f V | Corrente: %.2f mA | Potencia: %.2f mW\n", tensao, corrente, potencia);
  } else {
    Serial.println("[INA219]  Sensor nao inicializado.");
  }

  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  if (temp == -127.0) {
    Serial.println("[DS18B20] ERRO: leitura invalida (-127).");
  } else {
    Serial.printf("[DS18B20] Temperatura: %.2f C\n", temp);
  }

  int ldrBruto = analogRead(LDR_PIN);
  float luminosidade = map(ldrBruto, minLdrValue, maxLdrValue, 0, 100);
  luminosidade = constrain(luminosidade, 0, 100);
  Serial.printf("[LDR]     Bruto: %d | Luminosidade: %.0f%%\n", ldrBruto, luminosidade);

  // -------- TESTE DO SERVO --------
  Serial.println();
  Serial.println("--- SERVO ---");
  Serial.println("Varrendo de 20 a 50 graus...");
  for (int pos = 20; pos <= 50; pos += 10) {
    servo.write(pos);
    Serial.printf("  Servo: %d graus\n", pos);
    delay(500);
  }
  Serial.println("Retornando a 20 graus...");
  for (int pos = 50; pos >= 20; pos -= 10) {
    servo.write(pos);
    Serial.printf("  Servo: %d graus\n", pos);
    delay(500);
  }

  // -------- TESTE DO MOTOR DE PASSO --------
  Serial.println();
  Serial.println("--- MOTOR DE PASSO ---");
  int steps90 = STEPS_PER_REV / 4;
  Serial.println("Girando 90 graus (sentido horario)...");
  stepperMotor.step(steps90);
  Serial.println("Concluido.");
  delay(1000);
  Serial.println("Retornando 90 graus (sentido anti-horario)...");
  stepperMotor.step(-steps90);
  Serial.println("Concluido.");
  Serial.println();
  Serial.println("Proximo ciclo em 5 segundos...");
  Serial.println();
  delay(5000);
}