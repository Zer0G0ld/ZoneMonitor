# ZoneMonitor

![GitHub repo size](https://img.shields.io/github/repo-size/Zer0G0ld/ZoneMonitor)
![GitHub issues](https://img.shields.io/github/issues/Zer0G0ld/ZoneMonitor)
![GitHub license](https://img.shields.io/github/license/Zer0G0ld/ZoneMonitor)
![Platform](https://img.shields.io/badge/platform-ESP8266-blue)
![SNMP](https://img.shields.io/badge/SNMP-v2c-green)

**ZoneMonitor** é um agente SNMP v2c profissional para ESP8266 que monitora temperatura e umidade com calibração configurável via web, ideal para CPDs, salas de servidores e automação industrial.

---

## 🎯 Funcionalidades

| Funcionalidade | Descrição |
|----------------|-----------|
| **Monitoramento** | Leitura de temperatura e umidade com sensor DHT11 (calibrado via software) |
| **SNMP v2c** | Agente SNMP com OIDs customizados para integração com Zabbix, PRTG e Nagios |
| **Display OLED** | Exibe IP, MAC, temperatura e umidade calibrada localmente |
| **Web Configuração** | Interface moderna para ajustar offset de calibração (até ±10°C) |
| **LittleFS Persistente** | Configurações e offsets salvos mesmo após reinicialização |
| **Calibração Inteligente** | Offset ajustável via web para corrigir imprecisões do DHT11 |
| **Reboot Remoto** | Reinicie o dispositivo pela interface web |
| **Dashboard JSON** | Endpoint `/status` para integração com sistemas externos |

---

## 📡 OIDs SNMP

| OID | Descrição | Tipo | Exemplo |
|-----|-----------|------|---------|
| `1.3.6.1.4.1.49760.3.3` | Temperatura Calibrada | INTEGER (x100) | 2450 = 24.50°C |
| `1.3.6.1.4.1.49760.3.6` | Umidade Calibrada | INTEGER (x100) | 5800 = 58.00% |
| `1.3.6.1.4.1.49760.2.3` | RSSI do WiFi | INTEGER | -52 dBm |
| `1.3.6.1.4.1.49760.1.3` | Nome do Dispositivo | STRING | ZoneMonitor-CPD |
| `1.3.6.1.4.1.49760.2.1` | IP Local | STRING | 192.168.8.95 |
| `1.3.6.1.4.1.49760.2.2` | MAC Address | STRING | CC:50:E3:55:DB:62 |

---

## 🔌 Materiais necessários

| Componente | Especificação |
|------------|----------------|
| **Microcontrolador** | ESP8266 (NodeMCU, Wemos D1 Mini, LOLIN V3) |
| **Sensor** | DHT11 (precisão nominal ±2°C, calibrado via software) |
| **Display** | OLED 128x64 I²C (SSD1306) - opcional |
| **Conexões** | Cabos jumper |
| **Alimentação** | USB (5V) ou 3.3V |

> ⚖️ **Nota de Engenharia Zer0 Tech:** O ZoneMonitor v2.0 foi desenvolvido exclusivamente para o sensor DHT11. Nos testes de campo, identificamos que o DHT11, combinado com o sistema de **Offset calibrado via LittleFS**, entrega precisão suficiente para ambientes de CPD com excelente custo-benefício. O suporte ao DHT22 não está implementado devido às diferenças de timing e curva de resposta - decisão de arquitetura para manter a estabilidade do firmware.

---

## 📁 Esquema de Pinos (Pinout)

| Componente | Função | Pino ESP8266 | Cor Sugerida |
|------------|--------|--------------|--------------|
| **DHT11** | VCC | 3V | Vermelho |
| | DATA | D6 | Azul |
| | GND | G | Preto |
| **OLED** | VCC | 3V | Vermelho |
| | GND | G | Preto |
| | SCL | D1 | Amarelo |
| | SDA | D2 | Verde |

---

## 🚀 Instalação

### 1. Clone o repositório

```bash
git clone https://github.com/Zer0G0ld/ZoneMonitor.git
cd ZoneMonitor
```

### 2. Instale as bibliotecas necessárias (Arduino IDE)

| Biblioteca | Versão | Link |
|------------|--------|------|
| ESP8266WiFi | 3.1.2+ | Pacote ESP8266 |
| SNMP (patricklaf) | 2.1.0+ | [GitHub](https://github.com/patricklaf/SNMP) |
| DHT sensor library | 1.4.4+ | [Adafruit](https://github.com/adafruit/DHT-sensor-library) |
| Adafruit GFX | 1.11.5+ | [Adafruit](https://github.com/adafruit/Adafruit-GFX-Library) |
| Adafruit SSD1306 | 2.5.7+ | [Adafruit](https://github.com/adafruit/Adafruit_SSD1306) |
| LittleFS | - | (Já incluso no pacote ESP8266) |
| ArduinoJson | 6.21.3+ | [GitHub](https://github.com/bblanchon/ArduinoJson) |

### 3. Configure as credenciais WiFi

No arquivo `ZoneMonitor_v2.0.ino`:

```cpp
const char* ssid = "SEU_SSID";
const char* password = "SUA_SENHA";
```

### 4. Faça o upload

| Configuração | Valor |
|--------------|-------|
| **Placa** | NodeMCU 1.0 (ESP-12E Module) |
| **Porta** | COMx (verificar no Gerenciador de Dispositivos) |
| **Upload Speed** | 115200 |

---

## 🌐 Uso

### Acesso via Web

1. Abra o Serial Monitor (115200 baud) para descobrir o IP do dispositivo
2. No navegador, acesse: `http://[IP_DO_ESP]`
3. Login padrão: `admin` / `1234`
4. Configure o offset de calibração:

```
Offset = Temperatura_Real - Temperatura_do_Sensor
Exemplo: 24 - 27 = -3.0
```

### Dashboard Web

A interface possui layout responsivo em 2 colunas:

- **Esquerda:** Formulário de calibração (ajuste de offset)
- **Direita:** Status atual (temp, umidade, IP, MAC, RSSI, SNMP)

### Endpoints HTTP

| Endpoint | Método | Descrição |
|----------|--------|-----------|
| `/` ou `/config` | GET | Dashboard de configuração |
| `/config` | POST | Salva configurações (offset, device name) |
| `/status` | GET | Retorna JSON com todos os dados |
| `/reboot` | GET | Reinicia o dispositivo |

---

## 🔌 Teste SNMP

### No Linux/WSL:

```bash
# Temperatura Calibrada
snmpget -v2c -c public 192.168.8.95 1.3.6.1.4.1.49760.3.3

# Umidade Calibrada
snmpget -v2c -c public 192.168.8.95 1.3.6.1.4.1.49760.3.6

# Walk em todas as OIDs
snmpwalk -v2c -c public 192.168.8.95 1.3.6.1.4.1.49760
```

### No Windows:

Use o **iReasoning MIB Browser** (gratuito):
- **Address**: `192.168.8.95`
- **Community**: `public`
- **Version**: `v2c`
- **OID**: `1.3.6.1.4.1.49760.3.3`

### Integração com Zabbix

Crie um item do tipo **SNMPv2 agent** com:

| Campo | Valor |
|-------|-------|
| **OID** | `1.3.6.1.4.1.49760.3.3` |
| **Community** | `public` |
| **Type of information** | Numeric (float) - divida o valor por 100 |
| **Update interval** | 30s |

---

## 📊 Estatísticas do Firmware

| Métrica | Valor | Status |
|---------|-------|--------|
| Flash usado | ~339KB / 1MB (33%) | ✅ Folga |
| RAM usado | ~31KB / 80KB (38%) | ✅ Confortável |
| IRAM usado | ~28KB / 65KB (93%) | ⚠️ Atenção |

> ⚠️ O IRAM está em 93% - evite adicionar muitas bibliotecas extras para não comprometer a estabilidade do sistema.

---

## 🛠️ Estrutura do Projeto

```
ZoneMonitor/
├── ZoneMonitor_v2.0.ino   # Código principal
├── README.md              # Documentação
├── LICENSE                # GPL-3.0
└── docs/
    └── config.json.example # Exemplo de configuração
```

---

## 🤝 Contribuição

Contribuições são bem-vindas! Áreas de interesse:

- Melhorias na interface web (gráficos históricos)
- Aumento da segurança SNMP (filtragem por IP)
- Implementação de SNMP Traps para alertas de temperatura
- Dashboard com autenticação aprimorada
- Suporte a notificações (Telegram, E-mail)

> ⚠️ **Nota:** Não estamos aceitando contribuições para suporte a outros sensores (DHT22, BME280) pois o firmware foi otimizado especificamente para o DHT11.

Abra uma **issue** ou **pull request** com sua ideia!

---

## 🔬 Nota Técnica - Por que DHT11?

Após extensos testes em ambiente de CPD, a equipe da Zer0 Tech optou por **não implementar** suporte ao DHT22 pelos seguintes motivos:

| Critério | DHT11 + Offset | DHT22 nativo |
|----------|----------------|--------------|
| Precisão final | ±0.5°C (após calibração) | ±0.5°C |
| Custo | R$ 16,00 | R$ 45,00+ |
| Complexidade de timing | Baixa (1Hz) | Alta (0.5Hz) |
| Estabilidade no ESP8266 | ✅ Validada | ⚠️ Oscilações observadas |
| Consumo de CPU | Baixo | Alto |

**Conclusão:** O sistema de offset via software entrega o mesmo resultado prático com custo reduzido e maior estabilidade no firmware.

---

## 👥 Autores

**Zer0G0ld** - Fundador, Zer0 Tech Enterprise
- GitHub: [Zer0G0ld](https://github.com/Zer0G0ld)
- LinkedIn: [TheuZer0](https://www.linkedin.com/in/theuzer0)

**Zer0 Tech Enterprise** - Inovação em Monitoramento de Infraestrutura
- LinkedIn: [Zer0 Tech](https://www.linkedin.com/company/Zer0Tech)

---

## ⭐ Agradecimentos

- **Sujiro Kimimame** - pelo suporte, validação de requisitos e aprovação do sistema de calibração.
- Biblioteca SNMP por [patricklaf](https://github.com/patricklaf/SNMP)
- Biblioteca DHT por [Adafruit](https://github.com/adafruit/DHT-sensor-library)
- Comunidade open source de monitoramento ambiental

---

## 📜 Licença

Este projeto está licenciado sob a **GNU General Public License v3.0** - veja o arquivo [LICENSE](LICENSE) para detalhes.

---

**Desenvolvido com 💻 e 🔧 pela Zer0 Tech Enterprise**

*Monitoramento profissional com software livre*
