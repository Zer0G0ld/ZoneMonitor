/*
 * ZoneMonitor v2.0. - Zer0 Tech Enterprise
 * Monitor de CPD com SNMP, LittleF5, Calibração
 * ESP8266 + DHT11 + SNMP_Agent + OLED + HTTP Moderno
 * Autor: Zer0
 * GitHub: https://github.com/Zer0G0ld/ZoneMonitor
 * Data: 30/04/2026
 *
 * Novidades:
 * - Código modular e otimizado
 * - Página HTTP aprimorada com feedback visual
 * - Novos endpoints: /data, /info, /reboot
 * - Melhor gestão de falhas do sensor
 * - Preparado para expansão (ex: configuração via web)
 */

// ========== CREDENCIAIS WIFI ==========
const char* ssid = "Ohost";
const char* password = "h12345678";

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <SNMP.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// ========== CONFIGURAÇÕES DE HARDWARE ==========
#define DHTPIN D6
#define DHTTYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
ESP8266WebServer server(80);
WiFiUDP udp;
SNMP::Agent snmp;

// ========== VARIÁVEIS DO SISTEMA ==========
float tempRaw = 0;      // Leitura bruta do sensor
float tempCalib = 0;    // Temperatura com offset aplicado
float humRaw = 0;
float humCalib = 0;
float tempOffset = 0.0; // Offset configurável via web
float humOffset = 0.0;

char deviceName[32] = "ZoneMonitor-CPD";
char location[32] = "CPD Principal";
char adminUser[16] = "admin";
char adminPass[16] = "1234";

unsigned long startTime = 0;
bool snmpActive = false;

// Valores para SNMP (inteiros com 2 casas decimais: 25.5°C = 2550)
int snmpTempRaw = 0;
int snmpTempCalib = 0;
int snmpHumRaw = 0;
int snmpHumCalib = 0;
int snmpRSSI = 0;
uint64_t snmpUptime = 0;

// ========== OIDs ZER0 TECH ENTERPRISE ==========
#define OID_SYS_DESCR    ".1.3.6.1.2.1.1.1.0"
#define OID_SYS_UPTIME   ".1.3.6.1.2.1.1.3.0"
#define OID_SYS_NAME     ".1.3.6.1.2.1.1.5.0"

#define OID_BASE         ".1.3.6.1.4.1.49760"

// 1.x - Sistema
#define OID_VERSION      OID_BASE ".1.1"
#define OID_BUILD        OID_BASE ".1.2"
#define OID_DEVICE_NAME  OID_BASE ".1.3"
#define OID_LOCATION     OID_BASE ".1.4"

// 2.x - Rede
#define OID_IP           OID_BASE ".2.1"
#define OID_MAC          OID_BASE ".2.2"
#define OID_RSSI         OID_BASE ".2.3"
#define OID_SSID         OID_BASE ".2.4"

// 3.x - Sensor (RAW, OFFSET, CALIB)
#define OID_TEMP_RAW     OID_BASE ".3.1"
#define OID_TEMP_OFFSET  OID_BASE ".3.2"
#define OID_TEMP_CALIB   OID_BASE ".3.3"
#define OID_HUM_RAW      OID_BASE ".3.4"
#define OID_HUM_OFFSET   OID_BASE ".3.5"
#define OID_HUM_CALIB    OID_BASE ".3.6"

// 4.x - Ações (writeable)
//#define OID_ACTION_REBOOT       OID_BASE ".4.1"
//#define OID_ACTION_RESET_CONFIG OID_BASE ".4.2"

// ========== FUNÇÕES DE CONFIGURAÇÃO (LittleFS) ==========
void loadConfig() {
  if (!LittleFS.begin()) {
    Serial.println("[FS] Erro ao montar LittleFS! Usando defaults");
    return;
  }
  
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("[FS] config.json não encontrado, criando defaults...");
    saveConfig(); // cria com valores padrão
    configFile = LittleFS.open("/config.json", "r");
    if (!configFile) return;
  }
  
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();
  
  if (error) {
    Serial.println("[FS] Erro ao ler config.json");
    return;
  }
  
  // Carrega valores
  strlcpy(deviceName, doc["device_name"] | "ZoneMonitor-CPD", sizeof(deviceName));
  strlcpy(location, doc["location"] | "CPD Principal", sizeof(location));
  strlcpy(adminUser, doc["admin_user"] | "admin", sizeof(adminUser));
  strlcpy(adminPass, doc["admin_pass"] | "1234", sizeof(adminPass));
  tempOffset = doc["temp_offset"] | 0.0;
  humOffset = doc["hum_offset"] | 0.0;
  
  Serial.printf("[FS] Config carregada: Temp Offset=%.1f\n", tempOffset);
}

void saveConfig() {
  StaticJsonDocument<512> doc;
  doc["device_name"] = deviceName;
  doc["location"] = location;
  doc["admin_user"] = adminUser;
  doc["admin_pass"] = adminPass;
  doc["temp_offset"] = tempOffset;
  doc["hum_offset"] = humOffset;
  
  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("[FS] Erro ao salvar config.json");
    return;
  }
  
  serializeJson(doc, configFile);
  configFile.close();
  Serial.println("[FS] Config salva");
}

// OIDs como strings
const char* OID_TEMP_CALIB_STR = ".1.3.6.1.4.1.49760.3.3";
const char* OID_HUM_CALIB_STR = ".1.3.6.1.4.1.49760.3.6";
const char* OID_RSSI_STR = ".1.3.6.1.4.1.49760.2.3";
const char* OID_DEVICE_NAME_STR = ".1.3.6.1.4.1.49760.1.3";

// Callback para mensagens SNMP
void onSNMPMessage(const SNMP::Message *message, const IPAddress remote, const uint16_t port) {
    Serial.println("[SNMP] Mensagem recebida!");
    
    // Pega a lista de variáveis solicitadas
    SNMP::VarBindList *varbindlist = message->getVarBindList();
    
    // Cria resposta
    SNMP::Message *response = new SNMP::Message(SNMP::Version::V2C, "public", SNMP::Type::GetResponse);
    response->setRequestID(message->getRequestID());
    
    for (unsigned int index = 0; index < varbindlist->count(); ++index) {
        SNMP::VarBind *varbind = (*varbindlist)[index];
        const char *oidName = varbind->getName();
        
        Serial.print("[SNMP] OID solicitada: ");
        Serial.println(oidName);
        
        // Verifica qual OID foi pedida
        if (strcmp(oidName, OID_TEMP_CALIB_STR) == 0) {
            SNMP::IntegerBER* value = new SNMP::IntegerBER(snmpTempCalib);
            response->add(oidName, value);
        }
        else if (strcmp(oidName, OID_HUM_CALIB_STR) == 0) {
            SNMP::IntegerBER* value = new SNMP::IntegerBER(snmpHumCalib);
            response->add(oidName, value);
        }
        else if (strcmp(oidName, OID_RSSI_STR) == 0) {
            SNMP::IntegerBER* value = new SNMP::IntegerBER(snmpRSSI);
            response->add(oidName, value);
        }
        else if (strcmp(oidName, OID_DEVICE_NAME_STR) == 0) {
            SNMP::OctetStringBER* value = new SNMP::OctetStringBER(deviceName);
            response->add(oidName, value);
        }
    }
    
    // Envia resposta
    snmp.send(response, remote, port);
    delete response;
}

// Setup SNMP
void setupSNMP() {
    Serial.println("[SNMP] Iniciando...");
    
    // Inicia o agente
    snmp.begin(udp);
    snmp.onMessage(onSNMPMessage);
    
    snmpActive = true;
    Serial.println("[SNMP] Agente iniciado na porta 161");
    Serial.print("[SNMP] IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[SNMP] OID Temperatura: ");
    Serial.println(OID_TEMP_CALIB_STR);
}

// ========== LEITURA DO SENSOR COM CALIBRAÇÃO ==========
void updateSensors() {
  float rTemp = dht.readTemperature();
  float rHum = dht.readHumidity();
  
  if (!isnan(rTemp)) {
    tempRaw = rTemp;
    tempCalib = rTemp + tempOffset;
    snmpTempRaw = (int)(tempRaw * 100);
    snmpTempCalib = (int)(tempCalib * 100);
  }
  
  if (!isnan(rHum)) {
    humRaw = rHum;
    humCalib = rHum + humOffset;
    snmpHumRaw = (int)(humRaw * 100);
    snmpHumCalib = (int)(humCalib * 100);
  }
  
  snmpRSSI = WiFi.RSSI();
  snmpUptime = (millis() - startTime) / 10;
}

// ========== DISPLAY OLED ==========
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  if (isnan(tempCalib)) {
    display.setTextSize(1);
    display.setCursor(0, 25);
    display.println("Erro sensor!");
  } else {
    display.setTextSize(2);
    display.setCursor(10, 0);
    display.printf("%.1fC", tempCalib);
    display.setTextSize(1);
    display.setCursor(10, 25);
    display.printf("Umid: %.0f%%", humCalib);
    display.setCursor(0, 45);
    display.printf("IP: %s", WiFi.localIP().toString().c_str());
    display.setCursor(0, 55);
    display.printf("DNS: %s", WiFi.dnsIP().toString().c_str());
  }
  display.display();
}

// ========== ENDPOINTS HTTP LEVES ==========
void handleStatus() {
  StaticJsonDocument<256> doc;
  doc["temp_raw"] = tempRaw;
  doc["temp_calib"] = tempCalib;
  doc["temp_offset"] = tempOffset;
  doc["hum_raw"] = humRaw;
  doc["hum_calib"] = humCalib;
  doc["hum_offset"] = humOffset;
  doc["ip"] = WiFi.localIP().toString();
  doc["mac"] = WiFi.macAddress();
  doc["rssi"] = WiFi.RSSI();
  doc["ssid"] = WiFi.SSID();
  doc["device"] = deviceName;
  doc["uptime"] = (millis() - startTime) / 1000;
  doc["snmp"] = snmpActive;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleConfig() {
  if (server.method() == HTTP_POST) {
    // Atualiza configurações
    if (server.hasArg("temp_offset"))
      tempOffset = server.arg("temp_offset").toFloat();
    if (server.hasArg("hum_offset"))
      humOffset = server.arg("hum_offset").toFloat();
    if (server.hasArg("device_name"))
      strlcpy(deviceName, server.arg("device_name").c_str(), sizeof(deviceName));
    
    saveConfig();
    server.send(200, "text/plain", "Config salva. Reinicie para aplicar.");
    return;
  }
  
  // GET - mostra formulário HTML com layout em 2 colunas
  String html = F(
    "<!DOCTYPE html>"
    "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>ZoneMonitor - Zer0 Tech</title>"
    "<style>"
    "*{margin:0;padding:0;box-sizing:border-box}"
    "body{background:#f4f7fa;color:#4a4a6a;font-family:'Segoe UI',Roboto,sans-serif;padding:20px}" 
    ".container{max-width:1200px;margin:0 auto}"
    "h1{color:#524d6d;margin-bottom:20px;border-left:4px solid #524d6d;padding-left:15px;font-weight:800}"
    "h2{color:#6e6e8e;font-size:1.2em;margin-bottom:15px;border-bottom:1px solid #d1d5db;padding-bottom:8px}"
    ".dashboard{display:grid;grid-template-columns:1fr 1fr;gap:25px;margin-bottom:30px}"
    ".card{background:#ffffff;border-radius:12px;padding:20px;box-shadow:0 10px 15px -3px rgba(0,0,0,0.1);border:1px solid #e2e8f0}"
    ".card h3{color:#524d6d;margin-bottom:15px;display:flex;align-items:center;gap:8px;border-bottom:2px solid #f4f7fa;padding-bottom:10px}"
    ".form-group{margin-bottom:15px}"
    "label{display:block;margin-bottom:5px;color:#64748b;font-size:0.9em;font-weight:600}"
    "input{width:100%;padding:12px;background:#f8fafc;border:2px solid #e2e8f0;border-radius:8px;color:#4a4a6a;font-size:1em;transition:border-color 0.2s}"
    "input:focus{outline:none;border-color:#524d6d}"
    "button{background:#524d6d;color:#fff;border:none;padding:12px 24px;border-radius:8px;cursor:pointer;font-size:1em;font-weight:600;transition:opacity 0.2s;width:100%}"
    "button:hover{opacity:0.9}"
    ".status-grid{display:grid;grid-template-columns:1fr 1fr;gap:15px}"
    ".status-item{background:#f8fafc;border-radius:12px;padding:12px;text-align:center;border:1px solid #edf2f7}"
    ".status-label{font-size:0.8em;color:#94a3b8;margin-bottom:5px;text-transform:uppercase;letter-spacing:0.5px}"
    ".status-value{font-size:1.6em;font-weight:700;color:#524d6d}"
    ".temp-value{color:#e11d48}" /* Destaque para temperatura */
    ".ip-value{color:#524d6d;font-size:1.1em}"
    ".footer{text-align:center;margin-top:30px;padding-top:20px;border-top:1px solid #e2e8f0;color:#94a3b8;font-size:0.8em}"
    ".reboot-btn{background:#64748b;margin-top:10px;width:auto}"
    ".reboot-btn:hover{background:#475569}"
    "@media(max-width:768px){.dashboard{grid-template-columns:1fr}}"
    "</style></head>"
    "<body>"
    "<div class='container'>"
    "<h1>🔧 Zer0 Tech - ZoneMonitor v2.0</h1>"
    
    "<div class='dashboard'>"
    "<!-- COLUDA ESQUERDA: CALIBRAÇÃO -->"
    "<div class='card'>"
    "<h3>📐 Calibração</h3>"
    "<form method='POST'>"
    "<div class='form-group'>"
    "<label>🌡️ Temp Offset (°C)</label>"
    "<input type='number' step='0.1' name='temp_offset' value='%TEMP_OFFSET%'>"
    "</div>"
    "<div class='form-group'>"
    "<label>💧 Hum Offset (%)</label>"
    "<input type='number' step='0.1' name='hum_offset' value='%HUM_OFFSET%'>"
    "</div>"
    "<div class='form-group'>"
    "<label>🏷️ Device Name</label>"
    "<input type='text' name='device_name' value='%DEVICE_NAME%'>"
    "</div>"
    "<button type='submit'>💾 Salvar Configuração</button>"
    "</form>"
    "</div>"
    
    "<!-- COLUNA DIREITA: STATUS ATUAL -->"
    "<div class='card'>"
    "<h3>📊 Status Atual</h3>"
    "<div class='status-grid'>"
    "<div class='status-item'><div class='status-label'>🌡️ Temperatura</div><div class='status-value temp-value'>%TEMP%°C</div><div class='status-label' style='font-size:0.7em'>Raw: %TEMP_RAW%°C</div></div>"
    "<div class='status-item'><div class='status-label'>💧 Umidade</div><div class='status-value hum-value'>%HUM%%</div></div>"
    "<div class='status-item'><div class='status-label'>📡 SNMP</div><div class='status-value'>%SNMP%</div><div class='status-label'>Porta 161</div></div>"
    "<div class='status-item'><div class='status-label'>📶 RSSI</div><div class='status-value'>%RSSI% dBm</div></div>"
    "<div class='status-item'><div class='status-label'>🖥️ IP</div><div class='status-value ip-value'>%IP%</div></div>"
    "<div class='status-item'><div class='status-label'>🔌 MAC</div><div class='status-value ip-value'>%MAC%</div></div>"
    "</div>"
    "</div>"
    "</div>"
    
    "<div style='text-align:center'>"
    "<a href='/reboot'><button class='reboot-btn'>🔄 Reiniciar Dispositivo</button></a>"
    "</div>"
    
    "<div class='footer'>"
    "Zer0 Tech Enterprise - Monitoramento Profissional via SNMP | OIDs: .1.3.6.1.4.1.49760.3.3 (Temp) / .3.6 (Hum)"
    "</div>"
    "</div>"
    "</body></html>"
  );
  
  // Substituições
  html.replace("%TEMP_OFFSET%", String(tempOffset));
  html.replace("%HUM_OFFSET%", String(humOffset));
  html.replace("%DEVICE_NAME%", String(deviceName));
  html.replace("%TEMP%", String(tempCalib, 1));
  html.replace("%TEMP_RAW%", String(tempRaw, 1));
  html.replace("%HUM%", String(humCalib, 0));
  html.replace("%IP%", WiFi.localIP().toString());
  html.replace("%MAC%", WiFi.macAddress());
  html.replace("%SNMP%", snmpActive ? "✅ Ativo" : "❌ Inativo");
  html.replace("%RSSI%", String(WiFi.RSSI()));
  
  server.send(200, "text/html", html);
}

void handleReboot() {
  server.send(200, "text/plain", "Reiniciando...");
  delay(500);
  ESP.restart();
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Inicia LittleFS
  loadConfig();
  
  // Sensor
  dht.begin();
  
  // Display
  Wire.begin(D2, D1);
  delay(100);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] Falha no display");
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Zer0 Tech");
  display.println("ZoneMonitor v2.0");
  display.println("Conectando WiFi...");
  display.display();
  
  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi conectado!");
  Serial.println("IP: " + WiFi.localIP().toString());
  
  startTime = millis();
  
  // SNMP
  setupSNMP();
  
  // HTTP Endpoints
  server.on("/", handleConfig);
  server.on("/status", handleStatus);
  server.on("/config", handleConfig);
  server.on("/reboot", handleReboot);
  server.begin();
  
  Serial.println("[HTTP] Servidor iniciado");
  
  // Display final
  updateDisplay();
}

// ========== LOOP ==========
void loop() {
  snmp.loop();
  yield(); // Estabilizar o UDP
  server.handleClient();
  
  static unsigned long lastSensor = 0;
  static unsigned long lastDisplay = 0;
  
  if (millis() - lastSensor > 3000) {
    lastSensor = millis();
    updateSensors();
  }
  
  if (millis() - lastDisplay > 1000) {
    lastDisplay = millis();
    updateDisplay();
  }
}