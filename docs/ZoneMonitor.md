# Documentação Técnica – ZoneMonitor ESP8266 + DHT11 + OLED

**Autor:** Zer0
**Data:** 29/10/2025
**Objetivo:** Documentar fiação, pinos e código de teste para sensor DHT11 com ESP8266 NodeMCU e display OLED.

---

## 1. Hardware Utilizado

* **Placa:** NodeMCU ESP8266
* **Sensor:** DHT11 (umidade e temperatura)
* **Display:** OLED 128x64 I2C
* **Cabos:** Diversos (preto, cinza, azul, vermelho, roxo, verde, amarelo)
* **Resistor pull-up:** recomendado 4.7kΩ a 10kΩ no pino DATA do DHT11

---

## 2. Pinos do NodeMCU

**Direita (de cima para baixo, microUSB embaixo):**

| Pino | Função |
| ---- | ------ |
| D0   | GPIO16 |
| D1   | GPIO5  |
| D2   | GPIO4  |
| D3   | GPIO0  |
| D4   | GPIO2  |
| 3V   | 3.3V   |
| G    | GND    |
| D5   | GPIO14 |
| D6   | GPIO12 |
| D7   | GPIO13 |
| D8   | GPIO15 |
| RX   | RX     |
| TX   | TX     |

**Esquerda (de cima para baixo):**

| Pino | Função     |
| ---- | ---------- |
| AO   | ADC0       |
| G    | GND        |
| VU   | 5V USB     |
| EN   | Enable     |
| RST  | Reset      |
| VIN  | Entrada 5V |

---

## 3. Fiação atual (segundo descrição do usuário)

### 3.1 Display OLED

| Cor Cabo Display | NodeMCU | Função |
| ---------------- | ------- | ------ |
| Verde            | D2      | SDA    |
| Amarelo          | D1      | SCL    |
| Preto            | G       | GND    |
| Vermelho         | 3V      | VCC    |

### 3.2 Sensor DHT11

**Sensor com 4 pinos físicos (da esquerda para a direita, olhando para a face com informações):**

| Pino Sensor | Cor Fio | Função |
| ----------- | ------- | ------ |
| Pino1       | Cinza   | VCC    |
| Pino2       | Vazio   | NC     |
| Pino3       | Azul    | DATA   |
| Pino4       | Roxo    | GND    |

**Conexão via cabo com cores misturadas:**

| Cabo Arduino | Cabo Sensor | Conexão final |
| ------------ | ----------- | ------------- |
| Azul         | Marrom      | DATA          |
| Cinza        | Preto       | GND           |
| Roxo         | Vermelho    | VCC           |

**Observação:** Essa fiação não é padrão e provavelmente causa erro de leitura do DHT11.

---

## 4. Código de teste do DHT11 + OLED

```cpp
#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DHTPIN D6      // pino azul conectado ao NodeMCU
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  dht.begin();

  Wire.begin(D2, D1); // OLED: SDA=D2, SCL=D1
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)){
    Serial.println("Falha ao inicializar OLED!");
    while(true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Iniciando...");
  display.display();
  delay(2000);
}

void loop() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);

  if (isnan(t) || isnan(h)) {
    Serial.println("Erro ao ler DHT11!");
    display.setTextSize(1);
    display.println("Erro ao ler DHT11!");
  } else {
    Serial.printf("Temp: %.1fC Hum: %.1f%%\n", t, h);
    display.print(t,1);
    display.print("C");
    display.setTextSize(1);
    display.setCursor(0,40);
    display.print("Hum: ");
    display.print(h,1);
    display.print("%");
  }

  display.display();
  delay(2000);
}
```

---

## 5. Observações importantes

1. Com a fiação atual, o DHT11 **não está respondendo**, mostrando sempre `NaN` → “Erro ao ler DHT11!” no display.
2. Para funcionar corretamente, seria necessário ligar:

```
DHT11 VCC → 3.3V do NodeMCU
DHT11 GND → GND do NodeMCU
DHT11 DATA → pino digital escolhido (D6 no código atual)
Pull-up 4.7kΩ entre VCC e DATA
```

3. O código atual ajusta apenas o pino DATA (`DHTPIN D6`) sem alterar cabos.
4. Para o display OLED, o código assume SDA=D2 e SCL=D1.
5. Para testar sensor sem mudar fiação, poderia-se criar **sketch que testa múltiplos pinos** para encontrar um que funcione.

