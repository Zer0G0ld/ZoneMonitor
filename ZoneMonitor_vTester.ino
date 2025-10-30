/*
 * ZoneMonitor CPD v1.4 - ESP8266 + DHT11 + SNMP_Agent + OLED + HTTP Moderno
 * Autor: Zer0
 * GitHub: https://github.com/Zer0G0ld/ZoneMonitor
 * Data: 30/10/2025
 *
 * Novidades:
 * - Novos OIDs SNMP: RSSI, Heap, Sensor Status, HTTP Status, Uptime HW
 * - Contadores de erro do sensor
 * - Atualização SNMP/HTTP a cada 10s
 * - Dashboard e OLED continuam atualizando a cada 2s
 */

#include <string>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <SNMP_Agent.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === CONFIG Wi-Fi ===
const char* ssid = "Ohost";
const char* password = "h12345678";

// === SENSOR ===
#define DHTPIN D6
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ===== SNMP =====
const char* creator      = "Zer0";
const char* repo         = "https://github.com/Zer0G0ld/ZoneMonitor";
const char* location     = "CPD";
const char* description  = "ESP8266 ZoneMonitor";
const char* deviceName   = "ESP-ZoneMonitor";
const char* ssidName     = "Ohost";
const char* snmpPublic   = "public";
const char* snmpPrivate  = "private";

int snmpTemp = 0;
int snmpHum  = 0;
uint64_t sysUptime = 0;    // centésimos de segundo
int snmpSensorFail = 0;
int snmpRSSI = 0;
int snmpHeap = 0;
int snmpHttp = 0;

// Base MIB-2 padrão
#define OID_SYS_DESCR   ".1.3.6.1.2.1.1.1.0"
#define OID_SYS_UPTIME  ".1.3.6.1.2.1.1.3.0"
#define OID_SYS_NAME    ".1.3.6.1.2.1.1.5.0"

// Base custom enterprise
#define OID_BASE_ENTERPRISE ".1.3.6.1.4.1.49760"

// Device Info
#define OID_CREATOR       OID_BASE_ENTERPRISE ".1.1"
#define OID_REPO          OID_BASE_ENTERPRISE ".1.2"
#define OID_LOCATION      OID_BASE_ENTERPRISE ".1.3"
#define OID_DESCRIPTION   OID_BASE_ENTERPRISE ".1.4"
#define OID_DEVICENAME    OID_BASE_ENTERPRISE ".1.5"
#define OID_IP            OID_BASE_ENTERPRISE ".1.6"
#define OID_MAC           OID_BASE_ENTERPRISE ".1.7"
#define OID_SSID          OID_BASE_ENTERPRISE ".1.8"
#define OID_HTTP          OID_BASE_ENTERPRISE ".1.9"
#define OID_SNMP_PUBLIC   OID_BASE_ENTERPRISE ".1.10"
#define OID_SNMP_PRIVATE  OID_BASE_ENTERPRISE ".1.11"

// Sensor Data
#define OID_TEMPERATURA   OID_BASE_ENTERPRISE ".2.1"
#define OID_UMIDADE       OID_BASE_ENTERPRISE ".2.2"

// Novos OIDs SNMP para monitoramento
#define OID_SENSOR_FAIL   OID_BASE_ENTERPRISE ".3.1"
#define OID_WIFI_RSSI     OID_BASE_ENTERPRISE ".3.2"
#define OID_FREE_HEAP     OID_BASE_ENTERPRISE ".3.3"
#define OID_HTTP_STATUS   OID_BASE_ENTERPRISE ".3.4"
#define OID_HW_UPTIME     OID_BASE_ENTERPRISE ".3.5"

// === SNMP CORE OBJECTS ===
WiFiUDP udp;
SNMPAgent snmp(snmpPublic, snmpPrivate);

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
unsigned long startMillis = 0;
int sensorFailCount = 0;

// ===== HTML DASHBOARD =====
const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='pt-br'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>ZoneMonitor CPD</title>
<style>
body { margin:0;font-family:'Segoe UI', Roboto,sans-serif;background:#0f172a;color:#e2e8f0;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh; }
h2 { color:#38bdf8;font-weight:600;margin-bottom:25px;text-align:center;display:flex;align-items:center;gap:8px; }
.status-dot { width:10px;height:10px;border-radius:50%;background:#22c55e;animation:pulseDot 1.5s infinite; }
@keyframes pulseDot {0%,100%{transform:scale(1);opacity:1;}50%{transform:scale(1.4);opacity:0.6;}}
.dashboard { display:flex; flex-direction:column; align-items:center; gap:15px; }
.card { background:#1e293b;border-radius:14px;padding:20px 30px;width:260px; box-shadow:0 0 12px rgba(0,0,0,0.35); text-align:center; transition:transform 0.3s ease;}
.card:hover { transform:scale(1.03); }
.value { font-size:2.3em;font-weight:600;margin:5px 0; display:flex; align-items:center; justify-content:center; gap:8px; }
.temp svg { fill:#ef4444; animation:pulse 2s infinite; }
.hum svg { fill:#3b82f6; animation:pulse 2s infinite; }
@keyframes pulse {0%,100%{opacity:1;transform:scale(1);}50%{opacity:0.6;transform:scale(1.05);} }
.info { margin-top:8px; font-size:0.9em; color:#94a3b8; }
.footer { margin-top:15px; font-size:0.8em; color:#64748b; text-align:center; }
.divider { height:1px;width:80%;background:#334155;margin:10px 0; }
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
<div class='footer'>Atualização automática a cada 2s</div>
<script>
async function atualizarDados() {
  try {
    const res = await fetch('/data');
    const data = await res.json();
    document.getElementById('temp').textContent = data.temp.toFixed(1);
    document.getElementById('hum').textContent = data.hum.toFixed(1);
  } catch(e){ console.error('Erro ao atualizar', e); }
}
setInterval(atualizarDados,2000);
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
  String json = "{\"temp\":" + String(tempAtual,1) + 
                ",\"hum\":" + String(humAtual,1) +
                ",\"uptime\":" + String((millis()-startMillis)/1000) +
                ",\"heap\":" + String(ESP.getFreeHeap()) +
                ",\"rssi\":" + String(WiFi.RSSI()) +
                ",\"sensor_fail\":" + String(sensorFailCount) + "}";
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

    display.setCursor(0, 45);
    display.print("IP: "); display.println(ipLocal);
    display.setCursor(0, 55);
    display.print("MAC:"); display.println(macLocal);
  }

  display.display();
}

// ===== SETUP SNMP =====
void setupSNMP() {
    snmp.setUDP(&udp);

    static char ipBuffer[16];
    static char macBuffer[18];
    static char httpBuffer[64];

    sprintf(ipBuffer, "%s", WiFi.localIP().toString().c_str());
    sprintf(macBuffer, "%s", WiFi.macAddress().c_str());
    sprintf(httpBuffer, "http://%s", WiFi.localIP().toString().c_str());

    // MIB-2 padrão
    snmp.addReadOnlyStaticStringHandler(OID_SYS_DESCR, description);
    snmp.addCounter64Handler(OID_SYS_UPTIME, &sysUptime);
    snmp.addReadOnlyStaticStringHandler(OID_SYS_NAME, deviceName);

    // OIDs custom
    snmp.addReadOnlyStaticStringHandler(OID_CREATOR, creator);
    snmp.addReadOnlyStaticStringHandler(OID_REPO, repo);
    snmp.addReadOnlyStaticStringHandler(OID_LOCATION, location);
    snmp.addReadOnlyStaticStringHandler(OID_DESCRIPTION, description);
    snmp.addReadOnlyStaticStringHandler(OID_DEVICENAME, deviceName);
    snmp.addReadOnlyStaticStringHandler(OID_IP, ipBuffer);
    snmp.addReadOnlyStaticStringHandler(OID_MAC, macBuffer);
    snmp.addReadOnlyStaticStringHandler(OID_SSID, ssidName);
    snmp.addReadOnlyStaticStringHandler(OID_HTTP, httpBuffer);
    snmp.addReadOnlyStaticStringHandler(OID_SNMP_PUBLIC, snmpPublic);
    snmp.addReadOnlyStaticStringHandler(OID_SNMP_PRIVATE, snmpPrivate);

    // Sensor e monitoramento
    snmp.addIntegerHandler(OID_TEMPERATURA, &snmpTemp);
    snmp.addIntegerHandler(OID_UMIDADE, &snmpHum);
    snmp.addIntegerHandler(OID_SENSOR_FAIL, &snmpSensorFail);
    snmp.addIntegerHandler(OID_WIFI_RSSI, &snmpRSSI);
    snmp.addIntegerHandler(OID_FREE_HEAP, &snmpHeap);
    snmp.addIntegerHandler(OID_HTTP_STATUS, &snmpHttp);
    snmp.addCounter64Handler(OID_HW_UPTIME, &sysUptime);

    snmp.sortHandlers();
    snmp.begin();
    Serial.println("✅ SNMP Agent completo iniciado!");
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  delay(200);
  dht.begin();

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
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("WiFi conectado!");
    display.print("IP: "); display.println(ipLocal);
    display.print("MAC: "); display.println(macLocal);
    display.display();
    delay(1500);
  } else {
    display.clearDisplay();
    display.println("Falha WiFi!");
    display.display();
  }

  startMillis = millis();

  setupSNMP();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("HTTP ativo.");
  Serial.println("Acesse: http://" + ipLocal);
}

// ===== LOOP =====
void loop() {
    snmp.loop();
    server.handleClient();
    yield();

    static unsigned long lastDisplay = 0;
    static unsigned long lastSNMP = 0;

    // Atualiza OLED e leitura sensor a cada 2s
    if (millis() - lastDisplay > 2000) {
        lastDisplay = millis();
        tempAtual = dht.readTemperature();
        humAtual  = dht.readHumidity();

        if (isnan(tempAtual) || isnan(humAtual)) sensorFailCount++;
        atualizarDisplay();
    }

    // Atualiza SNMP/HTTP a cada 10s
    if (millis() - lastSNMP > 10000) {
        lastSNMP = millis();
        snmpTemp  = isnan(tempAtual) ? 0 : (int)tempAtual;
        snmpHum   = isnan(humAtual) ? 0 : (int)humAtual;
        snmpSensorFail = sensorFailCount;
        snmpRSSI  = WiFi.RSSI();
        snmpHeap  = ESP.getFreeHeap();
        snmpHttp  = server.client() ? 1 : 0;
        sysUptime = (millis() - startMillis) / 10;
    }
}
