#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <Adafruit_INA219.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// =======================
// CONFIGURAÇÕES DE WI-FI E THINGSBOARD
// =======================

const char* ssids[] = {
  "EXEMPLO"
};

const char* passwords[] = {
  "123456"
};

const int numRedes = 1;

const char* thingsboardServer = "thingsboard.cloud";
const int mqttPort = 1883;
const char* mqttUser = "EXEMPLO";

WiFiClient espClient;
PubSubClient client(espClient);

// =======================
// SENSORES
// =======================

Adafruit_INA219 ina219(0x44);

#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

#define LDR_PIN 34

// =======================
// NORMALIZACAO MIN-MAX
// =======================

const int ADC_ESCURO_FIXO = 373;
const int ADC_CLARO_FIXO  = 2435;

int normalizarLdrFixo(int adcBruto) {
  long norm = (long)(adcBruto - ADC_ESCURO_FIXO) * 1000 /
              (ADC_CLARO_FIXO - ADC_ESCURO_FIXO);
  return constrain((int)norm, 0, 1000);
}

const unsigned long intervaloEnvio = 10000; // 10 segundos
unsigned long ultimoEnvio = 0;

bool inaOk = false;

// =======================
// FUNCOES WIFI / MQTT
// =======================

void conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println();
  Serial.println("Tentando conectar ao Wi-Fi...");

  WiFi.mode(WIFI_STA);

  while (WiFi.status() != WL_CONNECTED) {
    for (int i = 0; i < numRedes; i++) {
      Serial.print("Tentando rede: ");
      Serial.println(ssids[i]);
      WiFi.begin(ssids[i], passwords[i]);
      unsigned long inicioTentativa = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - inicioTentativa < 10000) {
        delay(500);
        Serial.print(".");
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.println("Wi-Fi conectado!");
        Serial.print("Rede conectada: ");
        Serial.println(ssids[i]);
        Serial.print("IP do ESP32: ");
        Serial.println(WiFi.localIP());
        return;
      }
      Serial.println();
      Serial.println("Não conectou nessa rede.");
      WiFi.disconnect(true);
      delay(1000);
    }
    Serial.println("Nenhuma rede conectou. Tentando novamente em 5 segundos...");
    delay(5000);
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    conectarWiFi();
    Serial.print("Conectando ao ThingsBoard via MQTT... ");
    String clientId = "ESP32_Gerador_Fixo_";
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

// =======================
// SETUP
// =======================

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=== GERADOR FIXO - SENSORES + WIFI + MQTT ===");
  Serial.println();
  Wire.begin();
  sensors.begin();

  if (!ina219.begin()) {
    Serial.println("ERRO: INA219 não encontrado!");
    inaOk = false;
  } else {
    Serial.println("OK: INA219 encontrado.");
    ina219.setCalibration_16V_400mA();
    inaOk = true;
  }

  conectarWiFi();
  client.setServer(thingsboardServer, mqttPort);
  Serial.println();
  Serial.println("Sistema iniciado. Enviando dados ao ThingsBoard a cada 10 segundos.");
  Serial.println();
}

// =======================
// LOOP
// =======================

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi caiu. Reconectando...");
    conectarWiFi();
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  unsigned long agora = millis();

  if (agora - ultimoEnvio >= intervaloEnvio) {
    ultimoEnvio = agora;

    // =======================
    // LEITURA DE SENSORES
    // =======================
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);

    bool tempOk = (tempC > -100);

        int ldrValue_bruto = analogRead(LDR_PIN);

    int ldrNorm = normalizarLdrFixo(ldrValue_bruto);

        float v = 0.0;
    float i = 0.0;
    float p = 0.0;

    if (inaOk) {
      v = ina219.getBusVoltage_V();
      i = ina219.getCurrent_mA();
      p = v * i; // V * mA = mW
    }

    // =======================
    // SERIAL MONITOR
    // =======================
    Serial.println("-------------------------");
    Serial.printf("Wi-Fi:            %s\n", WiFi.SSID().c_str());
    Serial.printf("INA219:           V=%.2f V | I=%.2f mA\n", v, i);
    Serial.printf("Potencia:         %.2f mW\n", p);

    if (tempOk) {
      Serial.printf("Temperatura:      %.2f C\n", tempC);
    } else {
      Serial.println("Temperatura:      ERRO DS18B20");
    }

    Serial.printf("LDR bruto:        %d\n", ldrValue_bruto);
    Serial.printf("LDR normalizado:  %d (0-1000)\n", ldrNorm);
    Serial.println("-------------------------");

    // =======================
    // PAYLOAD JSON
    // =======================

    String payload = "{";
    payload += "\"tensao_gfixo\":" + String(v, 2);
    payload += ",\"corrente_gfixo\":" + String(i, 2);
    payload += ",\"potencia_gfixo\":" + String(p, 2);

    if (tempOk) {
      payload += ",\"temperatura_gfixo\":" + String(tempC, 2);
    }

    payload += ",\"ldr_bruto_gfixo\":" + String(ldrValue_bruto);
    payload += ",\"wifi_conectado_gfixo\":" + String(WiFi.status() == WL_CONNECTED ? 1 : 0);
    payload += ",\"mqtt_conectado_gfixo\":" + String(client.connected() ? 1 : 0);
    payload += ",\"wifi_rssi_gfixo\":" + String(WiFi.RSSI());
    payload += "}";

    Serial.print("Payload enviado: ");
    Serial.println(payload);

    bool publicado = client.publish("v1/devices/me/telemetry", payload.c_str());

    if (publicado) {
      Serial.println("Status MQTT: publicado com sucesso.");
    } else {
      Serial.println("Status MQTT: falha ao publicar.");
    }

    Serial.println();
  }
}