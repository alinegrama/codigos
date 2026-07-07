#include <Wire.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>

Adafruit_INA219 ina219(0x40);

#define ONE_WIRE_BUS 4 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define LDR_PIN 34 
const int minLdrValue = 2500;
const int maxLdrValue = 4000;

void setup() {
  Serial.begin(115200);
  delay(3000);
  Serial.println("==========================================");
  Serial.println("   TESTE COMPLETO - PLACA FIXA ESP32");
  Serial.println("==========================================");
  Serial.println();
  Wire.begin();
  sensors.begin();
  if (!ina219.begin()) {
    Serial.println("ERRO: INA219 não encontrado!");
  } else {
    Serial.println("OK: INA219 encontrado.");
    ina219.setCalibration_16V_400mA();
  }
  Serial.println();
  Serial.println("Iniciando leituras a cada 3 segundos...");
  Serial.println();
}

void loop() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  int ldrValue_bruto = analogRead(LDR_PIN);
  float luminosidade = map(ldrValue_bruto, minLdrValue, maxLdrValue, 0, 100);
  luminosidade = constrain(luminosidade, 0, 100);

  float v = ina219.getBusVoltage_V();
  float i = ina219.getCurrent_mA();
  float p = v * i;

  Serial.println("-------------------------");
  Serial.printf("INA219:           V=%.2f V | I=%.2f mA\n", v, i);
  Serial.printf("Potencia:         %.2f mW\n", p);
  Serial.printf("Temperatura:      %.2f C\n", tempC);
  Serial.printf("LDR bruto:        %d\n", ldrValue_bruto);
  Serial.printf("Luminosidade:     %.0f%%\n", luminosidade);
  Serial.println("-------------------------");
  Serial.println();
  delay(3000);
}