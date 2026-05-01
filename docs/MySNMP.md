# Documentação Técnica – SNMP ZoneMonitor v2.0

**Autor:** Zer0G0ld | Zer0 Tech Enterprise  
**Data:** 01/05/2026  
**Versão:** 2.0 (Produção)  
**Objetivo:** Documentar a implementação SNMP v2c para integração com sistemas de gerenciamento de rede (NMS) como Zabbix, PRTG, Nagios e TheDude.

---

## Índice

1. [Arquitetura do Agente](#1-arquitetura-do-agente)
2. [Estrutura de OIDs (MIB Customizada)](#2-estrutura-de-oids-mib-customizada)
   - 2.1 [Identificação do Dispositivo](#21-identificação-do-dispositivo-1313614141497601)
   - 2.2 [Telemetria de Sensores](#22-telemetria-de-sensores-1313614141497602)
   - 2.3 [Diagnóstico e Saúde](#23-diagnóstico-e-saúde-1313614141497603)
   - 2.4 [MIB-2 Padrão (Sistema)](#24-mib-2-padrão-sistema)
3. [Integração com Zabbix](#3-integração-com-zabbix)
   - 3.1 [Template Zabbix](#31-template-zabbix-zoneMonitor)
   - 3.2 [Triggers Recomendados](#32-triggers-recomendados)
   - 3.3 [Gráficos e Dashboards](#33-gráficos-e-dashboards)
4. [Integração com Outras Ferramentas](#4-integração-com-outras-ferramentas)
5. [Comandos de Validação (CLI)](#5-comandos-de-validação-cli)
6. [Dicas de Monitoramento Avançado](#6-dicas-de-monitoramento-avançado)
7. [Diagrama SNMP – ZoneMonitor](#7-diagrama-snmp--zonemonitor)
8. [Fluxo de Atualização](#8-fluxo-de-atualização)
9. [Considerações de Segurança](#9-considerações-de-segurança)
10. [Histórico de Revisões](#10-histórico-de-revisões)

---

## 1. Arquitetura do Agente

O ZoneMonitor opera um **Agente SNMP v2c** otimizado para o ESP8266, rodando em paralelo ao servidor HTTP.

| Parâmetro | Valor | Descrição |
|-----------|-------|-----------|
| **Protocolo** | SNMP v2c | Suporte a GET e GETNEXT |
| **Porta** | 161 (UDP) | Porta padrão SNMP |
| **Comunidade de Leitura** | `public` | Configurável via Web/LittleFS |
| **Comunidade de Escrita** | `private` | Não implementado (read-only) |
| **Frequência de Atualização** | 10 segundos | Cache SNMP renovado para preservar CPU |
| **Buffer UDP** | 255 bytes | Tamanho máximo do pacote |

**Arquitetura em camadas:**

```
┌─────────────────────────────────────────────────────────┐
│                    Cliente NMS                          │
│              (Zabbix / PRTG / Nagios)                   │
└─────────────────────┬───────────────────────────────────┘
                      │ UDP 161
                      ▼
┌─────────────────────────────────────────────────────────┐
│              Agente SNMP (ESP8266)                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │  snmp.loop()│  │ onMessage() │  │  Handlers   │    │
│  └─────────────┘  └─────────────┘  └─────────────┘    │
└─────────────────────┬───────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────┐
│              Cache SNMP (atualizado a cada 10s)         │
│  temp_calib │ hum_calib │ rssi │ uptime │ heap         │
└─────────────────────────────────────────────────────────┘
```

---

## 2. Estrutura de OIDs (MIB Customizada)

A árvore de OIDs foi organizada sob o prefixo Enterprise da Zer0 Tech: **.1.3.6.1.4.1.49760**

### 2.1 Identificação do Dispositivo (.1.3.6.1.4.1.49760.1)

| OID Final | Descrição | Tipo | Exemplo | Uso |
|-----------|-----------|------|---------|-----|
| `.1.1` | Versão do Firmware | String | `"2.0"` | Inventário |
| `.1.2` | Data de Build | String | `"Apr 30 2026"` | Inventário |
| `.1.3` | Nome do Dispositivo | String | `"ZoneMonitor-CPD-01"` | **Identificação** |
| `.1.4` | Localização | String | `"CPD Principal"` | Inventário |
| `.1.5` | Endereço IP | String | `"192.168.8.95"` | **Descoberta de rede** |
| `.1.6` | Endereço MAC | String | `"CC:50:E3:55:DB:62"` | **Identificação única** |
| `.1.7` | SSID Wi-Fi | String | `"Ohost"` | Diagnóstico |

### 2.2 Telemetria de Sensores (.1.3.6.1.4.1.49760.2)

| OID Final | Descrição | Tipo | Escala | Exemplo | Uso |
|-----------|-----------|------|--------|---------|-----|
| `.2.1` | **Temperatura Calibrada** | Integer | x100 | 2450 = 24.50°C | **Alertas e dashboards** |
| `.2.2` | **Umidade Calibrada** | Integer | x100 | 5800 = 58.00% | **Alertas e dashboards** |

### 2.3 Diagnóstico e Saúde (.1.3.6.1.4.1.49760.3)

| OID Final | Descrição | Tipo | Faixa | Uso |
|-----------|-----------|------|-------|-----|
| `.3.1` | Temperatura Bruta (Raw) | Integer | 0-50°C (x100) | Diagnóstico de hardware |
| `.3.2` | Sinal WiFi (RSSI) | Integer | -100 a 0 dBm | Qualidade de conectividade |
| `.3.3` | Heap Livre | Integer | 0-80000 bytes | Detecção de memory leak |
| `.3.4` | Status HTTP | Integer | 0=offline, 1=online | Monitoramento do servidor web |
| `.3.5` | Uptime do Hardware | Counter64 | centésimos de segundo | Detecção de reboots |

### 2.4 MIB-2 Padrão (Sistema)

| OID | Descrição | Valor no ZoneMonitor |
|-----|-----------|---------------------|
| `.1.3.6.1.2.1.1.1.0` | sysDescr | `"Zer0 Tech ZoneMonitor v2.0"` |
| `.1.3.6.1.2.1.1.3.0` | sysUpTime | Uptime em centésimos de segundo |
| `.1.3.6.1.2.1.1.5.0` | sysName | Nome configurável via web |

---

## 3. Integração com Zabbix

### 3.1 Template Zabbix (ZoneMonitor)

```xml
<?xml version="1.0" encoding="UTF-8"?>
<zabbix_export>
    <templates>
        <template>
            <name>ZoneMonitor v2.0</name>
            <description>Template para monitoramento de temperatura/umidade via SNMP</description>
            <groups>
                <group>IoT</group>
            </groups>
            <items>
                <item>
                    <name>Temperatura Calibrada</name>
                    <key>zone.temperature</key>
                    <type>SNMPv2 agent</type>
                    <snmp_oid>.1.3.6.1.4.1.49760.2.1</snmp_oid>
                    <units>°C</units>
                    <value_type>Numeric (float)</value_type>
                    <multiplier>0.01</multiplier>
                    <delay>60s</delay>
                </item>
                <item>
                    <name>Umidade Calibrada</name>
                    <key>zone.humidity</key>
                    <type>SNMPv2 agent</type>
                    <snmp_oid>.1.3.6.1.4.1.49760.2.2</snmp_oid>
                    <units>%</units>
                    <value_type>Numeric (float)</value_type>
                    <multiplier>0.01</multiplier>
                    <delay>60s</delay>
                </item>
                <item>
                    <name>WiFi RSSI</name>
                    <key>zone.rssi</key>
                    <type>SNMPv2 agent</type>
                    <snmp_oid>.1.3.6.1.4.1.49760.3.2</snmp_oid>
                    <units>dBm</units>
                    <value_type>Numeric (integer)</value_type>
                    <delay>5m</delay>
                </item>
                <item>
                    <name>Uptime do Sistema</name>
                    <key>zone.uptime</key>
                    <type>SNMPv2 agent</type>
                    <snmp_oid>.1.3.6.1.2.1.1.3.0</snmp_oid>
                    <units>centésimos de segundo</units>
                    <value_type>Numeric (unsigned)</value_type>
                    <delay>5m</delay>
                </item>
            </items>
        </template>
    </templates>
</zabbix_export>
```

### 3.2 Triggers Recomendados

| Trigger | Expressão | Severidade | Ação |
|---------|-----------|------------|------|
| **Temperatura alta** | `last(/ZoneMonitor/temp_calib) > 28` | High | Notificação por e-mail/Telegram |
| **Temperatura crítica** | `last(/ZoneMonitor/temp_calib) > 32` | Disaster | Alerta + escalonamento |
| **Umidade alta** | `last(/ZoneMonitor/hum_calib) > 80` | Warning | Notificação |
| **Sinal WiFi fraco** | `last(/ZoneMonitor/rssi) < -80` | Warning | Verificar posição do ESP |
| **Dispositivo reiniciou** | `last(/ZoneMonitor/uptime) < previous(/ZoneMonitor/uptime)` | Information | Registro log |
| **Sensor falhou** | `last(/ZoneMonitor/temp_raw) = 0` | High | Verificar hardware |

### 3.3 Gráficos e Dashboards

**Dashboard Zabbix sugerido:**

```
┌─────────────────────────────────────────────────────────────────┐
│                     ZoneMonitor - CPD Principal                 │
├─────────────────────────┬───────────────────────────────────────┤
│                         │                                       │
│   Temperatura Atual     │     Gráfico de Temperatura (24h)      │
│   ┌─────────────────┐   │   ┌─────────────────────────────────┐ │
│   │     24.5°C      │   │   │ 30 ┤                    ┌─────   │ │
│   │   Normal (20-28)│   │   │ 25 ┤     ┌──────┐      │         │ │
│   └─────────────────┘   │   │ 20 ┤─────┘      └──────┘         │ │
│                         │   └─────────────────────────────────┘ │
├─────────────────────────┼───────────────────────────────────────┤
│   Umidade Atual         │     Status SNMP                        │
│   ┌─────────────────┐   │   ┌─────────────────────────────────┐ │
│   │      58%        │   │   │ ✅ Agente ativo na porta 161     │ │
│   │   Normal (40-70)│   │   │ 📡 Comunidade: public            │ │
│   └─────────────────┘   │   └─────────────────────────────────┘ │
└─────────────────────────┴───────────────────────────────────────┘
```

---

## 4. Integração com Outras Ferramentas

### 4.1 PRTG

| Configuração | Valor |
|--------------|-------|
| **Sensor Type** | SNMP Custom |
| **OID** | `.1.3.6.1.4.1.49760.2.1` |
| **Community** | `public` |
| **Unit** | °C |
| **Scaling** | Dividir por 100 |

### 4.2 Nagios/Icinga

```bash
# Comando para check_temperature
define command {
    command_name    check_zone_temperature
    command_line    $USER1$/check_snmp -H $HOSTADDRESS$ -o .1.3.6.1.4.1.49760.2.1 -C public -P 2c -u '°C' -m '^([0-9]+)$' -w 2800 -c 3200
}
```

### 4.3 Grafana (via Prometheus SNMP Exporter)

```yaml
# snmp_exporter configuration
modules:
  zonemonitor:
    walk:
      - 1.3.6.1.4.1.49760.2.1  # temperature
      - 1.3.6.1.4.1.49760.2.2  # humidity
      - 1.3.6.1.4.1.49760.3.2  # rssi
    version: 2
    community: public
```

---

## 5. Comandos de Validação (CLI)

### 5.1 Varredura Completa

```bash
snmpwalk -v2c -c public 192.168.8.95 .1.3.6.1.4.1.49760
```

**Saída esperada:**
```
SNMPv2-SMI::enterprises.49760.1.3 = STRING: "ZoneMonitor-CPD"
SNMPv2-SMI::enterprises.49760.1.5 = STRING: "192.168.8.95"
SNMPv2-SMI::enterprises.49760.1.6 = STRING: "CC:50:E3:55:DB:62"
SNMPv2-SMI::enterprises.49760.2.1 = INTEGER: 2450
SNMPv2-SMI::enterprises.49760.2.2 = INTEGER: 5800
SNMPv2-SMI::enterprises.49760.3.2 = INTEGER: -52
```

### 5.2 Leitura de Temperatura Específica

```bash
snmpget -v2c -c public 192.168.8.95 .1.3.6.1.4.1.49760.2.1
```

**Saída esperada:**
```
SNMPv2-SMI::enterprises.49760.2.1 = INTEGER: 2450
```

### 5.3 Leitura do Nome do Dispositivo

```bash
snmpget -v2c -c public 192.168.8.95 .1.3.6.1.4.1.49760.1.3
```

### 5.4 Walk em todas as OIDs (MIB-2 + Enterprise)

```bash
snmpwalk -v2c -c public 192.168.8.95
```

### 5.5 Teste de Timeout (redes congestionadas)

```bash
snmpget -v2c -c public -t 5 -r 3 192.168.8.95 .1.3.6.1.4.1.49760.2.1
```

---

## 6. Dicas de Monitoramento Avançado

### 6.1 Detecção de Reboot

Monitore o `sysUpTime` (`.1.3.6.1.2.1.1.3.0`). Se o valor resetar, o dispositivo reiniciou.

**Exemplo de trigger no Zabbix:**
```
Expression: last(/ZoneMonitor/sysUptime) < previous(/ZoneMonitor/sysUptime)
Severity: Information
```

### 6.2 Saúde da RAM

Quedas constantes no Heap Livre (`.3.3`) podem indicar *memory leaks*.

| Padrão | Significado | Ação |
|--------|-------------|------|
| Queda gradual | Possível memory leak | Reiniciar dispositivo |