/*
 * ZoneMonitor Completo: ESP8266 + DHT11 + SNMP + OLED + HTTP
 * Autor: Zer0
 * GitHub: https://github.com/Zer0G0ld/ZoneMonitor
 * Data: 29/10/2025
 *
 * Funcionalidades:
 * - Conecta ao WiFi e mostra status no OLED (IP + MAC)
 * - Lê DHT11 (Temp + Hum) a cada 2s
 * - Exibe dados formatados no OLED
 * - Responde requisições SNMP (OIDs customizados)
 * - HTTP simples para consulta rápida
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <SNMP.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === CONFIG Wi-Fi ===
const char* ssid = "SSID";
const char* password = "SENHA";

// === SENSOR ===
#define DHTPIN D6
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// === SNMP ===
WiFiUDP udp;
SNMP::Agent snmp;
#define OID_TEMPERATURA "1.3.6.1.4.1.4976.1.1.0"
#define OID_UMIDADE     "1.3.6.1.4.1.4976.1.2.0"

// === DISPLAY OLED ===
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// === HTTP SERVER ===
ESP8266WebServer server(80);

// === VARIÁVEIS GLOBAIS ===
float tempAtual = 0;
float humAtual  = 0;
String ipLocal  = "0.0.0.0";
String macLocal = "";

// ===== SNMP HANDLER =====
void onMessage(const SNMP::Message* message, const IPAddress remote, const uint16_t port) {
  SNMP::VarBindList* varbindlist = message->getVarBindList();
  SNMP::Message* response = new SNMP::Message(SNMP::Version::V2C, "public", SNMP::Type::GetResponse);
  response->setRequestID(message->getRequestID());

  for (unsigned int i = 0; i < varbindlist->count(); ++i) {
    SNMP::VarBind* varbind = (*varbindlist)[i];
    const char* oid = varbind->getName();

    if (strcmp(oid, OID_TEMPERATURA) == 0)
      response->add(OID_TEMPERATURA, new SNMP::OpaqueFloatBER(tempAtual));
    else if (strcmp(oid, OID_UMIDADE) == 0)
      response->add(OID_UMIDADE, new SNMP::OpaqueFloatBER(humAtual));
  }

  snmp.send(response, remote, port);
  delete response;
}

// ===== HTTP HANDLER =====
void handleRoot() {
  String html = "<html><body style='font-family:monospace'>";
  html += "<h2>ZoneMonitor CPD</h2>";
  html += "<p><b>Temperatura:</b> " + String(tempAtual,1) + " °C</p>";
  html += "<p><b>Umidade:</b> " + String(humAtual,1) + " %</p>";
  html += "<p><b>IP:</b> " + ipLocal + "</p>";
  html += "<p><b>MAC:</b> " + macLocal + "</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// ===== FUNÇÃO PARA ATUALIZAR DISPLAY =====
void atualizarDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (isnan(tempAtual) || isnan(humAtual)) {
    display.setTextSize(1);
    display.setCursor(0,25);
    display.println("Erro sensor...");
    Serial.println("Erro ao ler DHT11!");
  } else {
    // Temperatura (maior)
    display.setTextSize(2);
    display.setCursor(10, 0);
    display.printf("%.1fC", tempAtual);

    // Umidade (média)
    display.setTextSize(1);
    display.setCursor(10, 25);
    display.printf("Umid: %.1f%%", humAtual);

    // Linha de status (IP e MAC)
    display.setTextSize(1);
    display.setCursor(0, 45);
    display.print("IP: ");
    display.println(ipLocal);
    display.setCursor(0, 55);
    display.print("MAC:");
    display.println(macLocal);
  }

  display.display();
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(200);
  dht.begin();

  // OLED
  Wire.begin(D2, D1);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Falha OLED!");
    while(true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("ZoneMonitor");
  display.println("Conectando WiFi...");
  display.display();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int tentativas = 0;
  while(WiFi.status() != WL_CONNECTED && tentativas < 40) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
    tentativas++;
  }

  if(WiFi.status() == WL_CONNECTED){
    ipLocal = WiFi.localIP().toString();
    macLocal = WiFi.macAddress();

    Serial.println("\nWiFi conectado!");
    Serial.println("IP: " + ipLocal);
    Serial.println("MAC: " + macLocal);

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("WiFi conectado!");
    display.print("IP: "); display.println(ipLocal);
    display.print("MAC: "); display.println(macLocal);
    display.display();
    delay(2500);
  } else {
    display.clearDisplay();
    display.println("Falha WiFi!");
    display.display();
  }

  // SNMP + HTTP
  snmp.begin(udp);
  snmp.onMessage(onMessage);
  server.on("/", handleRoot);
  server.begin();

  Serial.println("SNMP ativo.");
  Serial.println("HTTP ativo.");
}

// ===== LOOP =====
void loop() {
  snmp.loop();
  server.handleClient();

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();
    tempAtual = dht.readTemperature();
    humAtual  = dht.readHumidity();
    Serial.printf("Temp: %.1f°C Hum: %.1f%%\n", tempAtual, humAtual);
    atualizarDisplay();
  }
}
