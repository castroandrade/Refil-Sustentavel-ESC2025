# main.py para o Projeto Refil-Sustentável (MVP)
# ADAPTADO PARA A PLACA BITDOGLAB V6.3
# SBESC 2025

from machine import Pin, I2C
import utime
from ssd1306 import SSD1306_I2C

# --- 1. Configuração de Hardware ---

# Pinos para os componentes externos
RELAY_PIN = 16           # Pino para o sinal do Módulo Relé (controla a bomba)
FLOW_SENSOR_PIN = 17     # Pino para o sinal do Sensor de Vazão

# Pinos para os componentes integrados da BitDogLab
BUTTON_A_PIN = 5         # Botão 'A' será usado para iniciar o processo

# Pinos do Display OLED 
I2C_SDA_PIN = 14
I2C_SCL_PIN = 15
DISPLAY_WIDTH = 128
DISPLAY_HEIGHT = 64

# --- 2. Parâmetros de Operação ---

# Fator de calibração para o sensor YF-S201.
# Este valor converte pulsos em mililitros (ml).
# VALOR INICIAL - PRECISA SER CALIBRADO (RF08)!
# Exemplo: 1000 ml / 450 pulsos = 2.22 ml/pulso
CALIBRATION_FACTOR_ML_PER_PULSE = 2.22 

# Volume alvo para o MVP (em ml)
TARGET_VOLUME_ML = 100

# --- 3. Inicialização dos Componentes ---

# Inicializa o relé que controla a bomba (RF01)
relay = Pin(RELAY_PIN, Pin.OUT)
relay.value(0) # Garante que a bomba comece desligada

# Inicializa o Botão A da placa
start_button = Pin(BUTTON_A_PIN, Pin.IN, Pin.PULL_UP)

# Inicializa o display OLED (RF03, RF07)
# Usando o barramento I2C(1) para os pinos GP14/GP15
try:
    i2c = I2C(1, sda=Pin(I2C_SDA_PIN), scl=Pin(I2C_SCL_PIN), freq=400000)
    display = SSD1306_I2C(DISPLAY_WIDTH, DISPLAY_HEIGHT, i2c)
    display_ok = True
except Exception as e:
    print(f"ERRO: Não foi possível inicializar o display. {e}")
    display_ok = False
    
# --- 4. Variáveis Globais de Estado ---

# Variável volátil para contar os pulsos do sensor (usada na interrupção)
global pulse_count
pulse_count = 0

# Estado do sistema
dispensing = False
volume_dispensed_ml = 0.0

# --- 5. Funções Principais ---

# Função de callback para a interrupção do sensor de vazão (RF05)
def pulse_handler(pin):
    """Esta função é chamada a cada pulso detectado pelo sensor."""
    global pulse_count
    pulse_count += 1

# Função para atualizar o display com informações
def update_display(status, target, current):
    """Limpa e reescreve as informações no display OLED."""
    if not display_ok: return
    
    display.fill(0)
    display.text("Refil-Sustentavel", 0, 0)
    display.text(f"Status: {status}", 0, 15)
    display.text(f"Alvo: {target:.0f} ml", 0, 30)
    display.text(f"Atual: {current:.1f} ml", 0, 45)
    display.show()

# --- 6. Configuração da Interrupção ---

flow_sensor = Pin(FLOW_SENSOR_PIN, Pin.IN)
flow_sensor.irq(trigger=Pin.IRQ_RISING, handler=pulse_handler)


# --- 7. Loop Principal do Programa ---

# Exibe a mensagem inicial de "Pronto"
update_display("Pronto", TARGET_VOLUME_ML, volume_dispensed_ml)

while True:
    # Verifica se o Botão A foi pressionado (valor() == 0) e o sistema não está dispensando
    if not start_button.value() and not dispensing:
        
        # --- Início do Processo de Dosagem (RF04) ---
        dispensing = True
        pulse_count = 0
        volume_dispensed_ml = 0.0
        
        update_display("Dispensando...", TARGET_VOLUME_ML, volume_dispensed_ml)
        relay.value(1) # Liga a bomba (RF01)
        
        # Loop de monitoramento da dosagem
        while volume_dispensed_ml < TARGET_VOLUME_ML:
            volume_dispensed_ml = pulse_count * CALIBRATION_FACTOR_ML_PER_PULSE
            update_display("Dispensando...", TARGET_VOLUME_ML, volume_dispensed_ml)
            
            # ADICIONAL: Parada de emergência com Botão B (não estava no original, mas é uma boa prática)
            # if not button_b.value(): 
            #     break
            
            utime.sleep_ms(100)

        # --- Fim do Processo de Dosagem (RF06) ---
        relay.value(0) # Desliga a bomba
        
        final_volume = pulse_count * CALIBRATION_FACTOR_ML_PER_PULSE
        update_display("Concluido!", TARGET_VOLUME_ML, final_volume)
        utime.sleep(5)
        
        # Reseta para o próximo ciclo
        dispensing = False
        volume_dispensed_ml = 0.0
        pulse_count = 0
        update_display("Pronto", TARGET_VOLUME_ML, volume_dispensed_ml)

    utime.sleep_ms(50)