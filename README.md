# 🌱 SunHarvest Agro — Irrigação Solar Inteligente

> *"O sol irriga sua lavoura — nós garantimos que nenhuma gota seja desperdiçada e nenhuma bomba pare sem você saber."*

**FIAP ADS | Disruptive Architectures: IoT, IoB & Generative IA | Global Solution 2026 | Equipe 07**

---

## O Problema

O Brasil é o maior irrigante da América Latina. Mais de 8,2 milhões de hectares são irrigados, consumindo ~70% da água doce retirada no país. O problema é duplo:

1. **Irrigação por achismo** — sem dados locais de evapotranspiração, o produtor irriga por calendário fixo, desperdiçando até 30% a mais que o necessário.
2. **Bomba solar invisível** — quando o painel suja ou a bomba degrada, o produtor só descobre quando a lavoura já está murchando.

## A Solução

O SunHarvest combina **dados de satélite da NASA** (radiação solar, temperatura, vento) com **sensores IoT no campo** (umidade do solo, corrente da bomba) para calcular exatamente quanto irrigar — e avisar quando algo está errado.

```
ESP32 no campo           Backend Java (Spring Boot)      App React Native
──────────────           ──────────────────────────      ────────────────
Umidade solo      →      ETo Penman-Monteith         →   "Irrigue 18mm hoje"
Corrente bomba    →      + NASA POWER API             →   "Bomba a 92%"
                  ←      POST /comando: ligar_bomba   ←   "Acionar irrigação"
```

---

## Componente IoT (ESP32)

### Hardware

| Componente | Pino | Tipo | Função |
|---|---|---|---|
| Potenciômetro 1 | GPIO 34 | **ENTRADA** | Umidade do solo (0–100%) |
| Potenciômetro 2 | GPIO 35 | **ENTRADA** | Corrente da bomba (0–10A) |
| LED Verde | GPIO 26 | **SAÍDA** | Sistema OK |
| LED Vermelho | GPIO 27 | **SAÍDA** | Alerta (solo seco / bomba com falha) |
| LCD 16x2 I2C | SDA=21, SCL=22 | **Interface local** | Dados em tempo real |

### Endpoints REST

| Método | Endpoint | Descrição |
|---|---|---|
| GET | `/` | Identidade do nó IoT e configurações da fazenda |
| GET | `/leitura` | Dados atuais: umidade, corrente, potência, eficiência |
| GET | `/historico` | Últimas 10 leituras (série temporal) |
| POST | `/comando` | Aciona/desliga a bomba (`{"acao":"ligar_bomba"}`) |

---

## Como Rodar

### Simulação no Wokwi

1. Acesse [wokwi.com](https://wokwi.com) → New Project → ESP32
2. Substitua `sketch.ino` e `diagram.json` pelos arquivos da pasta `/iot`
3. Adicione as bibliotecas: `LiquidCrystal I2C` e `ArduinoJson`
4. Clique em **Play ▶**
5. O IP do ESP32 aparece no LCD e no Serial Monitor

---

*Dados climáticos: [NASA POWER Agroclimatology](https://power.larc.nasa.gov)*  
*Método de irrigação: FAO Penman-Monteith (Paper 56)*
