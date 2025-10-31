/*
 * ZoneMonitor CPD v1.5 - ESP8266 + DHT11 + SNMP_Agent + OLED + HTTP Moderno
 * Autor: Zer0
 * GitHub: https://github.com/Zer0G0ld/ZoneMonitor
 * Data: 30/10/2025
 *
 * Novidades:
 * - Código modular e otimizado
 * - Página HTTP aprimorada com feedback visual
 * - Novos endpoints: /data, /info, /reboot
 * - Melhor gestão de falhas do sensor
 * - Preparado para expansão (ex: configuração via web)
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

// ==================== CONFIGURAÇÕES ====================
const char* ssid = "SSID";
const char* password = "SENHA";
const bool useAuth = true;
const char* httpUser = "admin";
const char* httpPass = "1234";

#define DHTPIN D6
#define DHTTYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
ESP8266WebServer server(80);
WiFiUDP udp;
SNMPAgent snmp("public", "private");

// ==================== VARIÁVEIS ====================
float tempAtual = NAN, humAtual = NAN;
String ipLocal, macLocal;
unsigned long startMillis = 0;
int sensorFailCount = 0;

int snmpTemp = 0, snmpHum = 0, snmpRSSI = 0, snmpHeap = 0, snmpHttp = 0, snmpSensorFail = 0;
uint64_t sysUptime = 0;

// ==================== SNMP OIDs ====================
#define OID_BASE ".1.3.6.1.4.1.49760"
#define OID_SYS_DESCR   ".1.3.6.1.2.1.1.1.0"
#define OID_SYS_UPTIME  ".1.3.6.1.2.1.1.3.0"
#define OID_SYS_NAME    ".1.3.6.1.2.1.1.5.0"
#define OID_TEMP        OID_BASE ".2.1"
#define OID_HUM         OID_BASE ".2.2"
#define OID_SENSOR_FAIL OID_BASE ".3.1"
#define OID_WIFI_RSSI   OID_BASE ".3.2"
#define OID_FREE_HEAP   OID_BASE ".3.3"
#define OID_HTTP_STATUS OID_BASE ".3.4"
#define OID_HW_UPTIME   OID_BASE ".3.5"

// ==================== HTML ====================
const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang='pt-br'><head>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>
<title>ZoneMonitor CPD</title>
<style>
body{margin:0;font-family:'Segoe UI',sans-serif;background:#0f172a;color:#e2e8f0;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh}
.card{background:#1e293b;border-radius:14px;padding:25px 30px;width:260px;text-align:center;box-shadow:0 0 12px rgba(0,0,0,.35)}
.value{font-size:2.3em;font-weight:600;margin:5px 0}
.red{color:#ef4444} .blue{color:#3b82f6}
.info{margin-top:10px;font-size:.9em;color:#94a3b8}
.footer{margin-top:15px;font-size:.8em;color:#64748b;text-align:center}
</style></head><body>
<h2>ZoneMonitor CPD</h2>
<div class='card'>
  <div class='value red'><span id='temp'>--</span>°C</div>
  <div class='value blue'><span id='hum'>--</span>%</div>
  <div class='info'>IP: <b>%IP%</b><br>MAC: <b>%MAC%</b></div>
</div>
<div class='footer'>Atualiza a cada 2s</div>
<script>
async function atualizar(){
 try{let r=await fetch('/data'),d=await r.json();
 document.getElementById('temp').textContent=d.temp.toFixed(1);
 document.getElementById('hum').textContent=d.hum.toFixed(1);}catch(e){console.log(e);}
}
setInterval(atualizar,2000);atualizar();
</script></body></html>
)rawliteral";

// ==================== AUTENTICAÇÃO ====================
bool checkAuth() {
  if (!useAuth) return true;
  if (!server.authenticate(httpUser, httpPass)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

// ==================== HTTP HANDLERS ====================
void handleRoot() {
  if (!checkAuth()) return;
  String html = FPSTR(PAGE_INDEX);
  html.replace("%IP%", ipLocal);
  html.replace("%MAC%", macLocal);
  server.send(200, "text/html", html);
}

void handleData() {
  if (!checkAuth()) return;
  String json = "{\"temp\":" + String(tempAtual,1) + ",\"hum\":" + String(humAtual,1) +
                ",\"uptime\":" + String((millis()-startMillis)/1000) +
                ",\"heap\":" + String(ESP.getFreeHeap()) +
                ",\"rssi\":" + String(WiFi.RSSI()) + "}";
  server.send(200, "application/json", json);
}

void handleInfo() {
  if (!checkAuth()) return;
  String html = "<h2>Device Info</h2>";
  html += "IP: " + ipLocal + "<br>MAC: " + macLocal + "<br>RSSI: " + String(WiFi.RSSI()) + " dBm<br>";
  html += "Heap livre: " + String(ESP.getFreeHeap()) + " bytes<br>";
  html += "Uptime: " + String((millis()-startMillis)/1000) + "s<br>";
  server.send(200, "text/html", html);
}

void handleReboot() {
  if (!checkAuth()) return;
  server.send(200, "text/plain", "Reiniciando...");
  delay(1000);
  ESP.restart();
}

void handleNotFound() { server.send(404, "text/plain", "404 Not Found"); }

// ==================== DISPLAY ====================
void atualizarDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (isnan(tempAtual) || isnan(humAtual)) {
    display.setCursor(10, 25);
    display.println("Sensor FAIL");
  } else {
    display.setTextSize(2);
    display.setCursor(10, 0);
    display.printf("%.1fC", tempAtual);
    display.setTextSize(1);
    display.setCursor(10, 25);
    display.printf("Umid: %.1f%%", humAtual);
    display.setCursor(0, 45);
    display.print("IP: "); display.println(ipLocal);
  }

  display.display();
}

// ==================== SNMP SETUP ====================
void setupSNMP() {
  snmp.setUDP(&udp);
  snmp.addReadOnlyStaticStringHandler(OID_SYS_DESCR, "ZoneMonitor ESP8266");
  snmp.addCounter64Handler(OID_SYS_UPTIME, &sysUptime);
  snmp.addReadOnlyStaticStringHandler(OID_SYS_NAME, "ZoneMonitor");

  snmp.addIntegerHandler(OID_TEMP, &snmpTemp);
  snmp.addIntegerHandler(OID_HUM, &snmpHum);
  snmp.addIntegerHandler(OID_SENSOR_FAIL, &snmpSensorFail);
  snmp.addIntegerHandler(OID_WIFI_RSSI, &snmpRSSI);
  snmp.addIntegerHandler(OID_FREE_HEAP, &snmpHeap);
  snmp.addIntegerHandler(OID_HTTP_STATUS, &snmpHttp);
  snmp.addCounter64Handler(OID_HW_UPTIME, &sysUptime);

  snmp.sortHandlers();
  snmp.begin();
  Serial.println("✅ SNMP Agent iniciado.");
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  dht.begin();

  Wire.begin(D2, D1);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Conectando WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  ipLocal = WiFi.localIP().toString();
  macLocal = WiFi.macAddress();

  display.clearDisplay();
  display.println("WiFi OK");
  display.println("IP: " + ipLocal);
  display.display();

  setupSNMP();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/info", handleInfo);
  server.on("/reboot", handleReboot);
  server.onNotFound(handleNotFound);
  server.begin();

  startMillis = millis();
  Serial.println("HTTP ativo em http://" + ipLocal);
}

// ==================== LOOP ====================
void loop() {
  snmp.loop();
  server.handleClient();

  static unsigned long lastUpdate = 0;
  static unsigned long lastSNMP = 0;

  if (millis() - lastUpdate > 2000) {
    tempAtual = dht.readTemperature();
    humAtual  = dht.readHumidity();
    if (isnan(tempAtual) || isnan(humAtual)) sensorFailCount++;
    atualizarDisplay();
    lastUpdate = millis();
  }

  if (millis() - lastSNMP > 10000) {
    snmpTemp = isnan(tempAtual) ? 0 : (int)tempAtual;
    snmpHum  = isnan(humAtual) ? 0 : (int)humAtual;
    snmpSensorFail = sensorFailCount;
    snmpRSSI = WiFi.RSSI();
    snmpHeap = ESP.getFreeHeap();
    snmpHttp = 1;
    sysUptime = (millis() - startMillis) / 10;
    lastSNMP = millis();
  }
}
