# ZoneMonitor

![GitHub repo size](https://img.shields.io/github/repo-size/Zer0G0ld/ZoneMonitor)
![GitHub issues](https://img.shields.io/github/issues/Zer0G0ld/ZoneMonitor)
![GitHub license](https://img.shields.io/github/license/Zer0G0ld/ZoneMonitor)

**ZoneMonitor** é um agente SNMP baseado em ESP8266 que monitora temperatura e umidade usando sensores DHT22.  
Permite consultar os valores via SNMP v2c, sendo ideal para projetos de IoT, automação e monitoramento remoto.

---

## Funcionalidades

- Conecta o ESP8266 a uma rede Wi-Fi.
- Lê temperatura e umidade de um sensor DHT22.
- Responde a requisições SNMP v2c para OIDs configuráveis:
  - `1.3.6.1.4.1.4976.1.1.0` → Temperatura
  - `1.3.6.1.4.1.4976.1.2.0` → Umidade
- Permite integração com ferramentas de monitoramento (Zabbix, PRTG, etc.).
- Código modular e facilmente extensível para novos sensores e OIDs.

---

## Materiais necessários

- ESP8266 (NodeMCU, Wemos D1 Mini, etc.)
- Sensor DHT22
- Cabos de conexão
- Arduino IDE (ou PlatformIO)

---

## Instalação

1. Clone o repositório:

```bash
git clone https://github.com/SEU_USUARIO/ZoneMonitor.git
cd ZoneMonitor
````

2. Abra o projeto na Arduino IDE.
3. Instale as bibliotecas necessárias:

   * [ESP8266WiFi](https://github.com/esp8266/Arduino)
   * [SNMP](https://github.com/your/snmp-library)
   * [DHT sensor library](https://github.com/adafruit/DHT-sensor-library)
4. Configure seu Wi-Fi no sketch:

```cpp
const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA";
```

5. Faça o upload para o ESP8266.

---

## Uso

* Abra o Serial Monitor para verificar o IP do dispositivo.
* Use qualquer cliente SNMP para consultar os OIDs de temperatura e umidade.

Exemplo de teste via `snmpget`:

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
* Twitter/LinkedIn: [LinkedIn](www.linkedin.com/in/theuzer0)