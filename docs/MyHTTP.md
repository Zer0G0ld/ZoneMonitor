# MyHTTP.md – ZoneMonitor ESP8266 HTTP Dashboard

**Autor:** Zer0
**Data:** 29/10/2025
**Objetivo:** Documentar o funcionamento do HTTP Dashboard do ZoneMonitor, incluindo rotas, endpoints, dados e atualização automática.

---

## 1. Visão Geral

O ZoneMonitor possui um **HTTP Dashboard leve** acessível via navegador.
O ESP8266 atua como servidor HTTP (`ESP8266WebServer`), fornecendo:

* Página principal (`/`) com dashboard em HTML/CSS/JS.
* Endpoint de dados (`/data`) que retorna JSON atualizado do sensor DHT11.
* Atualização automática a cada 2 segundos via `fetch()` no JavaScript.

---

## 2. Rotas HTTP

### 2.1 `/` – Página Principal

* **Método:** GET
* **Descrição:** Retorna o HTML do dashboard.
* **Conteúdo:**

  * Informações de temperatura (`%TEMP%`) e umidade (`%HUM%`).
  * IP e MAC do ESP8266.
  * Dashboard estilizado com cards, cores e animações.
  * Atualização automática via JavaScript a cada 2s.

### 2.2 `/data` – Endpoint de Dados

* **Método:** GET
* **Descrição:** Retorna os valores atuais de temperatura e umidade em formato JSON.
* **Exemplo de resposta:**

```json
{
  "temp": 25.3,
  "hum": 60.1
}
```

* **Uso:** Consumido pelo JavaScript do dashboard para atualizar os valores em tempo real.

---

## 3. Estrutura HTML/CSS/JS

### 3.1 HTML

* Tags principais:

  * `<h2>`: Título do dashboard.
  * `<div class='dashboard'>`: Container principal.
  * `<div class='card'>`: Card com valores do sensor.
  * `<span id='temp'>` e `<span id='hum'>`: Valores atualizados dinamicamente.
  * `<div class='footer'>`: Observação sobre atualização automática.

### 3.2 CSS

* Dashboard responsivo, cores escuras (dark mode) e tipografia moderna.
* Animações:

  * `.pulse`: animação para ícones de temperatura e umidade.
  * `.status-dot` ou barra (opcional) para indicar atualização.

### 3.3 JavaScript

* Função `atualizarDados()`:

  * Faz `fetch('/data')`.
  * Atualiza `#temp` e `#hum` no HTML.
* Atualização contínua a cada 2 segundos com `setInterval(atualizarDados, 2000)`.

---

## 4. Uso Prático

1. Conecte o ESP8266 à rede Wi-Fi.
2. Abra o Serial Monitor para obter o **IP do dispositivo**.
3. Acesse o dashboard pelo navegador:

   ```
   http://<IP_DO_ESP8266>/
   ```
4. A página mostrará:

   * Temperatura e umidade em tempo real.
   * IP e MAC do dispositivo.
   * Atualização automática sem necessidade de recarregar a página.

---

## 5. Exemplo de Integração

* Aplicações externas podem consultar o endpoint `/data` para ler os valores via HTTP GET:

```bash
curl http://192.168.X.X/data
```

* Ideal para integrar com dashboards externos, sistemas de automação ou aplicações IoT.

---

## 6. Observações Técnicas

* O dashboard é servido diretamente pelo ESP8266 usando memória Flash (`PROGMEM`).
* O JSON é gerado dinamicamente via função `handleData()`.
* Nenhuma biblioteca externa de servidor web é necessária além do `ESP8266WebServer`.
