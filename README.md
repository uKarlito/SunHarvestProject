# 🌱 SunHarvest Agro — Irrigação Solar Inteligente

> *"O sol irriga sua lavoura — nós garantimos que nenhuma gota seja desperdiçada e nenhuma bomba pare sem você saber."*

**FIAP ADS | Disruptive Architectures: IoT, IoB & Generative IA | Global Solution - SPACE CONNECT 

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

## Estrutura do Repositório

```
sunharvest/
├── iot/
│   ├── sketch.ino          # Código ESP32 (Wi-Fi + WebServer + sensores)
│   ├── diagram.json        # Circuito Wokwi (montar automaticamente)
│   └── libraries.txt       # Dependências Arduino
├── docs/
│   └── documentacao.docx  # Documentação técnica completa
├── servidor_esp32.js       # Servidor mock para testar os endpoints REST
└── README.md
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

## Como Rodar a Simulação (Wokwi)

1. Acesse [wokwi.com/projects/465755302340271105](https://wokwi.com/projects/465755302340271105)
2. Clique em **Play ▶** para iniciar
3. Gire o **Pot 1** para simular variação de umidade do solo
4. Gire o **Pot 2** para simular variação de corrente da bomba
5. O LCD alterna entre página de umidade e página de bomba a cada 3s
6. LED verde = sistema OK | LED vermelho = alerta

> ⚠️ O WebServer do ESP32 roda internamente no Wokwi e não é acessível pelo navegador externo no plano gratuito. Para testar os endpoints REST, use o servidor mock abaixo.

---

## Como Testar os Endpoints REST

O arquivo `servidor_esp32.js` simula exatamente as respostas do ESP32, permitindo testar os endpoints no navegador.

### Pré-requisito

Node.js instalado — baixe em [nodejs.org](https://nodejs.org) se necessário.

Verifique com:
```bash
node --version
```

### Passo 1 — Rodar o servidor

No terminal, navegue até a pasta onde está o arquivo e execute:

```bash
node servidor_esp32.js
```

Deve aparecer:
```
SunHarvest - Mock ESP32 Rodando!
Servidor: http://localhost:3000

Testa no navegador:
  http://localhost:3000/
  http://localhost:3000/leitura
  http://localhost:3000/historico

Aguardando requisicoes... (Ctrl+C para parar)
```

### Passo 2 — Testar os endpoints no navegador

Com o servidor rodando, abra o navegador e acesse cada URL:

#### `GET /` — Identidade da fazenda
```
http://localhost:3000/
```
Resposta esperada:
```json
{
  "device": "SunHarvest-IoT-Node",
  "farm_id": "farm-equipe07-gs2026",
  "crop_type": "Soja",
  "soil_type": "Franco",
  "area_ha": 5,
  "panel_w": 3000,
  "pump_w": 1100,
  "status": "online",
  "uptime_s": 0,
  "bomba_ligada": false
}
```

#### `GET /leitura` — Dados atuais do campo
```
http://localhost:3000/leitura
```
Resposta esperada:
```json
{
  "umidade_solo_pct": 24.3,
  "solo_seco": true,
  "corrente_bomba_a": 0.15,
  "tensao_painel_v": 24,
  "potencia_bomba_w": 3.6,
  "eficiencia_bomba_pct": 0,
  "bomba_falha": false,
  "bomba_ligada": false,
  "recomendacao_local": "IRRIGAR - solo abaixo do limite critico",
  "timestamp_ms": 3000
}
```

> Acesse várias vezes para ver os dados variando — a umidade oscila simulando o comportamento real do solo ao longo do dia.

#### `GET /historico` — Últimas 10 leituras
```
http://localhost:3000/historico
```
Resposta esperada:
```json
{
  "farm_id": "farm-equipe07-gs2026",
  "total_leituras": 5,
  "leituras": [
    {
      "umidade_solo_pct": 22.1,
      "corrente_bomba_a": 0.13,
      ...
    }
  ]
}
```

#### `POST /comando` — Controlar a bomba

Para testar o POST, use o terminal com o comando `curl`:

```bash
# Ligar a bomba
curl -X POST http://localhost:3000/comando -H "Content-Type: application/json" -d "{\"acao\":\"ligar_bomba\"}"

# Desligar a bomba
curl -X POST http://localhost:3000/comando -H "Content-Type: application/json" -d "{\"acao\":\"desligar_bomba\"}"
```

Resposta esperada:
```json
{ "status": "bomba ligada" }
```

Após ligar a bomba, acesse `/leitura` novamente — o campo `bomba_ligada` vai aparecer como `true` e a corrente vai subir para o range operacional (2–5A).

### Passo 3 — Ver os logs no terminal

Cada requisição ao `/leitura` aparece no terminal:
```
[/leitura] Umidade: 24.3% | Corrente: 0.15A | IRRIGAR - solo abaixo do limite critico
[/leitura] Umidade: 35.7% | Corrente: 0.18A | MONITORANDO - aguardando calculo ETo
[/comando] Bomba LIGADA
[/leitura] Umidade: 48.2% | Corrente: 3.82A | MONITORANDO - aguardando calculo ETo
```

### Parar o servidor

```
Ctrl + C
```

---

## Checklist de Requisitos FIAP

- ✅ Protótipo ESP32 funcional (Wokwi)
- ✅ 2 Entradas (Pot1 umidade GPIO 34 + Pot2 corrente GPIO 35)
- ✅ 2 Saídas (LED verde GPIO 26 + LED vermelho GPIO 27)
- ✅ Interface local (LCD 16x2 I2C)
- ✅ Comunicação Wi-Fi (Wokwi-GUEST)
- ✅ WebServer + API REST (porta 80)
- ✅ 3+ endpoints JSON documentados (GET /, /leitura, /historico + POST /comando)
- ✅ Documentação técnica (docs/documentacao.docx)

---

## Integrantes

| Camilo Micheletto Ribeiro da Silva
| Carlos André Silva
| Guilherme Ribeiro da Costa
| Laura Lopes Cruz

---

*Dados climáticos: [NASA POWER Agroclimatology](https://power.larc.nasa.gov)*  
*Método de irrigação: FAO Penman-Monteith (Paper 56)*  
*Simulação: [Wokwi ESP32 Simulator](https://wokwi.com)*
