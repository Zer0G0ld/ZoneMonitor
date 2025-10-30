# 📄 Documentação Técnica – SNMP no ZoneMonitor

**Dispositivo:** ESP8266 + DHT11 + OLED + SNMP + HTTP
**Versão:** 1.4
**Autor:** Zer0
**Data:** 30/10/2025
**Objetivo:** Expor informações do sensor e do dispositivo via SNMP, podendo ser monitorado por Zabbix, TheDude ou outras ferramentas de gerenciamento de rede.

---

## 1️⃣ O que é SNMP e como funciona aqui

SNMP (Simple Network Management Protocol) é um protocolo usado para **monitorar e gerenciar dispositivos em rede**. No ZoneMonitor:

* O ESP8266 atua como **Agente SNMP**.
* Ele expõe **OIDs** (Object Identifiers) que representam informações do dispositivo.
* O agente escuta requisições **UDP** na porta SNMP padrão (161).
* As informações podem ser **lidas (GET)** ou contadas (Counter64), mas não há **setters**, ou seja, você não configura parâmetros via SNMP, apenas lê.

---

## 2️⃣ Estrutura de OIDs

### 2.1 MIB-2 padrão

Base padrão usada em quase todo dispositivo SNMP:

| OID                  | Descrição | Valor no ZoneMonitor                  |
| -------------------- | --------- | ------------------------------------- |
| `.1.3.6.1.2.1.1.1.0` | sysDescr  | `"ESP8266 ZoneMonitor"`               |
| `.1.3.6.1.2.1.1.3.0` | sysUpTime | Uptime do HW em centésimos de segundo |
| `.1.3.6.1.2.1.1.5.0` | sysName   | `"ESP-ZoneMonitor"`                   |

### 2.2 Enterprise OIDs customizados

Todos criados sob `.1.3.6.1.4.1.49760`:

| OID                       | Descrição           | Tipo   |
| ------------------------- | ------------------- | ------ |
| `.1.3.6.1.4.1.49760.1.1`  | Criador             | String |
| `.1.3.6.1.4.1.49760.1.2`  | GitHub Repo         | String |
| `.1.3.6.1.4.1.49760.1.3`  | Localização         | String |
| `.1.3.6.1.4.1.49760.1.4`  | Descrição           | String |
| `.1.3.6.1.4.1.49760.1.5`  | Nome do dispositivo | String |
| `.1.3.6.1.4.1.49760.1.6`  | IP                  | String |
| `.1.3.6.1.4.1.49760.1.7`  | MAC                 | String |
| `.1.3.6.1.4.1.49760.1.8`  | SSID Wi-Fi          | String |
| `.1.3.6.1.4.1.49760.1.9`  | HTTP URL            | String |
| `.1.3.6.1.4.1.49760.1.10` | SNMP public         | String |
| `.1.3.6.1.4.1.49760.1.11` | SNMP private        | String |

### 2.3 OIDs de sensores e monitoramento

| OID                      | Descrição                            | Tipo      |
| ------------------------ | ------------------------------------ | --------- |
| `.1.3.6.1.4.1.49760.2.1` | Temperatura (°C)                     | Integer   |
| `.1.3.6.1.4.1.49760.2.2` | Umidade (%)                          | Integer   |
| `.1.3.6.1.4.1.49760.3.1` | Contador de falhas do sensor         | Integer   |
| `.1.3.6.1.4.1.49760.3.2` | RSSI Wi-Fi (dBm)                     | Integer   |
| `.1.3.6.1.4.1.49760.3.3` | Heap livre (bytes)                   | Integer   |
| `.1.3.6.1.4.1.49760.3.4` | Status HTTP (1 = ativo, 0 = inativo) | Integer   |
| `.1.3.6.1.4.1.49760.3.5` | Uptime HW (centésimos de segundo)    | Counter64 |

---

## 3️⃣ Funcionamento interno

1. **Setup SNMP** (`setupSNMP()`):

   * Configura o agente SNMP com `public` e `private`.
   * Registra OIDs com `addReadOnlyStaticStringHandler` ou `addIntegerHandler`.
   * `sortHandlers()` organiza internamente os OIDs.
   * `snmp.begin()` inicia o agente.

2. **Loop principal** (`loop()`):

   * `snmp.loop()` processa requisições UDP de clientes SNMP.
   * Atualiza variáveis SNMP a cada 10 segundos:

     * Temperatura e umidade
     * Contador de falhas do sensor
     * RSSI, heap livre, status HTTP, uptime

3. **Atualização do Display/OLED**:

   * A cada 2 segundos, atualiza a tela e lê sensor DHT11.
   * Incrementa contador de falhas se sensor retornar NAN.

---

## 4️⃣ Como testar com SNMP

Você precisa de um **cliente SNMP**, por exemplo `snmpwalk` ou `snmpget` em Linux/Windows.

### 4.1 Comando básico

```bash
snmpwalk -v2c -c public 192.168.1.100
```

* `-v2c` → SNMP v2c (compatível com nosso agente)
* `-c public` → comunidade pública
* `192.168.1.100` → IP do ESP8266

### 4.2 Testar OIDs específicos

```bash
snmpget -v2c -c public 192.168.1.100 .1.3.6.1.4.1.49760.2.1
```

* Retorna **temperatura atual**.

Outros exemplos:

```bash
snmpget -v2c -c public 192.168.1.100 .1.3.6.1.4.1.49760.3.2   # RSSI
snmpget -v2c -c public 192.168.1.100 .1.3.6.1.4.1.49760.3.3   # Heap livre
```

### 4.3 Testar uptime

```bash
snmpget -v2c -c public 192.168.1.100 .1.3.6.1.2.1.1.3.0
```

* Retorna `sysUpTime` do dispositivo (centésimos de segundo).

---

## 5️⃣ Considerações importantes

1. **Comunidades SNMP**

   * `public` → leitura
   * `private` → normalmente usado para escrita, mas nosso agente não implementa write.

2. **Atualização**

   * Variáveis de sensor são atualizadas **a cada 10s para SNMP**, então não espere mudanças instantâneas.

3. **Segurança**

   * SNMP v2c é **sem criptografia**, só comunidade como senha.
   * Evite expor rede pública.

4. **SNMP vs HTTP**

   * HTTP é apenas visualização.
   * SNMP é para integração com sistemas de monitoramento (Zabbix, TheDude, PRTG).

5. **Integração em Zabbix**

   * Crie itens SNMP usando OIDs customizados.
   * Por exemplo, para temperatura:

     * Tipo de Item: SNMP v2
     * OID: `.1.3.6.1.4.1.49760.2.1`
     * Tipo de Dados: Numeric (float ou integer)
   * Para alarmes: use contador de falhas ou heap crítico.

---

## 6️⃣ Dicas de monitoramento avançado

* **Cronometrar uptime real**: use `sysUptime` e calcule diferença.
* **Falhas do sensor**: se `OID_SENSOR_FAIL` aumentar muito, considerar reset do DHT11.
* **Heap livre baixo**: importante para estabilidade do ESP8266.
* **RSSI Wi-Fi**: indica qualidade de conexão; menor que -80 dBm → sinal fraco.
* **HTTP Status**: pode indicar se o servidor web está respondendo ou travou.

---

# 🖼 Diagrama SNMP – ZoneMonitor
```
.1.3.6.1.2.1.1       (MIB-2 Standard)
├─ .1.3.6.1.2.1.1.1.0   sysDescr           → String   "ESP8266 ZoneMonitor"
├─ .1.3.6.1.2.1.1.3.0   sysUpTime          → Counter64 (centésimos de segundo)
└─ .1.3.6.1.2.1.1.5.0   sysName            → String   "ESP-ZoneMonitor"

.1.3.6.1.4.1.49760    (Enterprise ZoneMonitor)
├─ 1   Device Info (Strings)
│   ├─ .1   Creator          → String "Zer0"
│   ├─ .2   Repo             → String "https://github.com/Zer0G0ld/ZoneMonitor"
│   ├─ .3   Location         → String "CPD"
│   ├─ .4   Description      → String "ESP8266 ZoneMonitor"
│   ├─ .5   DeviceName       → String "ESP-ZoneMonitor"
│   ├─ .6   IP               → String "192.168.x.x"
│   ├─ .7   MAC              → String "AA:BB:CC:DD:EE:FF"
│   ├─ .8   SSID             → String "UUID"
│   ├─ .9   HTTP URL         → String "http://192.168.x.x"
│   ├─ .10  SNMP Public      → String "public"
│   └─ .11  SNMP Private     → String "private"

├─ 2   Sensor Data (Integers)
│   ├─ .1   Temperatura       → Integer (°C)
│   └─ .2   Umidade           → Integer (%)

└─ 3   Device Monitor (Integers / Counter64)
    ├─ .1   Sensor Fail Count → Integer
    ├─ .2   WiFi RSSI         → Integer (dBm)
    ├─ .3   Free Heap         → Integer (bytes)
    ├─ .4   HTTP Status       → Integer (1=online,0=offline)
    └─ .5   HW Uptime         → Counter64 (centésimos de segundo)
```

---

# 🔄 Fluxo de Atualização

1. **Loop principal do ESP8266**:

   * **OLED / Sensor DHT11**: Atualiza a cada 2 segundos.
   * **Variáveis SNMP**: Atualiza a cada 10 segundos (`snmpTemp`, `snmpHum`, `snmpRSSI`, `snmpHeap`, etc).

2. **Agente SNMP**:

   * Escuta requisições UDP na porta 161.
   * Responde GET requests com os valores atuais.
   * `Counter64` usado para uptime do hardware.

3. **Cliente SNMP externo**:

   * Pode usar `snmpwalk`, `snmpget`, ou sistemas como **Zabbix**, **PRTG**, **TheDude**.
   * Exemplo:

     ```bash
     snmpwalk -v2c -c public 192.168.1.100
     ```
   * Para OIDs específicos:

     ```bash
     snmpget -v2c -c public 192.168.1.100 .1.3.6.1.4.1.49760.2.1
     ```

---

# ⚡ Dicas Visuais

* OIDs de **Device Info** → Strings (nome, IP, MAC, HTTP, SSID).
* OIDs de **Sensor Data** → Integers (Temperatura e Umidade).
* OIDs de **Monitoramento / Health** → Integers ou Counter64 (falhas, heap, RSSI, uptime).
