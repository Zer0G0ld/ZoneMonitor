# 🖥️ ZoneMonitor CPD

Documentação das funcionalidades e estrutura do dashboard do ZoneMonitor CPD.

---

## 1. Monitoramento de Sensores 🌡️💧

| Sensor       | Valor Atual | Status       | Histórico (opcional) |
|-------------|------------|-------------|---------------------|
| Temperatura | 25.4°C     | ✅ OK        | Últimos valores / gráfico |
| Umidade     | 62%        | ✅ OK        | Últimos valores / gráfico |
| Sensor      | -          | ⚠️ Erro     | —                   |

**Funcionalidades:**
- Leitura em tempo real (2s)
- Alerta visual se sensor offline ou leitura inválida
- Possibilidade de adicionar novos sensores dinamicamente

---

## 2. Rede / Conectividade 🌐

| Item       | Valor             | Status       |
|-----------|-----------------|-------------|
| IP Local   | 192.168.1.100   | ✅ Conectado |
| MAC        | 5C:CF:7F:12:34:56 | ✅           |
| SSID       | Ohost           | ✅ Conectado |
| RSSI       | -70 dBm         | ⚠️ Fraco    |
| Ping       | 2ms             | ✅ OK        |

---

## 3. SNMP 📡

| Parâmetro        | Valor         | Status       |
|-----------------|--------------|-------------|
| Agente           | Ativo / Inativo | ✅ / ⚠️      |
| Community        | public       | ✅           |
| Porta            | 161          | ✅           |
| Uptime           | 1d 3h 20m    | ✅           |

---

## 4. Sistema / Dispositivo 🖧

| Informação       | Valor                        |
|-----------------|------------------------------|
| Nome do dispositivo | ESP-ZoneMonitor           |
| Localização        | CPD                        |
| Autor / Repo       | Zer0 / [GitHub](https://github.com/Zer0G0ld/ZoneMonitor) |
| Uptime geral       | 1d 3h 20m                  |
| Logs do sistema    | Últimos 10 eventos         |

**Ações:**
- 🔄 Reiniciar dispositivo remotamente
- ⚠️ Visualizar logs de erro e status

---

## 5. Dashboard Dinâmico / Widgets 🎛️

- Cards de sensores, rede, SNMP e sistema
- Atualização em tempo real (2s)
- Indicadores de alerta (cores e animações)
- Flexível para adicionar novos widgets sem recompilar

---

## 6. Configurações ⚙️

| Configuração           | Opções / Valores                   |
|------------------------|-----------------------------------|
| WiFi                   | SSID, Senha                        |
| Autenticação HTTP       | Usuário / Senha                    |
| SNMP                   | Community, Porta, Ativar/Desativar |
| Atualização do dashboard| Intervalo (s), cores, unidades     |
| Widgets                | Adicionar / Remover / Reorganizar |
| Reset / Reboot         | ✅ Botão físico ou web             |

---

## 7. Páginas do Dashboard 📄

- **Dashboard principal** – Gráficos, status, widgets
- **Informações do dispositivo** – IP, MAC, SNMP, Logs
- **Configurações avançadas** – Rede, SNMP, widgets
- **Reboot / manutenção** – Reiniciar ou resetar
- **Histórico / Logs** (opcional) – Últimos eventos e medições

---

## 8. Extras / Futuro 🚀

- Alertas por email ou SNMP trap
- Exportação de dados (CSV / JSON)
- Backup e restore da configuração
- Suporte a novos sensores ou módulos plugáveis
- Modo noturno / cores customizáveis