/*
 * SNMP Agent ESP8266 + Display OLED
 * Autor: Zer0
 * GitHub: https://github.com/Zer0G0ld/ZoneMonitor
 * Data: 27/10/2025
 *
 * Agente SNMP com ESP8266 para monitorar temperatura e umidade via DHT11
 * e mostrar no display OLED 128x64 I2C
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SNMP.h>
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// === CONFIG Wi-Fi ===
const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA";

// === SENSOR ===
#define DHTPIN 2        // GPIO2 (D4)
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

// Função SNMP
void onMessage(const SNMP::Message* message, const IPAddress remote, const uint16_t port) {
    SNMP::VarBindList* varbindlist = message->getVarBindList();
    SNMP::Message* response = new SNMP::Message(SNMP::Version::V2C, "public", SNMP::Type::GetResponse);
    response->setRequestID(message->getRequestID());

    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    for (unsigned int i = 0; i < varbindlist->count(); ++i) {
        SNMP::VarBind* varbind = (*varbindlist)[i];
        const char* oid = varbind->getName();

        if (isnan(temp) || isnan(hum)) {
            Serial.println("Erro ao ler DHT!");
            continue;  // ignora NaN
        }

        if (strcmp(oid, OID_TEMPERATURA) == 0) {
            Serial.printf("Solicitado OID TEMPERATURA: %.2f°C\n", temp);
            response->add(OID_TEMPERATURA, new SNMP::OpaqueFloatBER(temp));
        } 
        else if (strcmp(oid, OID_UMIDADE) == 0) {
            Serial.printf("Solicitado OID UMIDADE: %.2f%%\n", hum);
            response->add(OID_UMIDADE, new SNMP::OpaqueFloatBER(hum));
        }
    }

    snmp.send(response, remote, port);
    delete response;
}

void setup() {
    Serial.begin(9600);
    dht.begin();

    // I2C explicit pins NodeMCU
    Wire.begin(D2, D1);

    // Inicializa OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306 falhou!");
        while(true);  // para o código se o display falhar
    } else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0,0);
        display.println("Iniciando...");
        display.display();
    }

    // Conecta Wi-Fi
    Serial.println("Conectando ao WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Conectado! IP: ");
    Serial.println(WiFi.localIP());

    // Inicializa SNMP
    snmp.begin(udp);
    snmp.onMessage(onMessage);

    Serial.println("SNMP Agent ativo!");
}

void loop() {
    snmp.loop();

    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 2000) {  // Atualiza a cada 2s (DHT11)
        lastUpdate = millis();

        float temp = dht.readTemperature();
        float hum  = dht.readHumidity();

        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(0,0);

        if (isnan(temp) || isnan(hum)) {
            Serial.println("Erro ao ler DHT!");
            display.setTextSize(1);
            display.setCursor(0,0);
            display.println("Iniciando sensor...");
        } else {
            Serial.printf("Temp: %.1f°C  Hum: %.1f%%\n", temp, hum);
            display.print(temp,1);
            display.print("C");
            display.setTextSize(1);
            display.setCursor(0,40);
            display.print("Hum: ");
            display.print(hum,1);
            display.print("%");
        }
        display.display();
    }
}
