#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string>

// --- PINOS DO JOYSTICK ---
#define JOYSTICK_BUTTON 22
#define JOYSTICK_Y_PIN  26 // Eixo Y para Volume (GP26 = ADC0)
#define JOYSTICK_X_PIN  27 // Eixo X para Fluido (GP27 = ADC1)

// --- VALORES DE LIMITE (THRESHOLD) PARA O JOYSTICK ---
// O ADC do Pico é 10-bit (0-1023), com o centro em ~512.
#define JOY_THRESHOLD_HIGH 800  // Valor para registrar movimento para cima/direita
#define JOY_THRESHOLD_LOW  200  // Valor para registrar movimento para baixo/esquerda

// --- PINOS GERAIS ---
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


// --- ESTRUTURA PARA ARMAZENAR DADOS DOS FLUIDOS ---
struct Fluid {
  const char* name;       // Nome do fluido
  int flow_time_50ml;   // Tempo em milissegundos para dispensar 50ml
};

// --- ARRAY DE FLUIDOS DISPONÍVEIS ---
// Adicione novos fluidos aqui facilmente!
const int NUM_FLUIDS = 3;
Fluid fluids[NUM_FLUIDS] = {
  { "Agua",       32000 },
  { "Detergente", 36000 },
  { "Alcool",     28000 }
};

int selected_fluid_index = 0; // Começa com "Agua" (índice 0)
// ---------------------------------------------------

int volume_multiplier = 1;
bool is_dispensing = false;

unsigned long dispense_start_time;
unsigned long total_dispense_duration;

void iniciar_rele() {
  pinMode(pino_rele, OUTPUT);
  digitalWrite(pino_rele, LOW);
  Serial.println("Sistema do Rele INICIADO. Rele LIGADO.");
}

void parar_rele() {
  pinMode(pino_rele, INPUT);
  Serial.println("Sistema do Rele PARADO.");
}

void set_rgb_color(int r, int g, int b) {
  analogWrite(LED_R_PIN, r);
  analogWrite(LED_G_PIN, g);
  analogWrite(LED_B_PIN, b);
}

// Função de display atualizada para mostrar o fluido
void update_display(const char* fluid_name, int current_volume_multiplier, bool dispensing, long remaining_ms) {
  if (!display_ok) return;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (!dispensing) {
    // --- TELA DE ESPERA ---
    display.setCursor(0, 0);
    display.println("Dispensador BitDogLab");
    display.drawFastHLine(0, 10, display.width(), SSD1306_WHITE);

    display.setCursor(0, 18);
    display.print("Fluido: ");
    display.println(fluid_name); // Mostra o fluido selecionado

    display.setCursor(0, 30);
    display.print("Volume: ");
    display.print(current_volume_multiplier * 50);
    display.println(" ml");

    display.setCursor(0, 45);
    display.println("Joy X:Fluido Y:Vol");
    display.setCursor(0, 55);
    display.println("A: Iniciar");

  } else {
    // --- TELA DE AÇÃO (DISPENSANDO) ---
    float remaining_seconds = remaining_ms / 1000.0;
    
    display.setTextSize(1);
    display.setCursor(5, 10);
    display.print("Dispensando ");
    display.println(fluid_name);

    display.setTextSize(1);
    display.setCursor(0, 35);
    display.print("Tempo Restante:");
    display.setCursor(25, 50);
    display.print(remaining_seconds, 1);
    display.print(" s");
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Inicializando o Dispensador...");

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    display_ok = true;
    Serial.println("Display OLED inicializado com sucesso.");
  } else {
    Serial.println("Erro ao inicializar o display.");
  }

  pinMode(botao_A, INPUT_PULLUP);
  pinMode(botao_B, INPUT_PULLUP);
  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);
  pinMode(JOYSTICK_Y_PIN, INPUT);
  pinMode(JOYSTICK_X_PIN, INPUT); // Configura o pino do eixo X

  pinMode(LED_R_PIN, OUTPUT);
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);

  parar_rele();
  set_rgb_color(0, 255, 0); // Verde: sistema pronto
  
  // Atualiza o display com o nome do fluido
  update_display(fluids[selected_fluid_index].name, volume_multiplier, false, 0);
}

void loop() {
  if (!is_dispensing) {
    // --- ESTADO DE ESPERA (IDLE) ---
    set_rgb_color(0, 255, 0);

    // Botão A: Inicia a dispensação
    if (digitalRead(botao_A) == LOW) {
      if (volume_multiplier > 0) {
        is_dispensing = true;
        // --- CÁLCULO USA O TEMPO DO FLUIDO SELECIONADO ---
        total_dispense_duration = (unsigned long)fluids[selected_fluid_index].flow_time_50ml * volume_multiplier;
        dispense_start_time = millis();

        iniciar_rele();
        set_rgb_color(0, 0, 255);
        update_display(fluids[selected_fluid_index].name, volume_multiplier, true, total_dispense_duration);
      }
      delay(200);
    }

    // --- LÓGICA DO JOYSTICK ---
    int y_value = analogRead(JOYSTICK_Y_PIN);
    int x_value = analogRead(JOYSTICK_X_PIN);

    // EIXO Y: CONTROLA O VOLUME
    if (y_value > JOY_THRESHOLD_HIGH) {
      volume_multiplier++;
      if (volume_multiplier > 10) volume_multiplier = 1;
      set_rgb_color(255, 165, 0);
      update_display(fluids[selected_fluid_index].name, volume_multiplier, false, 0);
      delay(200);
    } else if (y_value < JOY_THRESHOLD_LOW) {
      volume_multiplier--;
      if (volume_multiplier < 1) volume_multiplier = 1;
      set_rgb_color(255, 165, 0);
      update_display(fluids[selected_fluid_index].name, volume_multiplier, false, 0);
      delay(200);
    }

    // EIXO X: CONTROLA O FLUIDO
    if (x_value > JOY_THRESHOLD_HIGH) { // Joystick para a Direita
      selected_fluid_index++;
      if (selected_fluid_index >= NUM_FLUIDS) {
        selected_fluid_index = 0; // Volta para o primeiro da lista
      }
      set_rgb_color(255, 0, 255); // Roxo para indicar mudança de fluido
      update_display(fluids[selected_fluid_index].name, volume_multiplier, false, 0);
      delay(300); // Delay um pouco maior para facilitar a seleção
    } else if (x_value < JOY_THRESHOLD_LOW) { // Joystick para a Esquerda
      selected_fluid_index--;
      if (selected_fluid_index < 0) {
        selected_fluid_index = NUM_FLUIDS - 1; // Vai para o último da lista
      }
      set_rgb_color(255, 0, 255); // Roxo
      update_display(fluids[selected_fluid_index].name, volume_multiplier, false, 0);
      delay(300);
    }

  } else {
    // --- ESTADO DE AÇÃO (DISPENSANDO) ---
    unsigned long elapsed_time = millis() - dispense_start_time;
    long remaining_time = total_dispense_duration - elapsed_time;

    if (digitalRead(JOYSTICK_BUTTON) == LOW) { // Botão do Joystick como parada
      remaining_time = 0;
    }

    if (remaining_time <= 0) {
      parar_rele();
      is_dispensing = false;
      volume_multiplier = 1; // Reseta o volume para o próximo uso
      set_rgb_color(0, 255, 0);
      update_display(fluids[selected_fluid_index].name, volume_multiplier, false, 0);
    } else {
      update_display(fluids[selected_fluid_index].name, volume_multiplier, true, remaining_time);
      delay(100);
    }
  }
}
