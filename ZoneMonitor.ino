/*
 * SNMP Agent ESP8266
 * Autor: Zer0
 * GitHub: https://github.com/Zer0G0ld/ZoneMonitor
 * Data: 27/10/2025
 *
 * Agente SNMP com ESP8266 para monitorar temperatura e umidade via DHT22
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <SNMP.h>
#include "DHT.h"

// === CONFIG Wi-Fi ===
const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA";

// === SENSOR ===
#define DHTPIN 2        // GPIO2 (D4 na placa)
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// === SNMP ===
WiFiUDP udp;
SNMP::Agent snmp;

// Nossos OIDs
#define OID_TEMPERATURA "1.3.6.1.4.1.4976.1.1.0"
#define OID_UMIDADE     "1.3.6.1.4.1.4976.1.2.0"

// Função para responder mensagens SNMP
void onMessage(const SNMP::Message* message, const IPAddress remote, const uint16_t port) {
    SNMP::VarBindList* varbindlist = message->getVarBindList();

    SNMP::Message* response = new SNMP::Message(SNMP::Version::V2C, "public", SNMP::Type::GetResponse);
    response->setRequestID(message->getRequestID());

    for (unsigned int i = 0; i < varbindlist->count(); ++i) {
        SNMP::VarBind* varbind = (*varbindlist)[i];
        const char* oid = varbind->getName();

        if (strcmp(oid, OID_TEMPERATURA) == 0) {
            float temp = dht.readTemperature();
            Serial.printf("Solicitado OID TEMPERATURA: %.2f°C\n", temp);
            SNMP::OpaqueFloatBER* value = new SNMP::OpaqueFloatBER(temp);
            response->add(OID_TEMPERATURA, value);
        }
        else if (strcmp(oid, OID_UMIDADE) == 0) {
            float hum = dht.readHumidity();
            Serial.printf("Solicitado OID UMIDADE: %.2f%%\n", hum);
            SNMP::OpaqueFloatBER* value = new SNMP::OpaqueFloatBER(hum);
            response->add(OID_UMIDADE, value);
        }
    }

    snmp.send(response, remote, port);
    delete response;
}

void setup() {
    Serial.begin(9600);
    dht.begin();

    Serial.println("Conectando ao WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\nConectado! IP: ");
    Serial.println(WiFi.localIP());

    snmp.begin(udp);
    snmp.onMessage(onMessage);

    Serial.println("SNMP Agent ativo!");
}

void loop() {
    snmp.loop();
}
