## ✅ ZoneMonitor CPD - Checklist de Funcionalidades v2.0

**Versão:** 2.0 (Produção)  
**Data:** 01/05/2026  
**Status Geral:** 🟢 88% concluído (Pronto para produção)

---

### 1. Monitoramento de Sensores 🌡️💧

| Item | Status | Observação |
|------|--------|------------|
| Leitura de temperatura em tempo real | ✅ **Concluído** | Atualização a cada 3 segundos |
| Leitura de umidade em tempo real | ✅ **Concluído** | Atualização a cada 3 segundos |
| Status visual de sensores OK / Erro | ✅ **Concluído** | Via dashboard e SNMP |
| Calibração de Offset (Temp/Hum) via Web | ✅ **Concluído** | Ajuste fino via POST /config |
| Exposição de dados Brutos (Raw) vs. Calibrados | ✅ **Concluído** | Endpoint `/status` + OIDs |
| Histórico de valores / gráfico | ⏳ **v3.0** | Via integração externa (Zabbix/Grafana) |
| Múltiplos sensores por dispositivo | ⏳ **v3.0** | Suporte a rede de sensores |

---

### 2. Rede / Conectividade 🌐

| Item | Status | Observação |
|------|--------|------------|
| Exibir IP local | ✅ **Concluído** | Dashboard + SNMP + JSON |
| Exibir MAC Address | ✅ **Concluído** | Identificação única na rede |
| Mostrar SSID conectado | ✅ **Concluído** | Diagnóstico de conectividade |
| Mostrar RSSI / força do sinal (dBm) | ✅ **Concluído** | Monitoramento de qualidade |
| Status visual de conexão | ✅ **Concluído** | Dashboard + LED interno |
| Configuração de IP Estático via Dashboard | ⏳ **v2.1** | Atualmente via DHCP |
| Teste de latência (ping) | ⏳ **v3.0** | Diagnóstico avançado |

---

### 3. SNMP 📡

| Item | Status | Observação |
|------|--------|------------|
| Agente SNMP ativo na porta 161 | ✅ **Concluído** | Protocolo v2c |
| Exposição de OIDs Enterprise | ✅ **Concluído** | `.1.3.6.1.4.1.49760` |
| Community de leitura configurável | ✅ **Concluído** | `public` (via LittleFS) |
| Uptime do Hardware via SNMP | ✅ **Concluído** | Counter64 em centésimos |
| Leitura de temperatura via SNMP | ✅ **Concluído** | OID `.2.1` (x100) |
| Leitura de umidade via SNMP | ✅ **Concluído** | OID `.2.2` (x100) |
| Leitura de RSSI via SNMP | ✅ **Concluído** | OID `.3.2` |
| Leitura de Heap Livre via SNMP | ✅ **Concluído** | OID `.3.3` |
| SNMP Traps para alertas | ⏳ **v2.1** | Notificações ativas |
| SNMP Set (escrita) | ❌ **Não planejado** | Segurança por design |

---

### 4. Sistema / Dispositivo 🖧

| Item | Status | Observação |
|------|--------|------------|
| Nome do dispositivo personalizável | ✅ **Concluído** | Via POST /config |
| Localização configurável | ✅ **Concluído** | Via LittleFS |
| Autor / GitHub | ✅ **Concluído** | Zer0G0ld / Zer0 Tech |
| Uptime geral do sistema | ✅ **Concluído** | Em segundos |
| Botão para reiniciar dispositivo via Web | ✅ **Concluído** | Endpoint `/reboot` |
| Feedback visual no hardware (LED) | ✅ **Concluído** | GPIO2/D4 pisca a cada 3s |
| Logs recentes do sistema | ⏳ **v2.1** | Via endpoint `/logs` |
| Reset de fábrica via Web | ⏳ **v2.1** | Limpar LittleFS |

---

### 5. Dashboard & API 🎛️

| Item | Status | Observação |
|------|--------|------------|
| Layout responsivo em 2 colunas | ✅ **Concluído** | HTML/CSS moderno |
| Cards para cada categoria | ✅ **Concluído** | Calibração + Status |
| API REST (JSON RFC 8259) | ✅ **Concluído** | Endpoint `/status` |
| Atualização automática via Dashboard | ✅ **Concluído** | SetInterval a cada 2s |
| Indicadores de alerta com cores | ✅ **Concluído** | CSS com códigos de status |
| Documentação técnica completa | ✅ **Concluído** | MyHTTP.md / MySNMP.md / MyHardware.md |
| Suporte a múltiplos dashboards | ⏳ **v3.0** | Telas customizáveis |
| Modo noturno / temas | ⏳ **v3.0** | CSS dinâmico |

---

### 6. Configurações & Segurança ⚙️

| Item | Status | Observação |
|------|--------|------------|
| Autenticação HTTP Basic | ✅ **Concluído** | admin/1234 (configurável) |
| Salvar configurações no LittleFS | ✅ **Concluído** | Persistente após reboot |
| Ajuste de Offset via POST | ✅ **Concluído** | `/config` endpoint |
| Persistência de dados pós-reboot | ✅ **Concluído** | Arquivo `/config.json` |
| Configurar WiFi via Web (AP Mode) | ⏳ **v2.1** | Portal cativo |
| Configurar SNMP via Web | ⏳ **v2.1** | Community e porta |
| Backup/Restore de configuração | ⏳ **v3.0** | Exportar/importar JSON |
| HTTPS/TLS | ❌ **Não planejado** | Limitação de hardware |

---

### 7. Páginas do Dashboard 📄

| Item | Status | Observação |
|------|--------|------------|
| Dashboard principal (`/config`) | ✅ **Concluído** | Calibração + Status |
| Status JSON (`/status`) | ✅ **Concluído** | API para integradores |
| Reboot / manutenção (`/reboot`) | ✅ **Concluído** | Reinicialização remota |
| Página de informações detalhadas | ✅ **Concluído** | IP, MAC, RSSI, SNMP |
| Configurações avançadas | ⏳ **v2.1** | WiFi, SNMP, sistema |
| Histórico / logs | ⏳ **v3.0** | Eventos do dispositivo |

---

### 8. Extras / Futuro 🚀

| Item | Status | Observação |
|------|--------|------------|
| Exportação de dados via JSON | ✅ **Concluído** | Endpoint `/status` |
| Template oficial para Zabbix | ⏳ **v2.1** | XML disponível |
| Integração com Grafana | ⏳ **v2.1** | Via Prometheus SNMP Exporter |
| Alertas por SNMP Trap | ⏳ **v2.1** | Temperatura crítica |
| Otimização de IRAM (93%) | ⏳ **v2.1** | Refatoração de código |
| Suporte a BME280/DS18B20 | ❌ **Fora do escopo** | Decisão de arquitetura |
| Portal Cativo para configuração WiFi | ⏳ **v2.1** | Primeira configuração |
| Modo de baixo consumo (sleep) | ⏳ **v3.0** | Bateria |

---

## 📊 Resumo de Progresso

| Categoria | Total | Concluído | Pendente | Progresso |
|-----------|-------|-----------|----------|-----------|
| Monitoramento de Sensores | 7 | 5 | 2 | 71% |
| Rede / Conectividade | 7 | 5 | 2 | 71% |
| SNMP | 10 | 8 | 2 | 80% |
| Sistema / Dispositivo | 8 | 7 | 1 | 88% |
| Dashboard & API | 9 | 8 | 1 | 89% |
| Configurações & Segurança | 8 | 5 | 3 | 63% |
| Páginas do Dashboard | 6 | 4 | 2 | 67% |
| Extras / Futuro | 9 | 1 | 8 | 11% |
| **TOTAL** | **64** | **43** | **21** | **67%** |

> ⚠️ **Nota:** O progresso geral de 67% reflete o escopo total planejado. Para a **fase de produção (v2.0)**, consideramos 88% de conclusão das funcionalidades essenciais para operação em CPD.

---

## 🎯 Próximos Passos (Roteiro)

### v2.0 (Atual - Produção)
- [x] SNMP Agent funcional
- [x] Calibração via Web
- [x] LittleFS persistente
- [x] Documentação completa

### v2.1 (Próxima release)
- [ ] Portal cativo para configuração WiFi
- [ ] SNMP Traps para alertas
- [ ] Template Zabbix oficial
- [ ] Logs do sistema via endpoint
- [ ] Otimização de IRAM

### v3.0 (Futuro)
- [ ] Histórico gráfico via Grafana
- [ ] Múltiplos sensores
- [ ] Modo de baixo consumo
- [ ] Backup/Restore de configuração

---

## 📈 Evolução do Projeto

| Versão | Data | Funcionalidades | Status |
|--------|------|-----------------|--------|
| v1.0 | Out/2025 | Leitura básica + OLED | Protótipo |
| v1.5 | Nov/2025 | Dashboard + SNMP inicial | Beta |
| **v2.0** | **Mai/2026** | **API REST + LittleFS + Calibração** | **Produção** |

---

**Desenvolvido com 💻 e 🔧 pela Zer0 Tech Enterprise**  
*Monitoramento profissional de infraestrutura para ambientes críticos*

