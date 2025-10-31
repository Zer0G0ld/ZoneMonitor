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

bool snmpAtivo = false;  // indica se o SNMP está ativo


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
const char* ssidName     = "Ohost";
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

// ====== ESTILO GLOBAL ======
const char* getGlobalStyle() {
  return R"rawliteral(
    <style>
      * {
        box-sizing: border-box;
      }
      body {
        margin: 0;
        font-family: 'Segoe UI', sans-serif;
        background: #0f172a;
        color: #e2e8f0;
        display: flex;
        flex-direction: row;
        height: 100vh;
        overflow: hidden;
      }
      .sidebar {
        width: 230px;
        background: #111827;
        display: flex;
        flex-direction: column;
        padding: 15px;
        box-shadow: 3px 0 10px rgba(0,0,0,0.4);
        transition: all 0.3s ease;
        z-index: 10;
      }
      .sidebar h2 {
        font-size: 1.2em;
        color: #60a5fa;
        margin-bottom: 15px;
        text-align: center;
      }
      .sidebar button {
        background: none;
        border: none;
        color: #e2e8f0;
        padding: 10px 12px;
        margin: 4px 0;
        text-align: left;
        border-radius: 8px;
        cursor: pointer;
        display: flex;
        align-items: center;
        gap: 10px;
        transition: background 0.2s, transform 0.2s;
      }
      .sidebar button svg {
        min-width: 18px;
        opacity: 0.9;
      }
      .sidebar button:hover, .sidebar button.active {
        background: #2563eb;
        transform: translateX(3px);
      }

      #content {
        flex: 1;
        padding: 20px;
        overflow-y: auto;
        overflow-x: hidden;
        background: linear-gradient(160deg, #0f172a, #1e293b);
        display: flex;
        flex-direction: column;
        align-items: flex-start;
        justify-content: flex-start;
      }
      h3 {
        color: #60a5fa;
        margin-top: 0;
      }
      pre {
        background: rgba(255,255,255,0.05);
        padding: 10px;
        border-radius: 8px;
        overflow: auto;
        width: 100%;
        max-width: 800px;
      }

      /* ====== RESPONSIVO ====== */
      @media (max-width: 800px) {
        body {
          flex-direction: column;
        }
        .sidebar {
          width: 100%;
          flex-direction: row;
          justify-content: space-around;
          box-shadow: 0 3px 10px rgba(0,0,0,0.3);
          padding: 10px 5px;
        }
        .sidebar h2 {
          display: none;
        }
        .sidebar button {
          flex-direction: column;
          font-size: 0.8em;
          padding: 6px;
        }
        #content {
          flex: 1;
          padding: 15px;
        }
      }

      @media (max-width: 480px) {
        .sidebar button span {
          display: none;
        }
        .sidebar button svg {
          width: 22px;
          height: 22px;
        }
      }
    </style>
  )rawliteral";
}

// ===== HTML DASHBOARD =====
const char PAGE_INDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='pt-br'>
<head>
<meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<title>ZoneMonitor CPD</title>
<style>
  body { margin:0;font-family:'Segoe UI',Roboto,sans-serif;background:#0f172a;color:#e2e8f0;display:flex;flex-direction:column;align-items:center;justify-content:center;height:100vh; }
  h2 { color:#38bdf8;font-weight:600;margin-bottom:25px;text-align:center;display:flex;align-items:center;gap:8px; }
  .status-dot { width:10px;height:10px;border-radius:50%;background:#22c55e;animation:pulseDot 1.5s infinite; }
  @keyframes pulseDot {0%,100%{transform:scale(1);opacity:1;}50%{transform:scale(1.4);opacity:0.6;}}

  .dashboard { display:flex; flex-direction:row; flex-wrap:wrap; justify-content:center; gap:20px; }
  .card { background:#1e293b;border-radius:14px;padding:20px 30px;width:260px; box-shadow:0 0 12px rgba(0,0,0,0.35); text-align:center; transition:transform 0.3s ease;}
  .card:hover { transform:scale(1.03); }

  .value { font-size:2.3em;font-weight:600;margin:5px 0; display:flex; align-items:center; justify-content:center; gap:8px; }
  .temp svg { fill:#ef4444; animation:pulse 2s infinite; }
  .hum svg { fill:#3b82f6; animation:pulse 2s infinite; }
  .snmp svg { fill:#22c55e; animation:pulse 2s infinite; }

  @keyframes pulse {0%,100%{opacity:1;transform:scale(1);}50%{opacity:0.6;transform:scale(1.05);} }

  .info { margin-top:8px; font-size:0.9em; color:#94a3b8; }
  .footer { margin-top:15px; font-size:0.8em; color:#64748b; text-align:center; }
  .divider { height:1px;width:80%;background:#334155;margin:10px auto; }
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

  <div class='card'>
    <div class='value snmp'>
      <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path d="M3 3v18h18V3H3Zm8 14H5v-2h6v2Zm8-4H5v-2h14v2Zm0-4H5V7h14v2Z"/></svg>
      <span>SNMP</span>
    </div>
    <div class='divider'></div>
    <div class='info'>
      <p>Status: <b id="snmpStatus">--</b></p>
      <p>Community: <b id="snmpCommunity">--</b></p>
      <p>Porta: <b id="snmpPort">161</b></p>
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
    document.getElementById('snmpStatus').textContent = data.snmpStatus ? 'Ativo' : 'Inativo';
    document.getElementById('snmpCommunity').textContent = data.snmpCommunity;
    document.getElementById('snmpPort').textContent = '161'; // fixa
  } catch(e){
    console.error('Erro ao atualizar', e);
  }
}
setInterval(atualizarDados, 2000);
</script>


</body>
</html>
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

// ===== HTTP HANDLERS =====
void handleData() {
  String json = "{";
  json += "\"temp\":" + String(tempAtual, 1) + ",";
  json += "\"hum\":" + String(humAtual, 1) + ",";
  json += "\"snmpStatus\":" + String(snmpAtivo ? "true" : "false") + ",";
  json += "\"snmpCommunity\":\"" + String(snmpPublic) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

void handleRoot() {
  if (!checkAuth()) return;

  String html = F("<!DOCTYPE html><html lang='pt-br'><head>");
  html += F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  html += F("<title>ZoneMonitor CPD</title>");
  html += getGlobalStyle();  // 👉 injeta o CSS global aqui
  html += F("</head><body>");
  html += F("<h2>ZoneMonitor CPD <span class='status-dot'></span></h2>");
  html += F("<div class='dashboard'><div class='card'>"
            "<div class='value temp'>"
            "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' viewBox='0 0 24 24'><path d='M17 10V5a5 5 0 0 0-10 0v5a7 7 0 1 0 10 0Z'/></svg>"
            "<span id='temp'>");
  html += String(tempAtual, 1);
  html += F("</span>°C</div><div class='divider'></div>"
            "<div class='value hum'>"
            "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24' viewBox='0 0 24 24'><path d='M12 2.69l-.94.94C7.33 7.36 5 10.68 5 14a7 7 0 0 0 14 0c0-3.32-2.33-6.64-6.06-10.37Z'/></svg>"
            "<span id='hum'>");
  html += String(humAtual, 1);
  html += F("</span>%</div><div class='info'>"
            "<p>IP: <b>");
  html += WiFi.localIP().toString();
  html += F("</b></p><p>MAC: <b>");
  html += WiFi.macAddress();
  html += F("</b></p></div></div></div><div class='footer'>Atualização automática a cada 2s</div>"
            "<script>"
            "async function atualizarDados(){"
            "try{const r=await fetch('/data');const j=await r.json();"
            "document.getElementById('temp').textContent=j.temp.toFixed(1);"
            "document.getElementById('hum').textContent=j.hum.toFixed(1);}catch(e){console.error(e);}}"
            "setInterval(atualizarDados,2000);"
            "</script></body></html>");

  server.send(200, "text/html", html);
}

void handlePage() {
  String page = server.arg("name");
  String html;

  if (page == "dashboard") {
    html = F(R"rawliteral(
      <h3>Dashboard de Monitoramento</h3>
      <div class='dashboard' style='display:flex;flex-wrap:wrap;gap:20px;'>
        <div class='card' style='background:#1e293b;border-radius:14px;padding:20px;width:250px;text-align:center;box-shadow:0 0 10px rgba(0,0,0,0.35);'>
          <div class='value temp'>
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path d="M17 10V5a5 5 0 0 0-10 0v5a7 7 0 1 0 10 0Z"/></svg>
            <span id='temp'>--</span>°C
          </div>
          <div class='divider' style='height:1px;width:80%;background:#334155;margin:10px auto;'></div>
          <div class='value hum'>
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24"><path d="M12 2.69l-.94.94C7.33 7.36 5 10.68 5 14a7 7 0 0 0 14 0c0-3.32-2.33-6.64-6.06-10.37Z"/></svg>
            <span id='hum'>--</span>%
          </div>
        </div>
        <div class='card' style='background:#1e293b;border-radius:14px;padding:20px;width:250px;text-align:left;box-shadow:0 0 10px rgba(0,0,0,0.35);'>
          <h4 style='color:#60a5fa;margin-top:0;'>Rede</h4>
          <p>IP: <b id='ip'>%IP%</b></p>
          <p>MAC: <b id='mac'>%MAC%</b></p>
          <p>SSID: <b>Ohost</b></p>
        </div>
      </div>
      <div class='footer' style='margin-top:15px;color:#64748b;font-size:0.85em;'>Atualiza a cada 2s</div>

      <script>
      async function atualizarDados(){
        try{
          const r=await fetch('/data');
          const j=await r.json();
          document.getElementById('temp').textContent=j.temp.toFixed(1);
          document.getElementById('hum').textContent=j.hum.toFixed(1);
        }catch(e){console.error(e);}
      }
      // atualiza a cada 2s
      setInterval(atualizarDados, 2000);

      // chama imediatamente após a página carregar
      window.onload = () => atualizarDados();
      </script>
    )rawliteral");

    html.replace("%IP%", WiFi.localIP().toString());
    html.replace("%MAC%", WiFi.macAddress());
  }

  else if (page == "info") {
    html = "<h3>Informações do Dispositivo</h3><pre>";
    html += "IP: " + WiFi.localIP().toString() + "\n";
    html += "MAC: " + WiFi.macAddress() + "\n";
    html += "SSID: " + String(ssidName) + "\n";
    html += "SNMP: " + String(snmpPublic) + "\n";
    html += "Autor: " + String(creator) + "\n";
    html += "</pre>";
  }

  else if (page == "config") {
    html = F("<h3>Configurações</h3><p>Em breve...</p>");
  }

  else if (page == "reboot") {
    html = F("<h3>Reiniciar Dispositivo</h3><p>Tem certeza?</p><button onclick=\"fetch('/reboot');alert('Reiniciando...')\">Reiniciar Agora</button>");
  }

  else {
    html = F("<h3>404 - Página não encontrada</h3>");
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

void handleReboot() {
  if (!checkAuth()) return;
  server.send(200, "text/plain", "Reiniciando...");
  delay(1000);
  ESP.restart();
}

// ===== ADMIN PANEL (SIDEBAR + CONTEÚDO DINÂMICO) =====
void handleAdmin() {
  if (!checkAuth()) return;

  String html = F("<!DOCTYPE html><html lang='pt-br'><head>");
  html += F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  html += F("<title>ZoneMonitor CPD</title>");
  html += getGlobalStyle();
  html += F("</head><body>");
  
  // ===== SIDEBAR + CONTEÚDO DINÂMICO =====
  html += F("<div class='sidebar'>"
              "<h2>ZoneMonitor</h2>"
              "<button class='active' onclick=\"loadPage('dashboard', this)\">"
                "<svg xmlns='http://www.w3.org/2000/svg' width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><path d='M3 13h8V3H3z'/><path d='M13 21h8V8h-8z'/><path d='M3 21h8v-4H3z'/></svg>"
                "<span>Dashboard</span>"
              "</button>"
              "<button onclick=\"loadPage('info', this)\">"
                "<svg xmlns='http://www.w3.org/2000/svg' width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><circle cx='12' cy='12' r='10'/><line x1='12' y1='16' x2='12' y2='12'/><line x1='12' y1='8' x2='12.01' y2='8'/></svg>"
                "<span>Info</span>"
              "</button>"
              "<button onclick=\"loadPage('config', this)\">"
                "<svg xmlns='http://www.w3.org/2000/svg' width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><circle cx='12' cy='12' r='3'/><path d='M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 1 1-4 0v-.09a1.65 1.65 0 0 0-1-1.51 1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09a1.65 1.65 0 0 0 1.51-1 1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33h.09a1.65 1.65 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09a1.65 1.65 0 0 0 1 1.51h.09a1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82v.09a1.65 1.65 0 0 0 1.51 1H21a2 2 0 1 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z'/></svg>"
                "<span>Config</span>"
              "</button>"
              "<button onclick=\"loadPage('reboot', this)\">"
                "<svg xmlns='http://www.w3.org/2000/svg' width='18' height='18' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2'><polyline points='23 4 23 10 17 10'/><path d='M20.49 15A9 9 0 1 1 12 3v7'/></svg>"
                "<span>Reiniciar</span>"
              "</button>"
            "</div>"
            "<div id='content'><h3>Carregando...</h3></div>");

  // ===== SCRIPT JS =====
  html += F("<script>"
              "async function loadPage(page, btn){"
                "document.querySelectorAll('.sidebar button').forEach(b=>b.classList.remove('active'));"
                "btn.classList.add('active');"
                "const res=await fetch('/page?name='+page);"
                "const html=await res.text();"
                "document.getElementById('content').innerHTML=html;"
              "}"
              "window.onload=()=>{loadPage('dashboard', document.querySelector('.sidebar button.active'));};"
            "</script>");

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

  // Inicia contagem de uptime
  startMillis = millis();

  // ===== SNMP NO SETUP =====
  setupSNMP();

  // HTTP
  server.on("/", handleAdmin);   // usa o painel como principal
  server.on("/admin", handleAdmin);
  server.on("/page", handlePage);
  server.on("/data", handleData);
  server.on("/reboot", handleReboot);
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
    if (millis() - lastUpdate > 2000) {
        lastUpdate = millis();
        tempAtual = dht.readTemperature();
        humAtual  = dht.readHumidity();

        // Atualiza SNMP
        snmpTemp  = isnan(tempAtual) ? 0 : (int)tempAtual;
        snmpHum   = isnan(humAtual) ? 0 : (int)humAtual;
        ipDevice  = WiFi.localIP().toString();
        macDevice = WiFi.macAddress();
        httpURL   = "http://" + ipDevice;

        // Atualiza uptime SNMP (em centésimos de segundo)
        sysUptime = (millis() - startMillis) / 10;

        atualizarDisplay(); // atualiza OLED
    }
}
