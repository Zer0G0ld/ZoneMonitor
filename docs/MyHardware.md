# Documentação de Hardware – ZoneMonitor v2.0

**Autor:** Zer0G0ld | Zer0 Tech Enterprise  
**Data:** 01/05/2026  
**Versão:** 2.0 (Produção - Revisão A)  
**Objetivo:** Documentar fiação, pinagem oficial, especificações técnicas e procedimentos de teste para o monitoramento de temperatura e umidade em CPDs.

---

## Índice

1. [Especificações Técnicas](#1-especificações-técnicas)
2. [Pinagem Oficial e Fiação](#2-pinagem-oficial-e-fiação)
   - 2.1 [Conexões do Sensor DHT11](#21-conexões-do-sensor-dht11)
   - 2.2 [Conexões do Display OLED (I2C)](#22-conexões-do-display-oled-i2c)
   - 2.3 [Mapa de GPIOs (Referência Rápida)](#23-mapa-de-gpios-referência-rápida)
3. [Diagnóstico de Hardware](#3-diagnóstico-de-hardware)
4. [Teste Rápido de Hardware (Checklist)](#4-teste-rápido-de-hardware-checklist-de-montagem)
5. [Notas de Manutenção](#5-notas-de-manutenção)
6. [Esquema de Montagem](#6-esquema-de-montagem-referência-rápida)
7. [Materiais Necessários para Instalação](#7-materiais-necessários-para-instalação)
8. [Especificações Elétricas](#8-especificações-elétricas)
9. [Dimensões e Encapsulamento](#9-dimensões-e-encapsulamento)
10. [Histórico de Revisões](#10-histórico-de-revisões)

---

## 1. Especificações Técnicas

| Componente | Especificação | Observação |
|------------|---------------|------------|
| **Microcontrolador** | NodeMCU ESP8266 (Lolin V3) | 4MB Flash, 80KB RAM |
| **Sensor de Temperatura** | DHT11 | Faixa: 0-50°C, precisão ±2°C (calibrado via software) |
| **Sensor de Umidade** | DHT11 | Faixa: 20-90%, precisão ±5% |
| **Display** | OLED 128x64 I2C (SSD1306) | Opcional, consumo ~20mA |
| **Feedback Visual** | LED interno (GPIO2/D4) | Pisca a cada 3 segundos indicando leitura ativa |
| **Conectividade** | Wi-Fi 802.11 b/g/n | 2.4GHz, suporte a WPA/WPA2 |
| **Protocolos** | HTTP, SNMP v2c | Portas: 80 (TCP), 161 (UDP) |
| **Alimentação** | USB 5V ou pino VIN | Consumo médio: 180mA |

---

## 2. Pinagem Oficial e Fiação

### 2.1 Conexões do Sensor DHT11

| Pino Sensor | Função | Pino NodeMCU | Cor Sugerida | Comprimento máximo |
|-------------|--------|--------------|--------------|-------------------|
| VCC | Alimentação (3.3V) | 3V | Vermelho | - |
| DATA | Sinal Digital | **D6 (GPIO12)** | Azul | 3 metros |
| GND | Terra | G (GND) | Preto | - |

> ⚠️ **Nota de Instalação em CPD:** 
> - Para distâncias superiores a 2 metros entre o sensor e o NodeMCU, utilize **cabo blindado**
> - Resistor pull-up de **10kΩ** (em vez do padrão 4.7kΩ) entre VCC e DATA para mitigar ruídos eletromagnéticos
> - Mantenha o sensor afastado de fontes de calor (ex: saída de ar de servidores)

### 2.2 Conexões do Display OLED (I2C)

| Pino OLED | Função | Pino NodeMCU | Cor Sugerida |
|-----------|--------|--------------|--------------|
| VCC | Alimentação (3.3V) | 3V | Vermelho |
| GND | Terra | G (GND) | Preto |
| SCL | Clock I2C | **D1 (GPIO5)** | Amarelo |
| SDA | Dados I2C | **D2 (GPIO4)** | Verde |

### 2.3 Mapa de GPIOs (Referência Rápida)

| GPIO | Pino Físico | Função no ZoneMonitor | Descrição |
|------|-------------|----------------------|-----------|
| GPIO4 | D2 | SDA (OLED) | Dados I2C do display |
| GPIO5 | D1 | SCL (OLED) | Clock I2C do display |
| GPIO12 | D6 | DATA (DHT11) | Sinal do sensor |
| GPIO2 | D4 | LED Interno | Indicador de atividade |

---

## 3. Diagnóstico de Hardware

O ZoneMonitor v2.0 monitora a integridade física através das seguintes métricas expostas via API e SNMP:

| Métrica | OID SNMP | Endpoint JSON | Descrição | Valor normal |
|---------|----------|---------------|-----------|--------------|
| Temp. Bruta | `.1.3.6.1.4.1.49760.3.1` | `temp_raw` | Leitura direta do sensor (sem offset) | 0 a 50°C |
| Temp. Calibrada | `.1.3.6.1.4.1.49760.3.3` | `temp_calib` | Valor final para o usuário | - |
| Umid. Calibrada | `.1.3.6.1.4.1.49760.3.6` | `hum_calib` | Umidade após offset | 20 a 90% |
| Sinal WiFi | `.1.3.6.1.4.1.49760.2.3` | `rssi` | Qualidade da conexão | Acima de -70dBm |
| Uptime | `.1.3.6.1.2.1.1.3.0` | `uptime` | Tempo de atividade (segundos) | - |
| Status SNMP | - | `snmp` | Agente ativo na porta 161 | `true` |

**Interpretação de valores anormais:**

| Valor | Significado | Ação necessária |
|-------|-------------|-----------------|
| `temp_raw = NaN` | Sensor não responde | Verificar cabo DATA (D6) e resistor pull-up |
| `temp_raw = 0` | Curto-circuito | Verificar alimentação do sensor |
| `rssi < -80dBm` | Sinal WiFi fraco | Reposicionar ESP ou adicionar antena externa |
| `snmp = false` | SNMP não iniciou | Verificar porta 161 (outro serviço?) |

---

## 4. Teste Rápido de Hardware (Checklist de Montagem)

Após finalizar a fiação, realize os testes abaixo **antes** da instalação no rack:

| Passo | Ação | Resultado Esperado | Se falhar |
|-------|------|---------------------|-----------|
| **1** | Ligar cabo USB no NodeMCU | LED interno pisca uma vez no boot | Verificar fonte/cabo USB |
| **2** | Observar o Display OLED | Exibe "Zer0 Tech" e "Conectando WiFi..." | Verificar D1, D2, 3V, GND |
| **3** | Aguardar conexão WiFi | Display mostra o IP obtido | Verificar credenciais no código |
| **4** | Validar leitura do sensor | Após 10s, temperatura aparece no OLED | Verificar D6 e resistor pull-up |
| **5** | Consultar API | `http://[IP]/status` retorna JSON válido | Verificar firewall |
| **6** | Testar SNMP | `snmpget -v2c -c public [IP] 1.3.6.1.4.1.49760.3.3` | Verificar porta 161 UDP |

**Comando para teste SNMP (Linux/WSL):**
```bash
snmpget -v2c -c public 192.168.8.95 1.3.6.1.4.1.49760.3.3
```

**Saída esperada:**
```
SNMPv2-SMI::enterprises.49760.3.3 = INTEGER: 2450
```
(2450 = 24.50°C)

---

## 5. Notas de Manutenção

### Problemas Comuns e Soluções

| Problema | Provável Causa | Solução | Tempo estimado |
|----------|----------------|---------|----------------|
| **Leituras NaN ou 0** | Falha no pino D6 | Verificar continuidade do cabo azul e resistor pull-up | 5 min |
| **OLED apagado** | Pinos D1/D2 invertidos | Confirmar SCL em D1, SDA em D2 | 2 min |
| **Instabilidade SNMP** | Firewall bloqueando UDP | Verificar porta 161 aberta na rede | 10 min |
| **Reboots frequentes** | Fonte USB insuficiente | Usar fonte de pelo menos 1A | - |
| **WiFi não conecta** | SSID/senha incorretos | Verificar credenciais no código | 2 min |
| **Temperatura irreal** | Offset incorreto | Ajustar via web: `http://[IP]/config` | 1 min |
| **Display com caracteres estranhos** | Endereço I2C errado | Trocar `0x3C` para `0x3D` no código | 2 min |
| **ESP não liga** | Curto-circuito | Verificar alimentação e soldas | 15 min |

### Manutenção Preventiva

| Frequência | Ação |
|------------|------|
| **Mensal** | Verificar se o uptime não reiniciou inesperadamente |
| **Trimestral** | Limpar poeira do sensor DHT11 |
| **Semestral** | Testar SNMP e calibrar offset se necessário |
| **Anual** | Substituir cabo USB se estiver desgastado |

---

## 6. Esquema de Montagem (Referência Rápida)

```
Diagrama de conexões:

NodeMCU ESP8266                    DHT11              OLED
┌─────────────┐                   ┌─────┐            ┌─────┐
│             │                   │     │            │     │
│ 3V  ────────┼───────────────────┤ VCC │            │ VCC │
│             │                   │     │            │     │
│ D6  ────────┼───────────────────┤ DATA│            │     │
│             │                   │     │            │     │
│ G   ────────┼───────────────────┤ GND │            │ GND │
│             │                   │     │            │     │
│ D1  ────────┼───────────────────────────────────────┤ SCL │
│             │                                       │     │
│ D2  ────────┼───────────────────────────────────────┤ SDA │
│             │                                       │     │
│ 3V  ────────┼───────────────────────────────────────┤ VCC │
│             │                                       │     │
│ G   ────────┼───────────────────────────────────────┤ GND │
│             │                                       │     │
└─────────────┘                   └─────┘            └─────┘

                    ┌─────────────────┐
                    │  Resistor 4.7kΩ │
                    │  a 10kΩ         │
                    └────────┬────────┘
                             │
                    Entre VCC e DATA
                    (pinos 1 e 3 do DHT11)

Componentes:
- NodeMCU: Alimentação USB 5V
- DHT11: Sensor sem solda (conector fêmea)
- OLED: Display opcional (remova da montagem se não usado)
```

---

## 7. Materiais Necessários para Instalação

### Kit Básico (1 unidade)

| Item | Quantidade | Especificação | Fornecedor sugerido |
|------|------------|---------------|---------------------|
| NodeMCU ESP8266 | 1 | Lolin V3 ou V2 | RoboCore / FilipeFlop |
| Sensor DHT11 | 1 | 3 pinos ou 4 pinos | RoboCore |
| Display OLED | 1 | 128x64 I2C (opcional) | RoboCore |
| Resistor | 1 | 4.7kΩ a 10kΩ | Qualquer loja de eletrônica |
| Cabos jumper | 6 | Fêmea-Fêmea, 20cm | Qualquer loja |
| Caixa para montagem | 1 | Acrílica ou ABS (opcional) | - |

### Kit para CPD (10 unidades)

| Item | Quantidade | Observação |
|------|------------|------------|
| NodeMCU ESP8266 | 10 | Comprar com garantia |
| Sensor DHT11 | 12 | 2 de reserva |
| Cabo USB longo | 10 | 1.5m a 2m |
| Fonte USB | 10 | 5V/1A mínimo |

---

## 8. Especificações Elétricas

| Parâmetro | Min | Típico | Max | Unidade |
|-----------|-----|--------|-----|---------|
| Tensão de alimentação | 4.5 | 5.0 | 5.5 | V |
| Consumo (WiFi + sensor) | - | 180 | 250 | mA |
| Consumo (WiFi off) | - | 70 | 100 | mA |
| Tensão lógica (GPIOs) | 3.0 | 3.3 | 3.6 | V |
| Corrente GPIO (máx) | - | - | 12 | mA |
| Nível alto (DATA DHT11) | 2.4 | 3.3 | 3.6 | V |
| Nível baixo (DATA DHT11) | 0 | 0 | 0.8 | V |

**Recomendação de fonte:** Carregador USB de celular (5V/1A a 2A) com bom isolamento.

---

## 9. Dimensões e Encapsulamento

| Componente | Largura (mm) | Altura (mm) | Profundidade (mm) |
|------------|--------------|-------------|-------------------|
| NodeMCU Lolin V3 | 58 | 18 | 32 |
| Sensor DHT11 | 15 | 18 | 6 |
| Display OLED | 27 | 27 | 4 |

**Recomendação de caixa para CPD:** 
- Caixa ABS 80x50x30mm
- Furos para ventilação do sensor
- Suporte para fixação em trilho DIN (opcional)

---

## 10. Histórico de Revisões

| Versão | Data | Autor | Alterações |
|--------|------|-------|------------|
| 1.0 | 29/10/2025 | Zer0 | Versão inicial com fiação experimental |
| 1.1 | 30/10/2025 | Zer0G0ld | Correção de pinos baseada em testes |
| 2.0 | 01/05/2026 | Zer0G0ld | Revisão A: oficiação da pinagem, resistor pull-up, testes de validação |
| 2.1 | 01/05/2026 | Zer0 Tech | Adicionadas especificações elétricas e checklist |

---

## Apêndice A: Código de Teste Rápido

Para validar o hardware antes de carregar o firmware completo, utilize este sketch mínimo:

```cpp
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DHTPIN D6
#define DHTTYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(D2, D1);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED não encontrado!");
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Teste Hardware");
  display.display();
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  
  display.clearDisplay();
  display.setCursor(0, 0);
  
  if (isnan(t) || isnan(h)) {
    display.println("ERRO: DHT11");
    Serial.println("DHT11 falhou!");
  } else {
    display.print("Temp: ");
    display.print(t, 1);
    display.println("C");
    display.print("Umid: ");
    display.print(h, 1);
    display.println("%");
    Serial.printf("T=%.1f U=%.1f\n", t, h);
  }
  
  display.display();
  delay(2000);
}
```

---

**Desenvolvido com 💻 e 🔧 pela Zer0 Tech Enterprise**  
*Monitoramento profissional de infraestrutura para ambientes críticos*

---

**Documento oficial – Proibida reprodução parcial sem autorização**
