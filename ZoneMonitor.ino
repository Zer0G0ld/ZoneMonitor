/*
 * ZoneMonitor CPD v1.2 - ESP8266 + DHT11 + SNMP + OLED + HTTP Moderno
 * Autor: Zer0
 * GitHub: https://github.com/Zer0G0ld/ZoneMonitor
 * Data: 29/10/2025
 *
 * Funcionalidades:
 * - Conecta ao WiFi e exibe IP e MAC no OLED
 * - Lê DHT11 (Temperatura e Umidade) a cada 2s
 * - Exibe dados no OLED com layout limpo
 * - SNMP responde a OIDs customizados (para Zabbix, TheDude etc)
 * - HTTP Dashboard moderno e leve com atualização em tempo real
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

// ===== HTML PRINCIPAL =====
const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='pt-br'>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>ZoneMonitor CPD</title>
  <style>
    body {
      margin: 0;
      font-family: 'Segoe UI', Roboto, system-ui, sans-serif;
      background: #0f172a;
      color: #e2e8f0;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      height: 100vh;
    }
    h2 {
      color: #38bdf8;
      font-weight: 600;
      margin-bottom: 25px;
      text-align: center;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .status-dot {
      width: 10px;
      height: 10px;
      border-radius: 50%;
      background: #22c55e;
      animation: pulseDot 1.5s infinite;
    }
    @keyframes pulseDot {
      0%, 100% { transform: scale(1); opacity: 1; }
      50% { transform: scale(1.4); opacity: 0.6; }
    }
    .dashboard {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 15px;
    }
    .card {
      background: #1e293b;
      border-radius: 14px;
      padding: 20px 30px;
      width: 260px;
      box-shadow: 0 0 12px rgba(0,0,0,0.35);
      text-align: center;
      transition: transform 0.3s ease;
    }
    .card:hover { transform: scale(1.03); }
    .value {
      font-size: 2.3em;
      font-weight: 600;
      margin: 5px 0;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 8px;
    }
    .temp svg { fill: #ef4444; animation: pulse 2s infinite; }
    .hum svg { fill: #3b82f6; animation: pulse 2s infinite; }
    @keyframes pulse {
      0%, 100% { opacity: 1; transform: scale(1); }
      50% { opacity: 0.6; transform: scale(1.05); }
    }
    .info {
      margin-top: 8px;
      font-size: 0.9em;
      color: #94a3b8;
    }
    .footer {
      margin-top: 15px;
      font-size: 0.8em;
      color: #64748b;
      text-align: center;
    }
    .divider {
      height: 1px;
      width: 80%;
      background: #334155;
      margin: 10px 0;
    }
  </style>
</head>
<body>
  <h2>ZoneMonitor CPD <span class="status-dot"></span></h2>
  <div class='dashboard'>
    <div class='card'>
      <div class='value temp'>
        <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path d="M17 10V5a5 5 0 0 0-10 0v5a7 7 0 1 0 10 0Z"/></svg>
        <span id='temp'>%TEMP%</span>°C
      </div>
      <div class='divider'></div>
      <div class='value hum'>
        <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path d="M12 2.69l-.94.94C7.33 7.36 5 10.68 5 14a7 7 0 0 0 14 0c0-3.32-2.33-6.64-6.06-10.37Z"/></svg>
        <span id='hum'>%HUM%</span>%
      </div>
      <div class='info'>
        <p>IP: <b>%IP%</b></p>
        <p>MAC: <b>%MAC%</b></p>
      </div>
    </div>
  </div>
  <div class='footer'>
    Atualização automática a cada 2s
  </div>

  <script>
    async function atualizarDados() {
      try {
        const res = await fetch('/data');
        const data = await res.json();
        document.getElementById('temp').textContent = data.temp.toFixed(1);
        document.getElementById('hum').textContent = data.hum.toFixed(1);
      } catch(e) { console.error('Erro ao atualizar', e); }
    }
    setInterval(atualizarDados, 2000);
  </script>
</body>
</html>
)rawliteral";

// ===== HTTP HANDLERS =====
void handleRoot() {
  String html = FPSTR(PAGE_INDEX);
  html.replace("%TEMP%", String(tempAtual, 1));
  html.replace("%HUM%", String(humAtual, 1));
  html.replace("%IP%", ipLocal);
  html.replace("%MAC%", macLocal);
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{\"temp\":" + String(tempAtual,1) + ",\"hum\":" + String(humAtual,1) + "}";
  server.send(200, "application/json", json);
}

// ===== DISPLAY =====
void atualizarDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (isnan(tempAtual) || isnan(humAtual)) {
    display.setTextSize(1);
    display.setCursor(0,25);
    display.println("Erro sensor...");
    Serial.println("Erro ao ler DHT11!");
  } else {
    display.setTextSize(2);
    display.setCursor(10, 0);
    display.printf("%.1fC", tempAtual);

    display.setTextSize(1);
    display.setCursor(10, 25);
    display.printf("Umid: %.1f%%", humAtual);

    display.setTextSize(1);
    display.setCursor(0, 45);
    display.print("IP: "); display.println(ipLocal);
    display.setCursor(0, 55);
    display.print("MAC:"); display.println(macLocal);
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

  if(WiFi.status() == WL_CONNECTED) {
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
  server.on("/data", handleData);
  server.begin();

  Serial.println("SNMP ativo.");
  Serial.println("HTTP ativo.");
  Serial.println("Acesse: http://" + ipLocal);
}

// ===== LOOP =====
void loop() {
  snmp.loop();
  server.handleClient();
  yield();

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 2000) {
    lastUpdate = millis();
    tempAtual = dht.readTemperature();
    humAtual  = dht.readHumidity();
    Serial.printf("Temp: %.1f°C | Umid: %.1f%%\n", tempAtual, humAtual);
    atualizarDisplay();
  }
}
