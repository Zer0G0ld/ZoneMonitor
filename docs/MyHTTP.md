## ZoneMonitor ESP8266 HTTP API

**Autor:** Zer0G0ld | **Zer0 Tech Enterprise**  
**Data:** 30/04/2026  
**Versão:** 2.0  
**Público-alvo:** Desenvolvedores, Sysadmins, Integradores de sistemas  
**Objetivo:** Documentar a API HTTP do ZoneMonitor para consumo de dados, configuração remota e integração com sistemas de monitoramento.

---

## 1. Visão Geral da API

O ZoneMonitor v2.0 expõe uma **API REST leve** via HTTP na porta 80.  
Todos os endpoints retornam dados em formato **JSON (RFC 8259)** ou texto simples, garantindo compatibilidade com Python, JavaScript, Node-RED, Home Assistant, Zabbix (via script), e qualquer outra ferramenta que suporte HTTP.

| Método | Endpoint | Descrição |
|--------|----------|-----------|
| GET | `/status` | Retorna JSON completo com todos os dados do sistema |
| GET | `/config` | Dashboard HTML (para humanos) |
| POST | `/config` | Salva offsets e nome do dispositivo |
| GET | `/reboot` | Reinicia o ESP8266 |

---

## 2. Endpoint Principal: `/status` (JSON)

### 2.1 Requisição

```http
GET http://<ESP_IP>/status
```

### 2.2 Resposta (Exemplo)

```json
{
  "temp_raw": 27.3,
  "temp_calib": 24.3,
  "temp_offset": -3.0,
  "hum_raw": 61.0,
  "hum_calib": 61.0,
  "hum_offset": 0.0,
  "ip": "192.168.8.95",
  "mac": "CC:50:E3:55:DB:62",
  "rssi": -52,
  "ssid": "Ohost",
  "device": "ZoneMonitor-CPD",
  "uptime": 86400,
  "snmp": true
}
```

### 2.3 Campos JSON (Detalhados)

| Campo | Tipo | Descrição | Uso recomendado |
|-------|------|-----------|-----------------|
| `temp_raw` | float | Leitura bruta do DHT11 (sem offset) | Diagnóstico de hardware, verificar necessidade de recalibração |
| `temp_calib` | float | **Temperatura final** após aplicar offset | **Alertas, dashboards, exibição** |
| `temp_offset` | float | Valor de calibração aplicado | Auditoria, debugging |
| `hum_raw` | float | Umidade bruta | Diagnóstico |
| `hum_calib` | float | **Umidade final** após offset | **Exibição, alertas** |
| `hum_offset` | float | Offset de umidade | Auditoria |
| `ip` | string | IP atual do dispositivo | Inventário, descoberta de rede |
| `mac` | string | MAC Address | Identificação única, DHCP reservation |
| `rssi` | int | Sinal WiFi (dBm) | Monitoramento de conectividade |
| `ssid` | string | Rede WiFi conectada | Diagnóstico |
| `device` | string | Nome personalizado do dispositivo | Identificação em dashboards |
| `uptime` | int | Tempo ligado (segundos) | Monitoramento de estabilidade |
| `snmp` | bool | Status do agente SNMP (porta 161) | Verificar se SNMP está ativo |

---

## 3. Endpoint de Configuração: `/config`

### 3.1 GET – Dashboard HTML

```http
GET http://<ESP_IP>/config
```

Retorna página HTML com formulário de calibração.  
**Autenticação HTTP Basic:** `admin` / `1234` (configurável via LittleFS)

### 3.2 POST – Atualizar Configurações

```http
POST http://<ESP_IP>/config
Content-Type: application/x-www-form-urlencoded
```

**Parâmetros:**

| Parâmetro | Tipo | Exemplo | Descrição |
|-----------|------|---------|-----------|
| `temp_offset` | float | `-3.0` | Offset para temperatura (Real - Sensor) |
| `hum_offset` | float | `0.0` | Offset para umidade |
| `device_name` | string | `CPD-Sul` | Nome do dispositivo (max 31 chars) |

**Resposta de sucesso:**
```
HTTP/1.1 200 OK
Content-Type: text/plain

Config salva. Reinicie para aplicar.
```

**Observação:** Após o POST, **o ESP não reinicia automaticamente**. É necessário chamar `/reboot` para aplicar as novas configurações.

---

## 4. Endpoint de Reinicialização: `/reboot`

```http
GET http://<ESP_IP>/reboot
```

**Resposta:**
```
HTTP/1.1 200 OK
Content-Type: text/plain

Reiniciando...
```

O ESP reinicia após 500ms. A conexão será interrompida.

---

## 5. Exemplos de Integração

### 5.1 cURL – Consultar status

```bash
curl -s http://192.168.8.95/status | jq '.temp_calib'
```

### 5.2 cURL – Calibrar remotamente

```bash
# Ajustar offset para -3.0 graus
curl -X POST http://192.168.8.95/config \
  -d "temp_offset=-3.0" \
  -d "hum_offset=0.0" \
  -d "device_name=CPD-Sul"

# Reiniciar para aplicar
curl http://192.168.8.95/reboot
```

### 5.3 Python – Coleta de dados

```python
import requests
import json

response = requests.get('http://192.168.8.95/status')
data = response.json()

print(f"Temperatura: {data['temp_calib']}°C")
print(f"Umidade: {data['hum_calib']}%")
print(f"SNMP Ativo: {data['snmp']}")
```

### 5.4 Home Assistant (MQTT alternativo via REST)

```yaml
sensor:
  - platform: rest
    name: "ZoneMonitor Temperatura"
    resource: http://192.168.8.95/status
    value_template: "{{ value_json.temp_calib }}"
    unit_of_measurement: "°C"
    scan_interval: 30

  - platform: rest
    name: "ZoneMonitor Umidade"
    resource: http://192.168.8.95/status
    value_template: "{{ value_json.hum_calib }}"
    unit_of_measurement: "%"
    scan_interval: 30
```

### 5.5 Node-RED (HTTP Request node)

- **Method:** GET
- **URL:** `http://192.168.8.95/status`
- **Output:** `msg.payload.temp_calib`

### 5.6 JavaScript (Frontend – atualização periódica)

```javascript
async function atualizarDados() {
  const response = await fetch('http://192.168.8.95/status');
  const data = await response.json();
  
  document.getElementById('temperatura').innerText = data.temp_calib + '°C';
  document.getElementById('umidade').innerText = data.hum_calib + '%';
}

setInterval(atualizarDados, 2000);
```

---

## 6. Considerações Técnicas

| Item | Detalhe |
|------|---------|
| **Porta TCP** | 80 |
| **Formato JSON** | RFC 8259 compatível |
| **CORS** | Não implementado (necessário proxy se for acessar via frontend externo) |
| **WebSockets** | ❌ Não suportado (economia de IRAM - 93% utilizado) |
| **Autenticação** | HTTP Basic Auth no `/config` (opcional, configurável) |
| **Persistência** | Offsets salvos no LittleFS, mantidos após reboot |
| **Limitação** | Máximo de 3-5 requisições simultâneas devido ao IRAM limitado |

---

## 7. Fluxo de Calibração para Integração Automática

1. **Consultar offset atual:**  
   `GET /status` → campo `temp_offset`

2. **Calcular novo offset:**  
   `novo_offset = temperatura_real - temp_calib`

3. **Enviar novo offset:**  
   `POST /config` com `temp_offset=novo_valor`

4. **Reiniciar dispositivo:**  
   `GET /reboot`

5. **Aguardar 10 segundos e verificar:**  
   `GET /status` → confirmar `temp_calib` correto

---

## 8. Diferenças entre Dashboard HTML e API JSON

| Característica | `/config` (HTML) | `/status` (JSON) |
|----------------|------------------|------------------|
| Público | Humanos | Máquinas / Scripts |
| Update em tempo real | Recarregar página | Script externo com `setInterval` |
| Autenticação | HTTP Basic | Sem autenticação (somente leitura) |
| Uso de CPU no ESP | Alto (serve página) | Baixo (apenas JSON) |

> 💡 **Recomendação para dashboards em TV no CPD:**  
> Implemente um servidor intermediário (Node-RED, Python) que consome `/status` a cada 2 segundos e exibe os dados em uma página web. Isso evita sobrecarregar o ESP com múltiplos clientes e respeita o limite de IRAM.

---

**Desenvolvido por Zer0G0ld para Zer0 Tech Enterprise**  
*API REST para monitoramento profissional de CPD*