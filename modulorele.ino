#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string>
#define JOYSTICK_BUTTON 22
#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define pino_rele  16
#define botao_A  5
#define botao_B  6
// Pinos do Display OLED (I2C)
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15
#define LED_R_PIN 13
#define LED_G_PIN 11
#define LED_B_PIN 12
arduino::MbedI2C myI2C(I2C_SDA_PIN, I2C_SCL_PIN);

// Configurações do Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &myI2C, -1);
bool display_ok = false;

const int tempo_vazao_50ml = 32000;
int volume = 0;
// Função para configurar o pino do relé como SAÍDA e desligá-lo.
// (Relés "ativos em nível baixo" desligam com HIGH)
void iniciar_rele() {
  pinMode(pino_rele, OUTPUT);
  digitalWrite(pino_rele, HIGH); // Garante que o relé comece desligado
  Serial.println("Sistema do Rele INICIADO. Rele DESLIGADO.");
}
// Função para "desconfigurar" o pino do relé, colocando-o em modo de entrada.
// Isso faz com que o pino pare de enviar sinal, desligando o relé.
void parar_rele() {
  pinMode(pino_rele, INPUT);
  Serial.println("Sistema do Rele PARADO.");
}

void set_rgb_color(int r, int g, int b) {
  analogWrite(LED_R_PIN, r);
  analogWrite(LED_G_PIN, g);
  analogWrite(LED_B_PIN, b);
}

void update_display(int volume_ml, bool is_dispensing) {
  // Se o display não iniciou, não faz nada.
  if (!display_ok) return;

  // Limpa o conteúdo anterior
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // --- LÓGICA DE EXIBIÇÃO ---
  // A tela muda completamente dependendo do estado do sistema.

  if (!is_dispensing) {
    // --- TELA DE ESPERA (IDLE) ---
    display.setCursor(0, 0);
    display.println("Dispensador BitDogLab");
    display.drawFastHLine(0, 10, display.width(), SSD1306_WHITE); // Linha separadora

    display.setCursor(0, 20);
    display.print("Volume: ");
    display.print(volume_ml*50); // A função print() aceita inteiros diretamente
    display.println(" ml");
    display.setCursor(0, 30);
    display.println("A: Iniciar");
    display.setCursor(0, 40);
    display.println("B: +volume");
  } else {
    // --- TELA DE AÇÃO (DISPENSANDO) ---
    display.setTextSize(2); // Usa texto maior para o status principal
    display.setCursor(5, 10);
    display.println("Dispensando");    
    display.setCursor(35, 35);
    display.print(volume_ml*50);
    display.println(" ml...");
    display.setCursor(0, 55);
    display.drawFastHLine(0, 52, display.width(), SSD1306_WHITE);    
  }

  // Envia o conteúdo do buffer para a tela
  display.display();
}

void setup() {
Serial.begin(115200);
  Serial.println("Inicializando o Dispensador com BitDogLab...");

  // Inicializa o Display OLED
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display_ok = true;
    Serial.println("Display OLED inicializado com sucesso.");
    display.clearDisplay();
    display.setCursor(5, 30);
    display.print("Dispensador Pronto!");
    display.display();
    delay(1500);
  } else {
    Serial.println("Erro ao inicializar o display.");
  }

  // Configura os botões como ENTRADA com pull-up interno
  pinMode(botao_A, INPUT_PULLUP);
  pinMode(botao_B, INPUT_PULLUP);

  // Configura os pinos dos LEDs e do Relé como SAÍDA
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  pinMode(pino_rele, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
 // Sinaliza que o sistema está pronto com a cor verde
  set_rgb_color(0, 255, 0);

  // Começa com o sistema do relé parado para economizar energia.
  parar_rele();
}

void loop() {
  // Verifica o estado do botão A
  // digitalRead(botao_A) == LOW significa que o botão foi pressionado
 set_rgb_color(0, 255, 0);
  bool is_dispensing = false;
  update_display(volume,is_dispensing);
  if (digitalRead(botao_A) == LOW) {
    iniciar_rele();
    set_rgb_color(0, 0, 255);
    is_dispensing = true;
    update_display(volume,is_dispensing);
    // Deley para preenchimento medio de 100ml a uma vazão de 4L por min 
    delay(tempo_vazao_50ml*volume);
    parar_rele();
    set_rgb_color(0, 255, 0);
    is_dispensing=false;    
    volume = 0;
    update_display(volume,is_dispensing);

  }
  // Verifica o estado do botão B
  if (digitalRead(botao_B) == LOW) {
    volume++;
    set_rgb_color(255, 0, 0);
    delay(200);
  }
}
