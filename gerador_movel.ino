#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <math.h>

// ============================================================
//  CONFIGURACOES DE WI-FI
// ============================================================
const char* ssids[] = {
  "Budweiser"
};

const char* passwords[] = {
  "Aa25101208"
};

const int numRedes = 1;

// ============================================================
//  CONFIGURACOES DO THINGSBOARD
// ============================================================
const char* thingsboardServer = "thingsboard.cloud";
const int mqttPort = 1883;

const char* mqttUser = "HdtR8SDiJp8zGj0RhhgT";

WiFiClient espClient;
PubSubClient client(espClient);

// ============================================================
//  HORARIO VIA NTP
// ============================================================
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000);

const bool USAR_RESTRICAO_HORARIO = true;
const int MINUTO_INICIO_RASTREIO = 7 * 60 + 30;
const int MINUTO_FIM_RASTREIO    = 16 * 60;

// ============================================================
//  INA219 - PLACA MOVEL
// ============================================================
Adafruit_INA219 ina219(0x40);
bool inaOk = false;

float vbus = 0.0;
float corrente_mA = 0.0;
float potencia_mW = 0.0;

// ============================================================
//  DS18B20
// ============================================================
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float tempC = 0.0;
bool tempOk = false;

// ============================================================
//  LDRs - LEITURA BRUTA E NORMALIZACAO MIN-MAX
// ============================================================
const int LDR_PINS[4] = {34, 35, 36, 39};
const int N_AMOSTRAS_LDR = 32;

int adcLdr[4] = {0, 0, 0, 0};

const int ADC_ESCURO_LDR[4] = {300, 360, 130, 370};
const int ADC_CLARO_LDR[4]  = {2200, 2860, 2200, 2350};

float adcNormLdr[4] = {0, 0, 0, 0};

float normMediaSuperior = 0.0;
float normMediaInferior = 0.0;
float normMediaEsquerda = 0.0;
float normMediaDireita = 0.0;
float normMedio = 0.0;

float razaoVertical = 1.0;
float razaoHorizontal = 1.0;

// ============================================================
//  SERVO - EIXO DE INCLINACAO / TILT
// ============================================================
const int SERVO_PIN = 14;
Servo servoTilt;

const int SERVO_MIN_GRAUS = 12;
const int SERVO_MAX_GRAUS = 57;
const int SERVO_REPOUSO_GRAUS = 22;
const int PASSO_SERVO_GRAUS = 2;

int servoAtualGraus = SERVO_REPOUSO_GRAUS;
bool servoAnexado = false;

const bool INVERTER_DIRECAO_SERVO = false;

// ============================================================
//  MOTOR DE PASSO - EIXO AZIMUTAL
// ============================================================
const int IN1 = 25;
const int IN2 = 26;
const int IN3 = 27;
const int IN4 = 33;

const int sequencia[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

int passoAtualSequencia = 0;
long posicaoPassos = 0;

const int PASSOS_POR_VOLTA = 4096;
const int DELAY_PASSO_MS = 4;

const float AZIMUTE_MIN_GRAUS = -86.0;
const float AZIMUTE_MAX_GRAUS = 100.0;
const float AZIMUTE_REPOUSO_GRAUS = 0.0;
const float PASSO_AZIMUTE_GRAUS = 5.0;

const bool INVERTER_DIRECAO_AZIMUTE = true;

// ============================================================
//  LOGICA DE RASTREAMENTO / COLETA
// ============================================================
const bool MOVIMENTACAO_PAINEL_ATIVA = true;
const bool COMANDAR_SERVO_NO_SETUP = false;

const float TOLERANCIA_RAZAO = 1.08;
const float NORM_MIN_RASTREIO = 100.0;
const int ADC_SATURACAO_LDR = 4000;
const int QTD_LDR_SATURADOS_BLOQUEIO = 3;

const unsigned long INTERVALO_TELEMETRIA_MS = 10000UL;
const unsigned long INTERVALO_RASTREIO_MS   = 10000UL;

unsigned long ultimoEnvio = 0;
unsigned long ultimoRastreio = 0;

int qtdLdrSaturados = 0;
bool ldrSaturado = false;

enum EstadoRastreador {
  REPOUSO,
  AGUARDANDO_CORRECAO,
  CORRIGINDO,
  COLETA_FIXA
};

EstadoRastreador estado = AGUARDANDO_CORRECAO;

// ============================================================
//  FUNCOES AUXILIARES - ESTADO / HORARIO
// ============================================================
const char* estadoTexto() {
  if (!MOVIMENTACAO_PAINEL_ATIVA) {
    return "COLETA_FIXA";
  }

  if (ldrSaturado) {
    return "LDR_SATURADO";
  }

  switch (estado) {
    case REPOUSO: return "REPOUSO";
    case CORRIGINDO: return "CORRIGINDO";
    case COLETA_FIXA: return "COLETA_FIXA";
    case AGUARDANDO_CORRECAO:
    default: return "AGUARDANDO";
  }
}

bool dentroPeriodoDiurno() {
  if (!USAR_RESTRICAO_HORARIO) {
    return true;
  }

  timeClient.update();
  int minutosAgora = timeClient.getHours() * 60 + timeClient.getMinutes();
  return (minutosAgora >= MINUTO_INICIO_RASTREIO && minutosAgora < MINUTO_FIM_RASTREIO);
}

// ============================================================
//  FUNCOES AUXILIARES - WIFI / MQTT
// ============================================================
void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println();
  Serial.println("Tentando conectar ao Wi-Fi...");
  WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    for (int i = 0; i < numRedes; i++) {
      Serial.print("Tentando rede: ");
      Serial.println(ssids[i]);

      WiFi.begin(ssids[i], passwords[i]);
      unsigned long inicioTentativa = millis();

      while (WiFi.status() != WL_CONNECTED && millis() - inicioTentativa < 10000UL) {
        delay(500);
        Serial.print(".");
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("Wi-Fi conectado!");
        Serial.print("IP do ESP32: ");
        Serial.println(WiFi.localIP());
        return;
      }

      Serial.println();
      Serial.println("Nao conectou nessa rede.");
      WiFi.disconnect(true);
      delay(1000);
    }

    Serial.println("Nenhuma rede conectou. Tentando novamente em 5 segundos...");
    delay(5000);
  }
}

void conectarMQTT() {
  while (!client.connected()) {
    conectarWiFi();

    Serial.print("Conectando ao ThingsBoard via MQTT... ");

    String clientId = "ESP32_Gerador_Movel_";
    clientId += String((uint32_t)ESP.getEfuseMac(), HEX);

    if (client.connect(clientId.c_str(), mqttUser, NULL)) {
      Serial.println("conectado!");
    } else {
      Serial.print("falha, rc=");
      Serial.print(client.state());
      Serial.println(" | tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void manterConexoes() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi caiu. Reconectando...");
    conectarWiFi();
  }

  if (!client.connected()) {
    conectarMQTT();
  }

  client.loop();
}

// ============================================================
//  FUNCOES AUXILIARES - SENSORES
// ============================================================
void lerLDR(int indice, int &adc_medio) {
  long soma = 0;

  for (int k = 0; k < N_AMOSTRAS_LDR; k++) {
    soma += analogRead(LDR_PINS[indice]);
    delayMicroseconds(200);
  }

  adc_medio = soma / N_AMOSTRAS_LDR;
}

float normalizarAdcLdr(int indice, int adcBruto) {
  float denominador = ADC_CLARO_LDR[indice] - ADC_ESCURO_LDR[indice];

  if (denominador <= 0.0) {
    return 0.0;
  }

  float normalizado = (adcBruto - ADC_ESCURO_LDR[indice]) * 1000.0 / denominador;
  return constrain(normalizado, 0.0, 1000.0);
}

void calcularMediasLDR() {
  for (int n = 0; n < 4; n++) {
    adcNormLdr[n] = normalizarAdcLdr(n, adcLdr[n]);
  }

  normMediaSuperior = (adcNormLdr[0] + adcNormLdr[1]) / 2.0;
  normMediaInferior = (adcNormLdr[2] + adcNormLdr[3]) / 2.0;
  normMediaEsquerda = (adcNormLdr[0] + adcNormLdr[2]) / 2.0;
  normMediaDireita  = (adcNormLdr[1] + adcNormLdr[3]) / 2.0;
  normMedio = (adcNormLdr[0] + adcNormLdr[1] + adcNormLdr[2] + adcNormLdr[3]) / 4.0;

  float menorV = min(normMediaSuperior, normMediaInferior);
  float maiorV = max(normMediaSuperior, normMediaInferior);
  if (maiorV <= 0.001) {
    razaoVertical = 1.0;
  } else if (menorV <= 0.001) {
    razaoVertical = 999.0;
  } else {
    razaoVertical = maiorV / menorV;
  }

  float menorH = min(normMediaEsquerda, normMediaDireita);
  float maiorH = max(normMediaEsquerda, normMediaDireita);
  if (maiorH <= 0.001) {
    razaoHorizontal = 1.0;
  } else if (menorH <= 0.001) {
    razaoHorizontal = 999.0;
  } else {
    razaoHorizontal = maiorH / menorH;
  }
}

int contarLdrsSaturados() {
  int quantidade = 0;

  for (int k = 0; k < 4; k++) {
    if (adcLdr[k] >= ADC_SATURACAO_LDR) {
      quantidade++;
    }
  }

  return quantidade;
}

void atualizarSensores() {
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);
  tempOk = (tempC > -100);

  for (int n = 0; n < 4; n++) {
    lerLDR(n, adcLdr[n]);
  }
  calcularMediasLDR();
  qtdLdrSaturados = contarLdrsSaturados();
  ldrSaturado = (qtdLdrSaturados >= QTD_LDR_SATURADOS_BLOQUEIO);

  if (inaOk) {
    vbus = ina219.getBusVoltage_V();
    corrente_mA = ina219.getCurrent_mA();
    potencia_mW = vbus * corrente_mA;
  } else {
    vbus = 0.0;
    corrente_mA = 0.0;
    potencia_mW = 0.0;
  }
}

// ============================================================
//  FUNCOES AUXILIARES - MOTOR DE PASSO
// ============================================================
void aplicarPasso(int indice) {
  digitalWrite(IN1, sequencia[indice][0]);
  digitalWrite(IN2, sequencia[indice][1]);
  digitalWrite(IN3, sequencia[indice][2]);
  digitalWrite(IN4, sequencia[indice][3]);
}

void desligarBobinas() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void moverPassos(long quantidade) {
  if (!MOVIMENTACAO_PAINEL_ATIVA) {
    desligarBobinas();
    return;
  }

  if (quantidade == 0) {
    desligarBobinas();
    return;
  }

  int direcao = (quantidade > 0) ? 1 : -1;
  quantidade = labs(quantidade);

  for (long i = 0; i < quantidade; i++) {
    passoAtualSequencia += direcao;

    if (passoAtualSequencia > 7) passoAtualSequencia = 0;
    if (passoAtualSequencia < 0) passoAtualSequencia = 7;

    aplicarPasso(passoAtualSequencia);
    posicaoPassos += direcao;

    delay(DELAY_PASSO_MS);
  }

  desligarBobinas();
}

float azimuteAtualGraus() {
  return (posicaoPassos * 360.0) / PASSOS_POR_VOLTA;
}

void moverAzimutePara(float alvoGraus) {
  if (!MOVIMENTACAO_PAINEL_ATIVA) {
    desligarBobinas();
    return;
  }

  alvoGraus = constrain(alvoGraus, AZIMUTE_MIN_GRAUS, AZIMUTE_MAX_GRAUS);

  long alvoPassos = lround((alvoGraus * PASSOS_POR_VOLTA) / 360.0);
  long deltaPassos = alvoPassos - posicaoPassos;

  moverPassos(deltaPassos);

  Serial.print("Azimute atual: ");
  Serial.print(azimuteAtualGraus(), 2);
  Serial.println(" graus");
}

// ============================================================
//  FUNCOES AUXILIARES - SERVO
// ============================================================
void anexarServoSeNecessario() {
  if (!servoAnexado) {
    servoTilt.attach(SERVO_PIN);
    servoAnexado = true;
    delay(50);
  }
}

void moverServoPara(int alvoGraus) {
  if (!MOVIMENTACAO_PAINEL_ATIVA) {
    return;
  }

  alvoGraus = constrain(alvoGraus, SERVO_MIN_GRAUS, SERVO_MAX_GRAUS);

  anexarServoSeNecessario();

  if (alvoGraus == servoAtualGraus) {
    servoTilt.write(servoAtualGraus);
    return;
  }

  int direcao = (alvoGraus > servoAtualGraus) ? 1 : -1;

  while (servoAtualGraus != alvoGraus) {
    servoAtualGraus += direcao;
    servoTilt.write(servoAtualGraus);
    delay(20);
  }

  Serial.print("Servo tilt atual: ");
  Serial.print(servoAtualGraus);
  Serial.println(" graus");
}

void retornarRepouso() {
  if (!MOVIMENTACAO_PAINEL_ATIVA) {
    desligarBobinas();
    return;
  }

  Serial.println("Retornando para posicao de repouso...");
  moverServoPara(SERVO_REPOUSO_GRAUS);
  moverAzimutePara(AZIMUTE_REPOUSO_GRAUS);
}

// ============================================================
//  LOGICA DE RASTREAMENTO DIFERENCIAL POR ADC NORMALIZADO
// ============================================================
void corrigirServoPorLDR() {
  if (razaoVertical <= TOLERANCIA_RAZAO) return;

  int sinalServo;

  if (normMediaSuperior > normMediaInferior) {
    sinalServo = -1;
  } else {
    sinalServo = +1;
  }

  if (INVERTER_DIRECAO_SERVO) sinalServo *= -1;

  int novoServo = servoAtualGraus + sinalServo * PASSO_SERVO_GRAUS;
  novoServo = constrain(novoServo, SERVO_MIN_GRAUS, SERVO_MAX_GRAUS);

  if (novoServo != servoAtualGraus) {
    moverServoPara(novoServo);
  }
}

void corrigirAzimutePorLDR() {
  if (razaoHorizontal <= TOLERANCIA_RAZAO) return;

  int sinalAzimute;

  if (normMediaEsquerda > normMediaDireita) {
    sinalAzimute = -1;
  } else {
    sinalAzimute = +1;
  }

  if (INVERTER_DIRECAO_AZIMUTE) sinalAzimute *= -1;

  float novoAzimute = azimuteAtualGraus() + sinalAzimute * PASSO_AZIMUTE_GRAUS;
  novoAzimute = constrain(novoAzimute, AZIMUTE_MIN_GRAUS, AZIMUTE_MAX_GRAUS);

  if (fabs(novoAzimute - azimuteAtualGraus()) > 0.01) {
    moverAzimutePara(novoAzimute);
  }
}

void executarCicloDeRastreamento() {
  qtdLdrSaturados = contarLdrsSaturados();
  ldrSaturado = (qtdLdrSaturados >= QTD_LDR_SATURADOS_BLOQUEIO);

  if (!MOVIMENTACAO_PAINEL_ATIVA) {
    estado = COLETA_FIXA;
    desligarBobinas();
    return;
  }

  if (!dentroPeriodoDiurno()) {
    if (estado != REPOUSO) {
      estado = REPOUSO;
      retornarRepouso();
      Serial.println("Fora da janela diurna. Sistema em REPOUSO.");
    }
    return;
  }

  if (estado == REPOUSO) {
    estado = AGUARDANDO_CORRECAO;
    Serial.println("Entrou na janela diurna. Aguardando proxima correcao.");
  }

  if (normMedio < NORM_MIN_RASTREIO) {
    estado = AGUARDANDO_CORRECAO;
    ldrSaturado = false;
    Serial.println("Pouca leitura normalizada nos LDRs. Posicao mantida.");
    return;
  }

  if (qtdLdrSaturados >= QTD_LDR_SATURADOS_BLOQUEIO) {
    estado = AGUARDANDO_CORRECAO;
    ldrSaturado = true;
    Serial.print("LDRs saturados: ");
    Serial.print(qtdLdrSaturados);
    Serial.println(". Posicao mantida.");
    return;
  }

  estado = CORRIGINDO;

  corrigirServoPorLDR();
  corrigirAzimutePorLDR();

  estado = AGUARDANDO_CORRECAO;

  Serial.println("=== CORRECAO DE RASTREAMENTO ===");
  Serial.printf("Norm Sup=%.1f | Inf=%.1f | Esq=%.1f | Dir=%.1f | Medio=%.1f\n",
                normMediaSuperior, normMediaInferior, normMediaEsquerda, normMediaDireita, normMedio);
  Serial.printf("Razao Vertical=%.3f | Razao Horizontal=%.3f\n", razaoVertical, razaoHorizontal);
  Serial.printf("Servo=%d graus | Azimute=%.2f graus | Estado=%s\n",
                servoAtualGraus, azimuteAtualGraus(), estadoTexto());
}

// ============================================================
//  TELEMETRIA
// ============================================================
void enviarTelemetria() {
  String payload = "{";

  payload += "\"tensao_gmovel\":" + String(vbus, 2);
  payload += ",\"corrente_gmovel\":" + String(corrente_mA, 2);
  payload += ",\"potencia_gmovel\":" + String(potencia_mW, 2);

  if (tempOk) {
    payload += ",\"temperatura_gmovel\":" + String(tempC, 2);
  }

  payload += ",\"ldr1_adc_gmovel\":" + String(adcLdr[0]);
  payload += ",\"ldr2_adc_gmovel\":" + String(adcLdr[1]);
  payload += ",\"ldr3_adc_gmovel\":" + String(adcLdr[2]);
  payload += ",\"ldr4_adc_gmovel\":" + String(adcLdr[3]);

  payload += ",\"servo_tilt_gmovel\":" + String(servoAtualGraus);
  payload += ",\"azimute_gmovel\":" + String(azimuteAtualGraus(), 2);

  payload += ",\"wifi_conectado_gmovel\":" + String(WiFi.status() == WL_CONNECTED ? 1 : 0);
  payload += ",\"mqtt_conectado_gmovel\":" + String(client.connected() ? 1 : 0);
  payload += ",\"wifi_rssi_gmovel\":" + String(WiFi.RSSI());

  payload += "}";

  Serial.print("Payload enviado: ");
  Serial.println(payload);

  bool publicado = client.publish("v1/devices/me/telemetry", payload.c_str());

  if (publicado) {
    Serial.println("Status MQTT: publicado com sucesso.");
  } else {
    Serial.println("Status MQTT: falha ao publicar.");
  }
}

void imprimirStatusSerial() {
  Serial.println("-------------------------");
  Serial.printf("Hora NTP:        %02d:%02d:%02d\n", timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
  Serial.printf("Estado:          %s | RSSI=%d dBm\n", estadoTexto(), WiFi.RSSI());
  Serial.printf("INA219:          V=%.2f V | I=%.2f mA | P=%.2f mW\n", vbus, corrente_mA, potencia_mW);

  if (tempOk) {
    Serial.printf("Temperatura:     %.2f C\n", tempC);
  } else {
    Serial.println("Temperatura:     ERRO DS18B20");
  }

  for (int n = 0; n < 4; n++) {
    Serial.printf("LDR%d GPIO%2d: ADC=%4d | Norm=%.1f/1000\n",
                  n + 1, LDR_PINS[n], adcLdr[n], adcNormLdr[n]);
  }

  Serial.printf("LDRs saturados: %d | Bloqueio: %s\n",
                qtdLdrSaturados, ldrSaturado ? "SIM" : "NAO");
  Serial.printf("Norm Sup=%.1f | Inf=%.1f | Esq=%.1f | Dir=%.1f | Medio=%.1f\n",
                normMediaSuperior, normMediaInferior, normMediaEsquerda, normMediaDireita, normMedio);
  Serial.printf("Servo=%d graus | Azimute=%.2f graus\n",
                servoAtualGraus, azimuteAtualGraus());
  Serial.println("-------------------------");
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=== GERADOR MOVEL - RASTREAMENTO MIN-MAX + WIFI + MQTT ===");
  Serial.println("ATENCAO: antes de energizar, deixe mecanicamente o azimute em 0 grau.");
  Serial.println();

  Wire.begin();
  sensors.begin();

  if (!ina219.begin()) {
    Serial.println("ERRO: INA219 nao encontrado! O restante do sistema continuara funcionando.");
    inaOk = false;
  } else {
    Serial.println("OK: INA219 encontrado.");
    ina219.setCalibration_16V_400mA();
    inaOk = true;
  }

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  desligarBobinas();

  servoAtualGraus = SERVO_REPOUSO_GRAUS;
  if (COMANDAR_SERVO_NO_SETUP) {
    anexarServoSeNecessario();
    servoTilt.write(servoAtualGraus);
    delay(500);
  }

  posicaoPassos = 0;
  desligarBobinas();

  conectarWiFi();
  timeClient.begin();
  timeClient.update();

  client.setServer(thingsboardServer, mqttPort);
  client.setBufferSize(2048);
  conectarMQTT();

  if (!MOVIMENTACAO_PAINEL_ATIVA) {
    estado = COLETA_FIXA;
    desligarBobinas();
  } else if (USAR_RESTRICAO_HORARIO && !dentroPeriodoDiurno()) {
    estado = REPOUSO;
    retornarRepouso();
  } else {
    estado = AGUARDANDO_CORRECAO;
  }

  Serial.println();
  Serial.println("Sistema iniciado.");
  Serial.println("Telemetria a cada 10 s.");
  if (MOVIMENTACAO_PAINEL_ATIVA) {
    Serial.println("Correcao diferencial a cada 10 s.");
  } else {
    Serial.println("Movimentacao desativada: painel fixo, apenas telemetria.");
  }
  Serial.println();

  atualizarSensores();
  imprimirStatusSerial();
  enviarTelemetria();
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  manterConexoes();
  timeClient.update();

  unsigned long agora = millis();

  if (agora - ultimoRastreio >= INTERVALO_RASTREIO_MS) {
    ultimoRastreio = agora;
    atualizarSensores();
    executarCicloDeRastreamento();
  }

  if (agora - ultimoEnvio >= INTERVALO_TELEMETRIA_MS) {
    ultimoEnvio = agora;
    atualizarSensores();
    imprimirStatusSerial();
    enviarTelemetria();
    Serial.println();
  }
}
