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
#include <EEPROM.h>

// ==================== CONFIGURAÇÕES ====================
const char* ssid = "SSID";
const char* password = "PASSWORD";
const bool useAuth = true;
const char* httpUser = "admin";
const char* httpPass = "1234";

bool snmpAtivo = false;  // indica se o SNMP está ativo


// === Histórico ===
#define HIST_MAX 30  // número de pontos no histórico (~1 min se atualizar a cada 2s)
float histTemp[HIST_MAX];
float histHum[HIST_MAX];
int histIndex = 0;
bool histFull = false;

// === SENSOR ===
#define DHTPIN D6
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// ===== SNMP =====
// Device Info
const char* creator      = "Zer0";
const char* repo         = "https://github.com/Zer0G0ld/ZoneMonitor";
const char* location     = "CPD";
const char* description  = "ESP8266 ZoneMonitor";
const char* deviceName   = "ESP-ZoneMonitor";
String ipDevice;
String macDevice;
const char* ssidName     = "SSID";
String httpURL;
const char* snmpPublic   = "public";
const char* snmpPrivate  = "private";

// Sensor Data
int snmpTemp = 0;
int snmpHum  = 0;
uint64_t sysUptime = 0;  // CORREÇÃO: uptime para SNMP

// ===== SNMP =====
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

// === SNMP CORE OBJECTS ===
WiFiUDP udp;
SNMPAgent snmp("public", "private");

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
unsigned long startMillis = 0;  // CORREÇÃO: para calcular uptime

// ===== CONFIGURAÇÕES SALVAS =====
struct Config {
  bool snmpAtivo;
  char community[20];
  char devLocation[20];
  char devName[20];
} config;

void salvarConfig() {
  EEPROM.put(0, config);
  EEPROM.commit();
}

void carregarConfig() {
  EEPROM.get(0, config);

  // Se for a primeira vez (EEPROM vazio), aplicar defaults
  if (config.community[0] == 0xFF || config.community[0] == '\0') {
    config.snmpAtivo = true;
    strcpy(config.community, snmpPublic);
    strcpy(config.devLocation, location);
    strcpy(config.devName, deviceName);
    salvarConfig();
  }
}


// ====== ESTILO GLOBAL ======
const char* getGlobalStyle() {
  return R"rawliteral(
    <style>
      * { box-sizing: border-box; margin: 0; padding: 0; }

      body {
        font-family: 'Segoe UI', Roboto, sans-serif;
        background: #0f172a;
        color: #e2e8f0;
        display: flex;
        flex-direction: row; /* row para sidebar, column para dashboards móveis */
        min-height: 100vh;
        overflow: hidden;
      }

      h2, h3, h4 {
        font-weight: 600;
        margin-bottom: 15px;
        color: #60a5fa;
      }

      h2 {
        font-size: 1.6em;
        display: flex;
        align-items: center;
        gap: 8px;
        justify-content: center;
      }

      .sidebar {
        width: 230px;
        background: #111827;
        display: flex;
        flex-direction: column;
        padding: 15px;
        box-shadow: 3px 0 12px rgba(0,0,0,0.5);
        transition: all 0.3s ease;
        position: fixed;
        height: 100%;
        z-index: 10;
      }

      .sidebar h2 {
        font-size: 1.2em;
        color: #38bdf8;
        margin-bottom: 20px;
        text-align: center;
      }

      .sidebar button {
        background: none;
        border: none;
        color: #e2e8f0;
        padding: 12px 15px;
        margin: 5px 0;
        border-radius: 10px;
        text-align: left;
        cursor: pointer;
        display: flex;
        align-items: center;
        gap: 10px;
        transition: background 0.3s ease, transform 0.2s ease;
      }

      .sidebar button svg {
        min-width: 20px;
        opacity: 0.9;
        transition: fill 0.2s;
      }

      .sidebar button:hover,
      .sidebar button.active {
        background: #2563eb;
        transform: translateX(4px);
      }

      #content {
        flex: 1;
        margin-left: 230px;
        padding: 25px 30px;
        overflow-y: auto;
        overflow-x: hidden;
        background: linear-gradient(160deg, #0f172a 0%, #1e293b 100%);
        display: flex;
        flex-direction: column;
        gap: 20px;
      }

      pre {
        background: rgba(255,255,255,0.05);
        padding: 12px;
        border-radius: 10px;
        overflow: auto;
        width: 100%;
        max-width: 800px;
      }

      /* ====== DASHBOARD & CARDS ====== */
      .dashboard {
        display: flex;
        flex-wrap: wrap;
        justify-content: center;
        gap: 20px;
        width: 100%;
        max-width: 1000px;
        margin: 0 auto;
      }

      .card {
        background: #1e293b;
        border-radius: 18px;
        padding: 22px 28px;
        flex: 1 1 260px;
        max-width: 260px;
        text-align: center;
        box-shadow: 0 8px 24px rgba(0,0,0,0.5);
        transition: transform 0.3s ease, box-shadow 0.3s ease;
      }

      .card:hover {
        transform: translateY(-6px) scale(1.05);
        box-shadow: 0 12px 36px rgba(0,0,0,0.7);
      }

      .value {
        font-size: 2.4em;
        font-weight: 600;
        display: flex;
        align-items: center;
        justify-content: center;
        gap: 10px;
        margin: 8px 0;
      }

      .temp svg { fill:#ef4444; animation:pulse 2s infinite; }
      .hum svg { fill:#3b82f6; animation:pulse 2s infinite; }
      .snmp svg { fill:#22c55e; animation:pulse 2s infinite; }

      @keyframes pulse {
        0%, 100% { opacity: 1; transform: scale(1); }
        50% { opacity: 0.6; transform: scale(1.05); }
      }

      @keyframes pulseDot {
        0%, 100% { transform: scale(1); opacity: 1; }
        50% { transform: scale(1.4); opacity: 0.6; }
      }

      .info { margin-top: 8px; font-size: 0.9em; color: #94a3b8; }
      .footer { margin-top: 30px; font-size: 0.8em; color: #64748b; text-align: center; }
      .divider { height: 1px; width: 80%; background: #334155; margin: 15px auto; }

      canvas {
        background: #111827;
        border-radius: 12px;
        margin-top: 25px;
        max-width: 100%;
      }

      .btn-reboot {
        background: #ef4444;
        color: #fff;
        border: none;
        padding: 14px 24px;
        border-radius: 10px;
        cursor: pointer;
        font-weight: 600;
        transition: transform 0.3s ease, background 0.3s ease;
      }

      .btn-reboot:hover {
        background: #f87171;
        transform: scale(1.08);
      }

      .status-dot {
        width: 14px;
        height: 14px;
        border-radius: 50%;
        display: inline-block;
        animation: pulseDot 1.2s infinite;
      }

      .status-dot.temp { background:#ef4444; }
      .status-dot.hum  { background:#3b82f6; }
      .status-dot.snmp { background:#22c55e; }

      /* ====== RESPONSIVO ====== */
      @media (max-width: 900px) {
        body { flex-direction: column; }
        #content { margin-left: 0; padding: 20px; }
        .sidebar { width: 100%; flex-direction: row; justify-content: space-around; padding: 10px; position: relative; height: auto; }
        .dashboard { flex-direction: column; align-items: center; gap: 15px; }
      }

      @media (max-width: 480px) {
        .sidebar button span { display: none; }
        .sidebar button svg { width: 26px; height: 26px; }
      }
    </style>
  )rawliteral";
}

// ==================== AUTENTICAÇÃO ====================
bool checkAuth() {
  if (!useAuth) return true;
  if (!server.authenticate(httpUser, httpPass)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

bool checkSession() {
  if (!useAuth) return true;

  // Permitir sempre acesso à tela de login
  if (server.uri() == "/login") return true;

  if (!sessionActive()) {
    server.sendHeader("Location", "/login");
    server.send(303);
    return false;
  }

  sessionStart = millis(); // 🔥 renova sessão se ativo
  return true;
}

// ===== HTTP HANDLERS =====
void handleData() {
  if (!checkSession()) return;

  String json = "{";
  json += "\"temp\":" + String(tempAtual, 1) + ",";
  json += "\"hum\":" + String(humAtual, 1) + ",";
  json += "\"snmpStatus\":" + String(snmpAtivo ? "true" : "false") + ",";
  json += "\"snmpCommunity\":\"" + String(snmpPublic) + "\",";
  json += "\"uptime\":" + String(sysUptime) + ",";
  json += "\"ip\":\"" + ipDevice + "\",";
  json += "\"mac\":\"" + macDevice + "\","; // ✅ vírgula corrigida

  // Histórico de temperatura
  json += "\"histTemp\":[";
  int count = histFull ? HIST_MAX : histIndex;
  for(int i = 0; i < count; i++){
    if(i > 0) json += ",";
    json += String(histTemp[i], 1);
  }
  json += "],";

  // Histórico de umidade
  json += "\"histHum\":[";
  for(int i = 0; i < count; i++){
    if(i > 0) json += ",";
    json += String(histHum[i], 1);
  }
  json += "]";

  json += "}";

  server.send(200, "application/json", json);
}

// ===== LOGIN =====
const char* PAGE_INDEX = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ZoneMonitor Login</title>
<style>
  body { font-family:sans-serif; display:flex; justify-content:center; align-items:center; height:100vh; background:#0f172a; color:#e2e8f0; }
  .login-box { background:#1e293b; padding:30px; border-radius:10px; box-shadow:0 0 15px rgba(0,0,0,0.5); width:300px; text-align:center; }
  input { width:90%; padding:10px; margin:10px 0; border-radius:5px; border:none; }
  button { padding:10px 20px; border:none; border-radius:5px; background:#2563eb; color:#fff; cursor:pointer; transition:0.2s; }
  button:hover { background:#3b82f6; }
</style>
</head>
<body>
<div class="login-box">
  <h2>ZoneMonitor Login</h2>
  <form method="POST" action="/login">
    <input type="text" name="user" placeholder="Usuário" required><br>
    <input type="password" name="pass" placeholder="Senha" required><br>
    <button type="submit">Entrar</button>
  </form>
</div>
</body>
</html>
)rawliteral";

// ===== SESSÃO =====
unsigned long sessionStart = 0;
const unsigned long sessionDuration = 5 * 60 * 1000; // 5 minutos
bool sessionActive() {
  return (millis() - sessionStart) < sessionDuration;
}

void handleLogin() {
  if (server.method() == HTTP_POST) {
    String user = server.arg("user");
    String pass = server.arg("pass");
    if (user == httpUser && pass == httpPass) {
      sessionStart = millis(); // inicia sessão
      server.sendHeader("Location", "/");
      server.send(303); // redireciona para root
      return;
    } else {
      server.send(200, "text/html", "<h3>Usuário ou senha incorretos</h3><a href='/'>Tentar novamente</a>");
      return;
    }
  }
  server.send(200, "text/html", PAGE_INDEX);
}

/*void handleRoot() {
  html.replace("%TEMP%", String(tempAtual, 1));
  html.replace("%HUM%", String(humAtual, 1));
  html.replace("%IP%", WiFi.localIP().toString());
  html.replace("%MAC%", WiFi.macAddress());

  server.send(200, "text/html", html);
}*/

void handlePage() {
  if (!checkSession()) return;

  String page = server.arg("name");
  String html;

  if (page == "dashboard") {
    html = R"rawliteral(
      <h2>ZoneMonitor CPD <span class="status-dot"></span></h2>

      <div class="dashboard">
        <div class="card">
          <div class="value temp">
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
              <path d="M17 10V5a5 5 0 0 0-10 0v5a7 7 0 1 0 10 0Z"/>
            </svg>
            <span id="temp">%TEMP%</span>°C
          </div>
          <div class="divider"></div>
          <div class="value hum">
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
              <path d="M12 2.69l-.94.94C7.33 7.36 5 10.68 5 14a7 7 0 0 0 14 0c0-3.32-2.33-6.64-6.06-10.37Z"/>
            </svg>
            <span id="hum">%HUM%</span>%
          </div>
          <div class="info">
            <p>IP: <b id="ip">%IP%</b></p>
            <p>MAC: <b id="mac">%MAC%</b></p>
          </div>
        </div>

        <div class="card">
          <div class="value snmp">
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24">
              <path d="M3 3v18h18V3H3Zm8 14H5v-2h6v2Zm8-4H5v-2h14v2Zm0-4H5V7h14v2Z"/>
            </svg>
            <span>SNMP</span>
          </div>
          <div class="divider"></div>
          <div class="info">
            <p>Status: <b id="snmpStatus">%SNMPSTATUS%</b></p>
            <p>Community: <b id="snmpCommunity">%SNMPCOMM%</b></p>
            <p>Porta: <b id="snmpPort">161</b></p>
          </div>
        </div>
      </div>

      <!-- Gráfico Histórico -->
      <canvas id="historicoChart" width="600" height="250"></canvas>
      <div class="footer">Atualização automática a cada 2s</div>

      <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

      <script>
        document.addEventListener('DOMContentLoaded', () => {
        const ctx = document.getElementById('historicoChart').getContext('2d');

        const chart = new Chart(ctx, {
          type: 'line',
          data: {
            labels: [],      // tempos
            datasets: [
              {
                label: 'Temperatura (°C)',
                data: [],
                borderColor: '#ef4444',
                backgroundColor: 'rgba(239,68,68,0.2)',
                tension: 0.3
              },
              {
                label: 'Umidade (%)',
                data: [],
                borderColor: '#3b82f6',
                backgroundColor: 'rgba(59,130,246,0.2)',
                tension: 0.3
              }
            ]
          },
          options: {
            responsive: true,
            plugins: { legend: { position: 'top' } },
            scales: {
              x: { display: true, title: { display: true, text: 'Amostras' } },
              y: { display: true }
            }
          }
        });

        async function atualizarDados() {
          try {
            const res = await fetch('/data');
            const data = await res.json();

            // Atualiza valores atuais
            document.getElementById('temp').textContent = data.temp.toFixed(1);
            document.getElementById('hum').textContent = data.hum.toFixed(1);
            document.getElementById('ip').textContent = data.ip || '--';
            document.getElementById('mac').textContent = data.mac || '--';
            document.getElementById('snmpStatus').textContent = data.snmpStatus ? 'Ativo' : 'Inativo';
            document.getElementById('snmpCommunity').textContent = data.snmpCommunity || '--';

            // Limpa os arrays
            chart.data.labels = [];
            chart.data.datasets[0].data = [];
            chart.data.datasets[1].data = [];

            // Preenche com histórico
            const histLength = data.histTemp.length;
            for (let i = 0; i < histLength; i++) {
              chart.data.labels.push(i + 1);  // ou você pode colocar timestamp se tiver
              chart.data.datasets[0].data.push(data.histTemp[i]);
              chart.data.datasets[1].data.push(data.histHum[i]);
            }

            chart.update();
          } catch(e) {
            console.error('Erro ao atualizar dados:', e);
          }
        }

        setInterval(atualizarDados, 2000);
        atualizarDados();
      });

      </script>
    )rawliteral";

    // Substitui placeholders por valores reais
    html.replace("%TEMP%", String(tempAtual, 1));
    html.replace("%HUM%", String(humAtual, 1));
    html.replace("%IP%", WiFi.localIP().toString());
    html.replace("%MAC%", WiFi.macAddress());
    html.replace("%SNMPSTATUS%", snmpAtivo ? "Ativo" : "Inativo");
    html.replace("%SNMPCOMM%", String(snmpPublic));
  }

  else if (page == "info") {
    html = R"rawliteral(
      <h2>Informações do Dispositivo <span class="status-dot snmp"></span></h2>
      <div class="dashboard">
        <!-- Hardware -->
        <div class="card" style="text-align:left;">
          <h3>Hardware</h3>
          <div class="info">
            <p>Chip ID: <b>%CHIPID%</b></p>
            <p>CPU: <b>%CPU%</b> MHz</p>
            <p>Memória Livre: <b>%FREEHEAP%</b> bytes</p>
            <p>Temperatura Sensor: <b>%TEMP%</b> °C</p>
            <p>Umidade Sensor: <b>%HUM%</b> %</p>
          </div>
        </div>

        <!-- Rede -->
        <div class="card" style="text-align:left;">
          <h3>Rede</h3>
          <div class="info">
            <p>IP: <b>%IP%</b></p>
            <p>MAC: <b>%MAC%</b></p>
            <p>SSID: <b>%SSID%</b></p>
            <p>Gateway: <b>%GATEWAY%</b></p>
            <p>Subnet: <b>%SUBNET%</b></p>
            <p>DNS: <b>%DNS%</b></p>
          </div>
        </div>

        <!-- Sistema -->
        <div class="card" style="text-align:left;">
          <h3>Sistema / Software</h3>
          <div class="info">
            <p>SDK: <b>%SDK%</b></p>
            <p>Uptime: <b>%UPTIME%</b> s</p>
            <p>SNMP Ativo: <b>%SNMPSTATUS%</b></p>
            <p>Community: <b>%SNMPCOMM%</b></p>
            <p>HTTP URL: <b>%HTTP%</b></p>
          </div>
        </div>
      </div>
    )rawliteral";

    // Substituições
    html.replace("%CHIPID%", String(ESP.getChipId()));
    html.replace("%CPU%", String(ESP.getCpuFreqMHz()));
    html.replace("%FREEHEAP%", String(ESP.getFreeHeap()));
    html.replace("%TEMP%", String(tempAtual, 1));
    html.replace("%HUM%", String(humAtual, 1));
    html.replace("%IP%", WiFi.localIP().toString());
    html.replace("%MAC%", WiFi.macAddress());
    html.replace("%SSID%", String(ssidName));
    html.replace("%GATEWAY%", WiFi.gatewayIP().toString());
    html.replace("%SUBNET%", WiFi.subnetMask().toString());
    html.replace("%DNS%", WiFi.dnsIP().toString());
    html.replace("%SDK%", String(ESP.getSdkVersion()));
    html.replace("%UPTIME%", String((millis()-startMillis)/1000));
    html.replace("%SNMPSTATUS%", snmpAtivo ? "Sim" : "Não");
    html.replace("%SNMPCOMM%", String(snmpPublic));
    html.replace("%HTTP%", "http://" + WiFi.localIP().toString());
  }

  else if (page == "config") {
    html = "<h3>Configurações</h3><p>Em breve...</p>";
  }

  else if (page == "reboot") {
    html = "<h3>Reiniciar Dispositivo</h3>"
           "<p>Tem certeza?</p>"
           "<button class='btn-reboot' onclick=\"fetch('/reboot');alert('Reiniciando...')\">Reiniciar Agora</button>";
  }

  else {
    html = "<h3>404 - Página não encontrada</h3>";
  }

  server.send(200, "text/html", html);
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

/*void handleReboot() {
  if (!checkAuth()) return;
  server.send(200, "text/plain", "Reiniciando...");
  delay(1000);
  ESP.restart();
}*/
void handleReboot() {
  if (!checkSession()) return;

  server.send(200, "text/html", "<h3>Reiniciando...</h3>");
  delay(1000);
  ESP.restart();
}


void handleConfigPage() {
  String html = R"====(
  <!DOCTYPE html>
  <html lang="pt-BR">
  <head>
    <meta charset="UTF-8">
    <style>
      body { font-family: Arial; background:#111; color:#eee; padding:20px; }
      h2 { color:#4dd0e1; }
      .card { background:#222; padding:20px; border-radius:10px; width:360px; }
      label { display:block; margin-top:12px; font-size:14px; }
      input[type=text] {
        width:100%; padding:10px; border-radius:6px; border:none; margin-top:5px; background:#333; color:#fff;
      }
      .btn {
        margin-top:18px; width:100%; padding:12px;
        background:#4dd0e1; color:#000; border:none; font-weight:bold;
        cursor:pointer; border-radius:8px; font-size:15px;
      }
      .btn:hover { background:#00bcd4; }
      .checkbox { margin-top:12px; }
      a { color:#4dd0e1; text-decoration:none; }
      a:hover { text-decoration:underline; }
    </style>
  </head>

  <body>
    <h2>ZoneMonitor - Configurações SNMP</h2>
    <div class="card">
      <form action="/saveconfig" method="POST">
        <label>SNMP Community:</label>
        <input type="text" name="community" value=")====" + String(config.community) + R"====(" required>

        <label class="checkbox">
          <input type="checkbox" name="snmp" )====" + (config.snmpAtivo ? String("checked") : String("")) + R"====(>
          SNMP Ativado
        </label>

        <button class="btn" type="submit">Salvar Configurações</button>
      </form>
    </div>
    <br>
    <a href="/">⬅ Voltar ao Painel</a>
  </body>
  </html>
  )====";

  server.send(200, "text/html", html);
}

void handleSaveConfig() {
  if (!autenticado()) return;

  if (!server.hasArg("community")) {
    server.send(400, "text/plain", "Erro: Dados faltando!");
    return;
  }

  // Lê campos recebidos
  String newCommunity = server.arg("community");
  bool newSNMPStatus = server.hasArg("snmp");

  Serial.println("\n== Salvando Config SNMP ==");
  Serial.println("SNMP: " + String(newSNMPStatus ? "ON" : "OFF"));
  Serial.println("Community: " + newCommunity);

  // Atualiza estrutura
  config.snmpAtivo = newSNMPStatus;
  strncpy(config.community, newCommunity.c_str(), sizeof(config.community));
  config.community[sizeof(config.community) - 1] = '\0'; // segurança

  // Salva na EEPROM
  EEPROM.put(0, config);
  EEPROM.commit();

  // Aplica em tempo real
  snmpAtivo = config.snmpAtivo;
  strcpy((char*)snmpPublic, config.community);

  server.sendHeader("Location", "/config");
  server.send(303);
}

// ===== ADMIN PANEL (SIDEBAR + CONTEÚDO DINÂMICO) =====
void handleAdmin() {
  // Proteção única de autenticação
  if (useAuth && !sessionActive()) {
    server.sendHeader("Location", "/login");
    server.send(303);
    return;
  }

  Serial.println("Painel ADMIN carregado");

  String html;
  html.reserve(5000); // ✅ evita fragmentação e resets

  html += F("<!DOCTYPE html><html lang='pt-br'><head>");
  html += F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  html += F("<title>ZoneMonitor CPD</title>");
  html += getGlobalStyle();
  html += F("</head><body>");

  // Sidebar
  html += F(
    "<div class='sidebar'>"
      "<h2>ZoneMonitor</h2>"
      "<button class='active' onclick=\"loadPage('dashboard', this)\">"
        "<span>Dashboard</span>"
      "</button>"
      "<button onclick=\"loadPage('info', this)\">"
        "<span>Info</span>"
      "</button>"
      "<button onclick=\"loadPage('config', this)\">"
        "<span>Config</span>"
      "</button>"
      "<button onclick=\"loadPage('reboot', this)\">"
        "<span>Reiniciar</span>"
      "</button>"
    "</div>"
    "<div id='content'><h3>Carregando...</h3></div>"
  );

  // Scripts
  html += F(
    "<script>"
      "async function loadPage(page, btn){"
        "document.querySelectorAll('.sidebar button').forEach(b=>b.classList.remove('active'));"
        "btn.classList.add('active');"
        "const res = await fetch('/page?name='+page);"
        "const html = await res.text();"
        "document.getElementById('content').innerHTML = html;"
      "}"
      "window.onload=()=>{"
        "loadPage('dashboard', document.querySelector('.sidebar button.active'));"
      "};"
    "</script>"
  );

  html += F("</body></html>");

  server.send(200, "text/html", html);
}

// ===== SETUP SNMP =====
void setupSNMP() {
  snmp.setUDP(&udp);

  // === Buffers fixos para IP, MAC e HTTP ===
  static char ipBuffer[16];
  static char macBuffer[18];
  static char httpBuffer[64];

  sprintf(ipBuffer, "%s", WiFi.localIP().toString().c_str());
  sprintf(macBuffer, "%s", WiFi.macAddress().c_str());
  sprintf(httpBuffer, "http://%s", WiFi.localIP().toString().c_str());

  // === Inicia o agente SNMP na porta 161 ===
  if (udp.begin(161)) {
    snmp.begin();
    snmpAtivo = true;
    Serial.println("[SNMP] Agente iniciado na porta 161");
  } else {
    snmpAtivo = false;
    Serial.println("[SNMP] Falha ao iniciar UDP (porta 161 ocupada?)");
  }

  // === MIB-2 padrão ===
  snmp.addReadOnlyStaticStringHandler(OID_SYS_DESCR, description);
  snmp.addCounter64Handler(OID_SYS_UPTIME, &sysUptime);
  snmp.addReadOnlyStaticStringHandler(OID_SYS_NAME, deviceName);

  // === OIDs customizados ===
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

  // === Sensores ===
  snmp.addIntegerHandler(OID_TEMPERATURA, &snmpTemp);
  snmp.addIntegerHandler(OID_UMIDADE, &snmpHum);
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  carregarConfig();  // Carrega configuração da EEPROM

  snmpAtivo = config.snmpAtivo;
  strcpy((char*)snmpPublic, config.community);

  // OLED
  Wire.begin(D2, D1);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("Falha OLED!");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ZoneMonitor");
  display.println("Conectando WiFi...");
  display.display();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int tentativas = 0;

  while (WiFi.status() != WL_CONNECTED && tentativas < 40) {
    delay(500);
    Serial.print(".");
    display.print(".");
    display.display();
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    ipLocal = WiFi.localIP().toString();
    macLocal = WiFi.macAddress();

    Serial.println("\nWiFi conectado!");
    Serial.println("IP: " + ipLocal);
    Serial.println("MAC: " + macLocal);

    display.clearDisplay();
    display.setCursor(0, 0);
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

  dht.begin(); // ✅ Agora apenas UMA vez e após WiFi

  startMillis = millis(); // Inicia uptime SNMP

  // SNMP
  setupSNMP();

  // Rotas HTTP
  server.on("/", handleAdmin);
  server.on("/admin", handleAdmin);
  server.on("/login", handleLogin);
  server.on("/page", handlePage);
  server.on("/data", handleData);
  server.on("/reboot", handleReboot);
  server.on("/config", HTTP_GET, handleConfigPage);
  server.on("/saveconfig", HTTP_POST, handleSaveConfig);
  server.begin();

  Serial.println("HTTP ativo.");
  Serial.println("Acesse: http://" + ipLocal);
}

// ===== LOOP =====
void loop() {
  snmp.loop();
  server.handleClient();
  yield();

  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 2000) {
    lastUpdate = millis();

    tempAtual = dht.readTemperature();
    humAtual  = dht.readHumidity();

    // Histórico
    histTemp[histIndex] = isnan(tempAtual) ? 0 : tempAtual;
    histHum[histIndex]  = isnan(humAtual) ? 0 : humAtual;
    histIndex = (histIndex + 1) % HIST_MAX;
    if (histIndex == 0) histFull = true;

    // SNMP
    snmpTemp = isnan(tempAtual) ? 0 : (int)tempAtual;
    snmpHum  = isnan(humAtual)  ? 0 : (int)humAtual;
    ipDevice  = WiFi.localIP().toString();
    macDevice = WiFi.macAddress();
    httpURL   = "http://" + ipDevice;
    sysUptime = (millis() - startMillis) / 10;

    atualizarDisplay();
  }
}
