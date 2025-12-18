#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// ==============================================================================
// --- CONFIGURACOES ---
const char* ssid = "SEU_WIFI_SSID";
const char* password = "SUA_SENHA_WIFI";
const char* mp_access_token = "SEU_ACCESS_TOKEN"; 
const float PRECO_POR_ML = 0.05; 

WebServer server(80);

// ==============================================================================
// --- HARDWARE ---
#define JOYSTICK_BUTTON 22
#define JOYSTICK_Y_PIN  26
#define JOYSTICK_X_PIN  27
#define JOY_THRESHOLD_HIGH 850
#define JOY_THRESHOLD_LOW  150

#define PINO_RELE       16
#define BOTAO_A         5
#define I2C_SDA_PIN     14
#define I2C_SCL_PIN     15
#define LED_R_PIN       13
#define LED_G_PIN       11
#define LED_B_PIN       12

// --- DISPLAY ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);
bool display_ok = false;

// --- DADOS DOS FLUIDOS ---
const float MAX_TANK_VOLUME = 750.0;
struct Fluid { const char* name; unsigned long flow_time_50ml; float current_level; };
const int NUM_FLUIDS = 3;
Fluid fluids[NUM_FLUIDS] = {
  { "Agua", 32000, 750.0 }, { "Detergente", 36000, 750.0 }, { "Alcool", 28000, 750.0 }
};
int selected_fluid_index = 0;

// --- NOVOS VOLUMES DEFINIDOS ---
const int VOL_OPTIONS[] = {25, 50, 100, 150};
const int NUM_VOL_OPTIONS = 4;
int vol_idx = 1; // Começa em 50ml (posição 1 do array)

// --- ESTADO GLOBAL ---
bool is_dispensing = false;
unsigned long dispense_start_time = 0;
unsigned long total_dispense_duration = 0;
bool error_state = false;
unsigned long error_timer = 0;
bool payment_enabled = false; 
bool waiting_payment = false;
String current_payment_id = "";
String current_qr_code_data = "";
String current_payer_email = "";
float current_price = 0.0;
unsigned long last_display_update = 0;
unsigned long last_action_time = 0;
unsigned long last_payment_check = 0;

// ==============================================================================
// --- HTML (Interface Atualizada) ---
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head><title>Refil Sustentavel</title><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<script src="https://cdnjs.cloudflare.com/ajax/libs/qrcodejs/1.0.0/qrcode.min.js"></script>
<style>body{font-family:'Segoe UI',sans-serif;background-color:#121212;color:#fff;text-align:center;padding:10px;margin:0}.card{background:#1e1e1e;max-width:400px;margin:15px auto;padding:20px;border-radius:15px;border:1px solid #333}button{width:100%;padding:15px;margin:5px 0;border:none;border-radius:8px;font-size:16px;font-weight:bold;color:#fff;cursor:pointer}.btn-go{background:linear-gradient(45deg,#00c853,#009624)}.btn-stop{background:#d50000}.btn-opt{background:#3700b3;width:48%}.btn-pay{background:#ff9800;color:#000}.bar-bg{background:#444;height:20px;border-radius:10px;margin:10px 0;overflow:hidden}.bar-fill{height:100%;background:#03dac6;width:0%;transition:width .3s}.hidden{display:none}#qr-area{background:#fff;padding:10px;display:inline-block;margin:10px;border-radius:10px}input{padding:12px;width:80%;text-align:center;border-radius:8px;border:1px solid #555;background:#2c2c2c;color:#fff;margin-bottom:10px}</style>
</head><body><div class="card">
<h2>&#9851; Refil Sustent&aacute;vel</h2> <!-- Alterado Titulo -->
<div id="idle-ui"><p style="margin:5px 0">Tanque: <span id="tank-val">0</span> / 750 ml</p><div class="bar-bg"><div id="tank-bar" class="bar-fill"></div></div><div style="display:flex;justify-content:space-between;margin:15px 0"><div style="width:48%"><span style="color:#aaa;font-size:12px">FLUIDO</span><br><strong id="fluid" style="font-size:18px">...</strong></div><div style="width:48%"><span style="color:#aaa;font-size:12px">VOLUME</span><br><strong id="vol" style="font-size:18px">0 ml</strong></div></div>
<!-- Botoes de Controle -->
<button class="btn-opt" style="width:100%; margin-bottom:10px;" onclick="req('fluid_next')">Trocar Fluido</button>
<div style="display:flex;justify-content:space-between">
  <button class="btn-opt" onclick="req('vol_down')">- Vol</button>
  <button class="btn-opt" onclick="req('vol_up')">+ Vol</button>
</div>
<div style="margin:15px 0;border:1px solid #444;padding:10px;border-radius:8px;background:#252525"><label style="font-size:13px;color:#bbb">Modo de Opera&ccedil;&atilde;o (R$ 0.05/ml)</label><br><button id="btn-mode" class="btn-pay" onclick="req('toggle_pay')">DESATIVADO</button></div><div style="margin-top:10px"><input type="email" id="user-email" placeholder="Seu e-mail (Opcional)"></div><button class="btn-go" onclick="startWithEmail()">INICIAR OPERA&Ccedil;&Atilde;O</button><div style="margin-top:20px;border-top:1px solid #444;padding-top:15px"><span style="font-size:12px;color:#777">&Aacute;rea de Manuten&ccedil;&atilde;o</span><br><input type="number" id="refill" placeholder="ml" value="750" style="width:40%;padding:8px"> <button class="btn-opt" style="width:40%;font-size:14px" onclick="refill()">Encher Tanque</button></div></div><div id="pay-ui" class="hidden"><h2 style="color:#00b0ff;margin-bottom:5px">Aguardando Pix...</h2><h3 style="margin:0">Valor: R$ <span id="pay-val">0.00</span></h3><div id="qr-area"><div id="qrcode"></div></div><p style="font-size:11px;color:#888">Abra o app do seu banco e escaneie.</p><button class="btn-stop" onclick="req('cancel_pay')">CANCELAR PEDIDO</button></div><div id="active-ui" class="hidden"><h2 style="color:orange;animation:blink 1s infinite">ENCHENDO...</h2><h1 id="rem" style="font-size:3rem">0.0s</h1><button class="btn-stop" onclick="req('stop')">PARADA DE EMERG&Ecirc;NCIA</button></div><p id="err" style="color:#ff1744;font-weight:bold;min-height:20px"></p><p style="font-size:11px;color:#555">Status: <span id="st">...</span></p></div><script>
let lastQr="";setInterval(()=>{fetch('/status').then(r=>r.json()).then(d=>{document.getElementById('st').innerText="Conectado";document.getElementById('fluid').innerText=d.fl;document.getElementById('vol').innerText=d.req+" ml";document.getElementById('tank-val').innerText=Math.round(d.tnk);let pct=(d.tnk/750)*100;document.getElementById('tank-bar').style.width=pct+"%";document.getElementById('tank-bar').style.background=pct<20?"#ff1744":"#03dac6";document.getElementById('err').innerText=d.err;let btnMode=document.getElementById('btn-mode');if(d.pay_mode){btnMode.innerText="PAGAMENTO ATIVO";btnMode.style.background="#00c853"}else{btnMode.innerText="MODO GRATUITO";btnMode.style.background="#555"}
document.getElementById('idle-ui').classList.add('hidden');document.getElementById('pay-ui').classList.add('hidden');document.getElementById('active-ui').classList.add('hidden');if(d.run){document.getElementById('active-ui').classList.remove('hidden');document.getElementById('rem').innerText=d.rem+"s"}else if(d.wait_pay){document.getElementById('pay-ui').classList.remove('hidden');document.getElementById('pay-val').innerText=d.price;if(d.qr_txt!==""&&d.qr_txt!==lastQr){document.getElementById("qrcode").innerHTML="";new QRCode(document.getElementById("qrcode"),{text:d.qr_txt,width:180,height:180});lastQr=d.qr_txt}}else{document.getElementById('idle-ui').classList.remove('hidden')}}).catch(()=>document.getElementById('st').innerText="Desconectado")},1000);function req(c){fetch('/control?cmd='+c)}
function startWithEmail(){let email=document.getElementById('user-email').value;if(!email||email.trim()==="")email="default";fetch('/control?cmd=start&email='+encodeURIComponent(email))}
function refill(){fetch('/control?cmd=set&val='+document.getElementById('refill').value)}</script><style>@keyframes blink{50%{opacity:.5}}</style></body></html>
)rawliteral";

// ==============================================================================
// --- FUNCOES AUXILIARES ---

void set_rgb_color(int r, int g, int b) { 
  analogWrite(LED_R_PIN, r); 
  analogWrite(LED_G_PIN, g); 
  analogWrite(LED_B_PIN, b); 
}

void update_display() {
  if (!display_ok) return;
  display.clearDisplay(); display.setTextColor(SSD1306_WHITE); display.setTextSize(1);
  if (waiting_payment) {
    display.setCursor(15, 10); display.println("AGUARDANDO PIX"); display.setTextSize(2);
    display.setCursor(15, 30); display.print("R$ "); display.println(current_price);
    display.setTextSize(1); display.setCursor(10, 55); display.println("Pague pelo Celular");
  } else if (is_dispensing) {
    display.setTextSize(2); display.setCursor(10, 10); display.println("ENCHENDO");
    long rem = total_dispense_duration - (millis() - dispense_start_time); if(rem<0) rem=0;
    display.setTextSize(1); display.setCursor(40, 40); display.print(rem/1000.0, 1); display.println(" s");
  } else {
    display.setCursor(0, 0); if(payment_enabled) display.print("Refil Sust. ($)"); else display.print("Refil Sust. (Free)");
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
    display.setCursor(0, 15); display.print("F: "); display.println(fluids[selected_fluid_index].name);
    
    // Mostra o volume baseado no array
    display.setCursor(0, 28); display.print("Vol: "); display.print(VOL_OPTIONS[vol_idx]); display.println(" ml");
    
    display.setCursor(0, 41); display.print("Tnk: "); display.print((int)fluids[selected_fluid_index].current_level); display.println("ml");
    display.setCursor(0, 56); display.print("IP: "); display.println(WiFi.localIP());
  }
  display.display();
}

String create_pix_payment(float value) {
  if (WiFi.status() != WL_CONNECTED) return "";
  WiFiClientSecure client; client.setInsecure();
  HTTPClient https;
  if (https.begin(client, "https://api.mercadopago.com/v1/payments")) {
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", String("Bearer ") + mp_access_token);
    https.addHeader("X-Idempotency-Key", String(millis()));
    JsonDocument doc;
    doc["transaction_amount"] = value;
    doc["description"] = "Refil Sustentavel";
    doc["payment_method_id"] = "pix";
    doc["payer"]["email"] = (current_payer_email.length() > 5 && current_payer_email.indexOf('@') > 0) ? current_payer_email : ("cliente_" + String(millis()) + "@terminal.com");
    String requestBody; serializeJson(doc, requestBody);
    int httpCode = https.POST(requestBody);
    if (httpCode == 201) { 
      String payload = https.getString();
      JsonDocument filter; filter["id"] = true; filter["point_of_interaction"]["transaction_data"]["qr_code"] = true;
      JsonDocument res; deserializeJson(res, payload, DeserializationOption::Filter(filter));
      current_payment_id = res["id"].as<String>();
      String qr = res["point_of_interaction"]["transaction_data"]["qr_code"].as<String>();
      https.end(); return qr;
    }
    https.end();
  }
  return "";
}

bool check_payment_status(String id) {
  if (WiFi.status() != WL_CONNECTED || id == "") return false;
  WiFiClientSecure client; client.setInsecure();
  HTTPClient https;
  String url = "https://api.mercadopago.com/v1/payments/" + id;
  if (https.begin(client, url)) {
    https.addHeader("Authorization", String("Bearer ") + mp_access_token);
    int httpCode = https.GET();
    if (httpCode == 200) {
      String payload = https.getString();
      JsonDocument filter; filter["status"] = true;
      JsonDocument res; deserializeJson(res, payload, DeserializationOption::Filter(filter));
      if (res["status"] == "approved") return true;
    }
    https.end();
  }
  return false;
}

void start_dispensing() {
  is_dispensing = true; waiting_payment = false; current_payer_email = ""; error_state = false;
  
  // CALCULO DE TEMPO BASEADO NO VOLUME SELECIONADO
  // Regra de 3: Se leva flow_time para 50ml, quanto leva para VOL_OPTIONS[vol_idx]?
  float time_per_ml = fluids[selected_fluid_index].flow_time_50ml / 50.0;
  total_dispense_duration = time_per_ml * VOL_OPTIONS[vol_idx];
  
  dispense_start_time = millis();
  digitalWrite(PINO_RELE, LOW); set_rgb_color(0, 0, 255); 
}

void start_process_logic() {
  if (is_dispensing || waiting_payment) return;
  
  // Pega o volume direto do array
  float req_vol = VOL_OPTIONS[vol_idx];
  
  if (req_vol > fluids[selected_fluid_index].current_level) {
    error_state = true; error_timer = millis();
    set_rgb_color(255, 0, 0); update_display(); return;
  }

  if (payment_enabled) {
    current_price = req_vol * PRECO_POR_ML;
    if (current_price < 0.01) current_price = 0.01;
    waiting_payment = true; current_qr_code_data = ""; 
    set_rgb_color(255, 255, 0); update_display();
    String qr = create_pix_payment(current_price);
    if (qr != "") current_qr_code_data = qr;
    else { waiting_payment = false; error_state = true; error_timer = millis(); }
  } else { start_dispensing(); }
}

void stop_dispensing() {
  if (!is_dispensing) return;
  unsigned long elapsed = millis() - dispense_start_time;
  if (elapsed > total_dispense_duration) elapsed = total_dispense_duration;
  float time_per_ml = (float)fluids[selected_fluid_index].flow_time_50ml / 50.0;
  fluids[selected_fluid_index].current_level -= (elapsed / time_per_ml);
  if(fluids[selected_fluid_index].current_level < 0) fluids[selected_fluid_index].current_level = 0;
  digitalWrite(PINO_RELE, HIGH); is_dispensing = false; 
  vol_idx = 1; // Reseta para 50ml (posicao 1)
  set_rgb_color(0, 255, 0);
}

// --- HANDLERS ---

void handleRoot() { server.send(200, "text/html; charset=utf-8", index_html); }

void handleStatus() {
  if (waiting_payment && millis() - last_payment_check > 3000) {
    last_payment_check = millis();
    if (check_payment_status(current_payment_id)) {
      waiting_payment = false;
      start_dispensing();
    }
  }

  JsonDocument doc;
  doc["run"] = is_dispensing;
  doc["wait_pay"] = waiting_payment;
  doc["pay_mode"] = payment_enabled;
  doc["fl"] = fluids[selected_fluid_index].name;
  
  // Envia volume selecionado
  doc["req"] = VOL_OPTIONS[vol_idx];
  
  doc["tnk"] = fluids[selected_fluid_index].current_level;
  doc["price"] = String(current_price, 2);
  doc["qr_txt"] = waiting_payment ? current_qr_code_data : "";
  
  long rem = 0;
  if(is_dispensing) {
    long elapsed = millis() - dispense_start_time;
    rem = total_dispense_duration - elapsed;
    if(rem < 0) rem = 0;
  }
  doc["rem"] = String(rem / 1000.0, 1);
  doc["err"] = error_state ? "ERRO: Tanque/Conexao" : "";

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleControl() {
  if (!server.hasArg("cmd")) { server.send(400, "text/plain", "Erro"); return; }
  String cmd = server.arg("cmd");
  
  if (cmd == "start") {
    current_payer_email = server.hasArg("email") && server.arg("email") != "default" ? server.arg("email") : "";
    start_process_logic();
  }
  else if (cmd == "stop") stop_dispensing();
  else if (cmd == "cancel_pay") { waiting_payment = false; current_payer_email = ""; set_rgb_color(0,255,0); }
  else if (cmd == "toggle_pay") { payment_enabled = !payment_enabled; }
  
  // LOGICA DE VOLUME ATUALIZADA (UP e DOWN)
  else if (cmd == "vol_up" && !is_dispensing && !waiting_payment) {
    vol_idx++; 
    if (vol_idx >= NUM_VOL_OPTIONS) vol_idx = 0;
  }
  else if (cmd == "vol_down" && !is_dispensing && !waiting_payment) {
    vol_idx--; 
    if (vol_idx < 0) vol_idx = NUM_VOL_OPTIONS - 1;
  }
  
  else if (cmd == "fluid_next" && !is_dispensing && !waiting_payment) {
    selected_fluid_index++; if (selected_fluid_index >= NUM_FLUIDS) selected_fluid_index = 0;
  }
  else if (cmd == "set") {
    float val = server.arg("val").toFloat();
    if(val >= 0 && val <= 750) fluids[selected_fluid_index].current_level = val;
    error_state = false;
  }
  server.send(200, "text/plain", "OK");
  update_display();
}

void handle_hardware_logic() {
  if (error_state && millis() - error_timer > 3000) { error_state = false; set_rgb_color(0, 255, 0); update_display(); }
  if (!is_dispensing && !waiting_payment) {
    if (digitalRead(BOTAO_A) == LOW) { 
        if(millis() - last_action_time > 500) { current_payer_email = ""; start_process_logic(); last_action_time = millis(); }
        return; 
    }
    if (millis() - last_action_time < 250) return;
    int y = analogRead(JOYSTICK_Y_PIN); int x = analogRead(JOYSTICK_X_PIN); bool changed = false;
    
    // JOYSTICK Y PARA CONTROLAR INDEX DO ARRAY
    if (y > JOY_THRESHOLD_HIGH) { 
        vol_idx++; 
        if(vol_idx >= NUM_VOL_OPTIONS) vol_idx = 0; 
        changed=true; 
    }
    else if (y < JOY_THRESHOLD_LOW) { 
        vol_idx--; 
        if(vol_idx < 0) vol_idx = NUM_VOL_OPTIONS - 1; 
        changed=true; 
    }
    
    if (x > JOY_THRESHOLD_HIGH) { selected_fluid_index++; if(selected_fluid_index >= NUM_FLUIDS) selected_fluid_index=0; changed=true; }
    else if (x < JOY_THRESHOLD_LOW) { selected_fluid_index--; if(selected_fluid_index<0) selected_fluid_index=NUM_FLUIDS-1; changed=true; }
    if (changed) { last_action_time = millis(); set_rgb_color(255, 165, 0); update_display(); }
  } else {
    if (digitalRead(JOYSTICK_BUTTON) == LOW) {
        if (waiting_payment) { waiting_payment = false; set_rgb_color(0,255,0); update_display(); delay(500); }
        else if (is_dispensing) { stop_dispensing(); }
    }
    if (is_dispensing && (millis() - dispense_start_time >= total_dispense_duration)) { stop_dispensing(); }
  }
}

// ==============================================================================
// --- SETUP E LOOP ---

void setup() {
  Serial.begin(115200);
  delay(3000); 

  pinMode(BOTAO_A, INPUT_PULLUP);
  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);
  pinMode(JOYSTICK_Y_PIN, INPUT);
  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  pinMode(PINO_RELE, OUTPUT);
  digitalWrite(PINO_RELE, HIGH);

  Wire1.setSDA(I2C_SDA_PIN);
  Wire1.setSCL(I2C_SCL_PIN);
  Wire1.begin(); 
  Wire1.setClock(400000);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display_ok = true;
    display.clearDisplay();
    display.display();
  }

  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(WiFi.localIP());
    server.on("/", handleRoot);
    server.on("/status", handleStatus);
    server.on("/control", handleControl);
    server.begin();
  }

  set_rgb_color(0, 255, 0);
  update_display();
}

void loop() {
  server.handleClient();
  handle_hardware_logic();
  if (millis() - last_display_update > 200) { 
    update_display(); 
    last_display_update = millis(); 
  }
}
