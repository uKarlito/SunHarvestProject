/*
 * =====================================================
 *  SUNHARVEST AGRO — IoT Node (FIAP ADS / GS 2026)
 *  Equipe 07 — Challenge 2025
 * =====================================================
 *
 *  ENTRADAS:
 *    GPIO 34 -> Pot 1: umidade do solo (0-100%)
 *    GPIO 35 -> Pot 2: corrente da bomba solar (INA219 no fisico)
 *
 *  SAIDAS:
 *    GPIO 26 -> LED Verde:    sistema OK
 *    GPIO 27 -> LED Vermelho: alerta (solo seco ou bomba com falha)
 *
 *  INTERFACE LOCAL:
 *    LCD 16x2 I2C - alterna umidade/bomba a cada 3s
 *
 *  ENDPOINTS REST (porta 80):
 *    GET  /          -> identidade do no IoT + config da fazenda
 *    GET  /leitura   -> dados atuais do campo para o backend
 *    GET  /historico -> ultimas 10 leituras (serie temporal)
 *    POST /comando   -> recebe {"acao":"ligar_bomba"} do backend
 */

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

const char* SSID     = "Wokwi-GUEST";
const char* PASSWORD = "";

const int PIN_UMIDADE  = 34;
const int PIN_CORRENTE = 35;
const int PIN_LED_V    = 26;
const int PIN_LED_R    = 27;

LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

// Config da fazenda (espelha o modelo Farm do backend SunHarvest)
const String FARM_ID        = "farm-equipe07-gs2026";
const String CROP_TYPE      = "Soja";
const String SOIL_TYPE      = "Franco";
const float  AREA_HA        = 5.0;
const float  PANEL_CAP_W    = 3000.0;
const float  PUMP_POWER_W   = 1100.0;
const float  IRR_EFFICIENCY = 0.85;
const float  TENSAO_PAINEL  = 24.0;

// Limiares agronomicos
const float UMIDADE_CRITICA = 30.0;
const float UMIDADE_IDEAL   = 60.0;
const float CORRENTE_MIN    = 2.0;
const float CORRENTE_MAX    = 8.5;

// Estado
float umidade = 0, corrente = 0, potBomba = 0, eficBomba = 0;
bool soloSeco = false, bombaFalha = false, bombaLigada = false;

// Historico
struct Leitura { float um, cor, pot, ef; bool seco, falha; unsigned long ts; };
Leitura hist[10];
int idxH = 0, totH = 0;

byte charGota[8] = {0b00100,0b00100,0b01110,0b11111,0b11111,0b11111,0b01110,0b00000};

void handleRoot() {
  StaticJsonDocument<300> d;
  d["device"] = "SunHarvest-IoT-Node"; d["farm_id"] = FARM_ID;
  d["crop_type"] = CROP_TYPE; d["soil_type"] = SOIL_TYPE;
  d["area_ha"] = AREA_HA; d["panel_w"] = PANEL_CAP_W;
  d["pump_w"] = PUMP_POWER_W; d["irr_efficiency"] = IRR_EFFICIENCY;
  d["status"] = "online"; d["uptime_s"] = millis()/1000;
  d["bomba_ligada"] = bombaLigada;
  String j; serializeJsonPretty(d,j); server.send(200,"application/json",j);
}

void handleLeitura() {
  StaticJsonDocument<400> d;
  d["umidade_solo_pct"] = umidade; d["solo_seco"] = soloSeco;
  d["corrente_bomba_a"] = corrente; d["tensao_painel_v"] = TENSAO_PAINEL;
  d["potencia_bomba_w"] = potBomba; d["eficiencia_bomba_pct"] = eficBomba;
  d["bomba_falha"] = bombaFalha; d["bomba_ligada"] = bombaLigada;
  String rec;
  if (soloSeco && !bombaFalha) rec = "IRRIGAR - solo abaixo do limite critico";
  else if (bombaFalha)         rec = "ALERTA - verificar bomba ou painel solar";
  else if (umidade > UMIDADE_IDEAL) rec = "OK - solo com umidade adequada";
  else                         rec = "MONITORANDO - aguardando calculo ETo";
  d["recomendacao_local"] = rec;
  d["timestamp_ms"] = millis();
  String j; serializeJsonPretty(d,j); server.send(200,"application/json",j);
}

void handleHistorico() {
  StaticJsonDocument<1200> d;
  d["farm_id"] = FARM_ID; d["total"] = totH;
  JsonArray arr = d.createNestedArray("leituras");
  int c = min(totH, 10);
  for (int i=0;i<c;i++) {
    int idx=(idxH-c+i+10)%10;
    JsonObject o=arr.createNestedObject();
    o["umidade_solo_pct"]=hist[idx].um; o["corrente_bomba_a"]=hist[idx].cor;
    o["potencia_bomba_w"]=hist[idx].pot; o["eficiencia_bomba_pct"]=hist[idx].ef;
    o["solo_seco"]=hist[idx].seco; o["bomba_falha"]=hist[idx].falha;
    o["timestamp_ms"]=hist[idx].ts;
  }
  String j; serializeJsonPretty(d,j); server.send(200,"application/json",j);
}

void handleComando() {
  if (!server.hasArg("plain")) { server.send(400,"application/json","{\"erro\":\"body ausente\"}"); return; }
  StaticJsonDocument<128> req;
  deserializeJson(req, server.arg("plain"));
  String acao = req["acao"] | "";
  if (acao=="ligar_bomba")    { bombaLigada=true;  server.send(200,"application/json","{\"status\":\"bomba ligada\"}"); }
  else if (acao=="desligar_bomba") { bombaLigada=false; server.send(200,"application/json","{\"status\":\"bomba desligada\"}"); }
  else server.send(400,"application/json","{\"erro\":\"acao desconhecida\"}");
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_V, OUTPUT); pinMode(PIN_LED_R, OUTPUT);
  lcd.init(); lcd.backlight(); lcd.createChar(0, charGota);
  lcd.setCursor(0,0); lcd.print("SunHarvest Agro");
  lcd.setCursor(0,1); lcd.print("Iniciando...");

  WiFi.begin(SSID, PASSWORD);
  int t=0;
  while (WiFi.status()!=WL_CONNECTED && t<20) { delay(500); Serial.print("."); t++; }
  lcd.clear();
  if (WiFi.status()==WL_CONNECTED) {
    lcd.setCursor(0,0); lcd.print("Wi-Fi OK!");
    lcd.setCursor(0,1); lcd.print(WiFi.localIP());
    Serial.println("\nIP: "+WiFi.localIP().toString());
  } else {
    lcd.setCursor(0,0); lcd.print("Sem Wi-Fi");
    lcd.setCursor(0,1); lcd.print("Modo local");
  }
  delay(2000);

  server.on("/",         HTTP_GET,  handleRoot);
  server.on("/leitura",  HTTP_GET,  handleLeitura);
  server.on("/historico",HTTP_GET,  handleHistorico);
  server.on("/comando",  HTTP_POST, handleComando);
  server.begin();

  Serial.println("─────────────────────────────────");
  Serial.println("SUNHARVEST IoT NODE — GS 2026");
  Serial.println("  GET  /           -> identidade");
  Serial.println("  GET  /leitura    -> dados do campo");
  Serial.println("  GET  /historico  -> serie temporal");
  Serial.println("  POST /comando    -> controla bomba");
  Serial.println("─────────────────────────────────");
}

unsigned long ultLeit = 0; int pagLCD = 0;

void loop() {
  server.handleClient();
  if (millis()-ultLeit >= 3000) {
    ultLeit = millis();

    // ENTRADA 1: umidade solo
    umidade = (analogRead(PIN_UMIDADE)/4095.0)*100.0;
    soloSeco = (umidade < UMIDADE_CRITICA);

    // ENTRADA 2: corrente bomba (INA219 no fisico)
    corrente = (analogRead(PIN_CORRENTE)/4095.0)*10.0;
    potBomba = TENSAO_PAINEL * corrente;

    if (bombaLigada && PUMP_POWER_W > 0) {
      eficBomba = min((potBomba/PUMP_POWER_W)*100.0, 100.0);
      bombaFalha = (corrente<CORRENTE_MIN) || (corrente>CORRENTE_MAX) || (eficBomba<40.0);
    } else {
      eficBomba = 0;
      bombaFalha = (corrente>1.0 && !bombaLigada);
    }

    // SAIDAS: LEDs
    bool alerta = soloSeco || bombaFalha;
    digitalWrite(PIN_LED_V, !alerta);
    digitalWrite(PIN_LED_R, alerta);

    // Historico
    hist[idxH]={umidade,corrente,potBomba,eficBomba,soloSeco,bombaFalha,millis()};
    idxH=(idxH+1)%10; if(totH<10) totH++;

    // LCD
    lcd.clear();
    if (pagLCD==0) {
      lcd.setCursor(0,0); lcd.write(0); lcd.printf(" Solo:%.0f%%  ", umidade);
      lcd.setCursor(0,1);
      if (soloSeco)             lcd.print("SECO! IRRIGAR   ");
      else if (umidade>UMIDADE_IDEAL) lcd.print("Umidade OK      ");
      else                      lcd.print("Monitorando...  ");
    } else {
      lcd.setCursor(0,0); lcd.printf("Bomba:%.1fA     ", corrente);
      lcd.setCursor(0,1);
      if (bombaFalha)     lcd.print("FALHA BOMBA!    ");
      else if (bombaLigada) lcd.printf("Op.%.0f%%         ", eficBomba);
      else                lcd.print("Bomba deslig.   ");
    }
    pagLCD=(pagLCD+1)%2;

    // Serial
    Serial.printf("Umidade: %.1f%% %s | Corrente: %.2fA | Bomba: %s %s\n",
      umidade, soloSeco?"[SECO]":"[OK]",
      corrente, bombaLigada?"LIGADA":"deslig.",
      bombaFalha?"[FALHA]":"");
  }
}
