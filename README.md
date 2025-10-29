# ZoneMonitor

![GitHub repo size](https://img.shields.io/github/repo-size/Zer0G0ld/ZoneMonitor)
![GitHub issues](https://img.shields.io/github/issues/Zer0G0ld/ZoneMonitor)
![GitHub license](https://img.shields.io/github/license/Zer0G0ld/ZoneMonitor)

**ZoneMonitor** é um agente SNMP baseado em ESP8266 que monitora temperatura e umidade usando sensores DHT11/DHT22.
Permite consultar os valores via SNMP v2c e exibir um dashboard web moderno, sendo ideal para projetos de IoT, automação e monitoramento remoto.

---

## Funcionalidades

* Conecta o ESP8266 a uma rede Wi-Fi.
* Lê temperatura e umidade de um sensor DHT11 ou DHT22.
* Exibe IP, MAC, temperatura e umidade em um **OLED 128x64**.
* Dashboard web moderno com atualização em tempo real (a cada 2s), mostrando temperatura, umidade e status online.
* Responde a requisições SNMP v2c para OIDs configuráveis:

  * `1.3.6.1.4.1.4976.1.1.0` → Temperatura
  * `1.3.6.1.4.1.4976.1.2.0` → Umidade
* Permite integração com ferramentas de monitoramento (Zabbix, PRTG, TheDude etc.).
* Código modular, leve e extensível para novos sensores e OIDs.

---

## Materiais necessários

* ESP8266 (NodeMCU, Wemos D1 Mini, etc.)
* Sensor DHT11 ou DHT22
* OLED 128x64 I²C (opcional, mas recomendado)
* Cabos de conexão
* Arduino IDE (ou PlatformIO)

---

## Instalação

1. Clone o repositório:

```bash
git clone https://github.com/Zer0G0ld/ZoneMonitor.git
cd ZoneMonitor
```

2. Abra o projeto na Arduino IDE.
3. Instale as bibliotecas necessárias:

* [ESP8266WiFi](https://github.com/esp8266/Arduino)
* [SNMP](https://github.com/your/snmp-library)
* [DHT sensor library](https://github.com/adafruit/DHT-sensor-library)
* [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
* [Adafruit SSD1306](https://github.com/adafruit/Adafruit_SSD1306)

4. Configure seu Wi-Fi no sketch:

```cpp
const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA";
```

5. Faça o upload para o ESP8266.

---

## Uso

* Abra o Serial Monitor para verificar o IP do dispositivo.
* Acesse o **dashboard web** digitando o IP do ESP8266 no navegador.
* O dashboard exibe:

  * Temperatura e umidade em tempo real.
  * IP e MAC do dispositivo.
  * Status online (ponto verde pulsante).

### Teste via SNMP

```bash
snmpget -v2c -c public 192.168.X.X 1.3.6.1.4.1.4976.1.1.0
snmpget -v2c -c public 192.168.X.X 1.3.6.1.4.1.4976.1.2.0
```

---

## Estrutura do Projeto

```
ZoneMonitor/
├── ZoneMonitor.ino       # Código principal
├── README.md             # Este arquivo
├── lib/                  # Bibliotecas adicionais, se houver
└── docs/                 # Documentação futura
```

---

## Contribuição

Contribuições são bem-vindas! Abra issues ou pull requests para melhorias, novos sensores ou recursos SNMP adicionais.

---

## Licença

Este projeto é licenciado sob a [GPL3 License](LICENSE).

---

## Contato

* GitHub: [Zer0G0ld](https://github.com/Zer0G0ld)
* LinkedIn: [The Uzér0](https://www.linkedin.com/in/theuzer0)
