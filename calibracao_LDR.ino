int pino_ldr1 = 34;  //GPIO 34
int pino_ldr2 = 35;  //GPIO 35
int pino_ldr3 = 36;  //GPIO 36 (VP)
int pino_ldr4 = 39;  //GPIO 39 (VN)

int ldr1, ldr2, ldr3, ldr4;
float tensao1, tensao2, tensao3, tensao4;

void setup() {
  Serial.begin(115200);
}

void loop() {
  ldr1 = analogRead(pino_ldr1);
  ldr2 = analogRead(pino_ldr2);
  ldr3 = analogRead(pino_ldr3);
  ldr4 = analogRead(pino_ldr4);

  tensao1 = (ldr1 / 4095.0) * 3.3;
  tensao2 = (ldr2 / 4095.0) * 3.3;
  tensao3 = (ldr3 / 4095.0) * 3.3;
  tensao4 = (ldr4 / 4095.0) * 3.3;

  Serial.println("================ ADC ================");
  Serial.print("LDR1: ");
  Serial.println(ldr1);
  Serial.print("LDR2: ");
  Serial.println(ldr2);
  Serial.print("LDR3: ");
  Serial.println(ldr3);
  Serial.print("LDR4: ");
  Serial.println(ldr4);

  Serial.println("================ Tensao ================");
  Serial.print("Tensao LDR1: ");
  Serial.println(tensao1);
  Serial.print("Tensao LDR2: ");
  Serial.println(tensao2);
  Serial.print("Tensao LDR3: ");
  Serial.println(tensao3);
  Serial.print("Tensao LDR4: ");
  Serial.println(tensao4);

  Serial.println("");
  String stringValue = String(tensao1) + " " + String(tensao2) + " " +
  String(tensao3) + " " + String(tensao4) + " " +
  String(ldr1) + " " + String(ldr2) + " " +
  String(ldr3) + " " + String(ldr4);
  Serial.println(stringValue);
  Serial.println("==========================================================");
  delay(5000);
}
