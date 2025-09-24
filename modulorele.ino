
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h> // Nome da biblioteca corrigido
#include <string>
#define JOYSTICK_BUTTON 22

#define pino_rele       16
#define botao_A         5
#define botao_B         6


#define I2C_SDA_PIN     14
#define I2C_SCL_PIN     15


#define LED_R_PIN       13
#define LED_G_PIN       11
#define LED_B_PIN       12


#include "pico.h"
#include "arduino.h"

arduino::MbedI2C myI2C(I2C_SDA_PIN, I2C_SCL_PIN);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &myI2C, -1);
bool display_ok = false;



const int tempo_vazao_50ml = 32000; 
int volume_multiplier = 1;        
bool is_dispensing = false;      


unsigned long dispense_start_time;
unsigned long total_dispense_duration;

void iniciar_rele() {
  pinMode(pino_rele, OUTPUT);
  digitalWrite(pino_rele, LOW); 
  Serial.println("Sistema do Rele INICIADO. Rele LIGADO.");
}

// Desativa o relé
void parar_rele() {
  pinMode(pino_rele, INPUT); // Coloca o pino em alta impedância para desligar com segurança
  Serial.println("Sistema do Rele PARADO.");
}

// Define a cor do LED RGB
void set_rgb_color(int r, int g, int b) {
  analogWrite(LED_R_PIN, r);
  analogWrite(LED_G_PIN, g);
  analogWrite(LED_B_PIN, b);
}

// Função para atualizar o display OLED
void update_display(int current_volume_multiplier, bool dispensing, long remaining_ms) {
  if (!display_ok) return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (!dispensing) {
    // --- TELA DE ESPERA ---
    display.setCursor(0, 0);
    display.println("Dispensador BitDogLab");
    display.drawFastHLine(0, 10, display.width(), SSD1306_WHITE);

    display.setCursor(0, 20);
    display.print("Volume: ");
    display.print(current_volume_multiplier * 50);
    display.println(" ml");
    
    display.setCursor(0, 40);
    display.println("A: Iniciar");
    display.setCursor(0, 50);
    display.println("B: +Volume");
  } else {
    // --- TELA DE AÇÃO (DISPENSANDO) ---
    float remaining_seconds = remaining_ms / 1000.0; // Converte para segundos com decimal
    
    display.setTextSize(1);
    display.setCursor(5, 10);
    display.println("Dispensando");
    
    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("Tempo Restante:");
    display.setCursor(25, 50);
    display.print(remaining_seconds, 1); // Exibe com 1 casa decimal
    display.print(" s");
  }

  display.display();
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando o Dispensador...");
  
  // A configuração dos pinos I2C agora é feita na declaração global.
  // Não é mais necessário usar Wire.setSDA() ou Wire.setSCL() aqui.

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // 0x3C é o endereço I2C comum para OLEDs
    display_ok = true;
    Serial.println("Display OLED inicializado com sucesso.");
  } else {
    Serial.println("Erro ao inicializar o display.");
  }
  
  pinMode(botao_A, INPUT_PULLUP);
  pinMode(botao_B, INPUT_PULLUP);
  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);
  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  
  parar_rele(); // Garante que o relé comece desligado
  
  set_rgb_color(0, 255, 0); // Verde: sistema pronto
  update_display(volume_multiplier, false, 0);
}

// --- Loop Principal (NÃO-BLOQUEANTE) ---
void loop() {

  if (!is_dispensing) {
    // --- ESTADO DE ESPERA (IDLE) ---
    set_rgb_color(0, 255, 0); // LED Verde

    // Botão A: Inicia a dispensação
    if (digitalRead(botao_A) == LOW) {
      if (volume_multiplier > 0) {
        is_dispensing = true; // Muda para o estado de "dispensando"
        total_dispense_duration = (unsigned long)tempo_vazao_50ml * volume_multiplier;
        dispense_start_time = millis();
        
        iniciar_rele();
        set_rgb_color(0, 0, 255); // LED Azul enquanto dispensa
        update_display(volume_multiplier, true, total_dispense_duration); 
      }
      delay(200); // Debounce do botão para evitar múltiplos cliques
    }

    // Botão B: Aumenta o volume
    if (digitalRead(botao_B) == LOW) {
      volume_multiplier++;
      // Limite de volume (ex: máximo de 10 * 50ml = 500ml)
      if (volume_multiplier > 10) {
        volume_multiplier = 1; // Volta para o início se passar do limite
      }
      set_rgb_color(255, 165, 0); // Laranja para indicar mudança de configuração
      update_display(volume_multiplier, false, 0);
      delay(200); // Debounce
    }

  } else {
    // --- ESTADO DE AÇÃO (DISPENSANDO) ---
    unsigned long elapsed_time = millis() - dispense_start_time;
    long remaining_time = total_dispense_duration - elapsed_time;

    // Se o Botão A for pressionado durante a dispensação, funciona como PARADA DE EMERGÊNCIA
    if(digitalRead(JOYSTICK_BUTTON) == LOW){
      remaining_time = 0;
    }
 
    if (remaining_time <= 0) {
      parar_rele();
      is_dispensing = false;
      volume_multiplier = 1; 
      set_rgb_color(0, 255, 0); 
      update_display(volume_multiplier, false, 0);
    } else {
      // Tempo ainda não acabou: atualizar o display com o tempo restante
      update_display(volume_multiplier, true, remaining_time);
      delay(100); // Pequeno delay para a tela não piscar excessivamente
    }
  }
}
