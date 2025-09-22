from machine import Pin, I2C
import utime
from ssd1306 import SSD1306_I2C

# --- 1. Configuração de Hardware ---

# Pino de SINAL do Sensor de Vazão
FLOW_SENSOR_PIN = 17

# Pinos do Display OLED
I2C_SDA_PIN = 14
I2C_SCL_PIN = 15
DISPLAY_WIDTH = 128
DISPLAY_HEIGHT = 64

# --- 2. Parâmetros de Calibração e Medição ---

# Fator de calibração inicial (ml por pulso). ESTE VALOR PRECISA SER AJUSTADO!
ML_PER_PULSE = 2.22 

# --- 3. Inicialização dos Componentes ---

# Inicializa o display OLED
try:
    i2c = I2C(1, sda=Pin(I2C_SDA_PIN), scl=Pin(I2C_SCL_PIN), freq=400000)
    display = SSD1306_I2C(DISPLAY_WIDTH, DISPLAY_HEIGHT, i2c)
    display_ok = True
    print("Display OLED inicializado.")
except Exception as e:
    print(f"ERRO: Não foi possível inicializar o display. {e}")
    display_ok = False

# --- 4. Variáveis Globais e Configuração da Interrupção ---

# Variável global para ser incrementada pela interrupção
# É 'volátil' - pode ser alterada a qualquer momento pelo hardware
global pulse_count
pulse_count = 0

def pulse_handler(pin):
    """Função de callback que é executada a cada pulso do sensor."""
    global pulse_count
    pulse_count += 1

# Configura o pino do sensor como entrada e anexa a interrupção
# A interrupção será acionada na 'borda de subida' (quando o sinal vai de LOW para HIGH)
flow_sensor = Pin(FLOW_SENSOR_PIN, Pin.IN)
flow_sensor.irq(trigger=Pin.IRQ_RISING, handler=pulse_handler)

# --- 5. Loop Principal de Monitoramento e Exibição ---

print("Iniciando monitoramento do sensor de vazão.")
print("Passe líquido pelo sensor para ver os dados...")

last_pulse_check = 0
last_time_check = utime.ticks_ms()

# Limpa o display e exibe a mensagem inicial
if display_ok:
    display.fill(0)
    display.text("Aguardando fluxo", 0, 30)
    display.show()

while True:
    # Calcula a vazão (frequência de pulsos) a cada segundo
    current_time = utime.ticks_ms()
    if utime.ticks_diff(current_time, last_time_check) >= 1000:
        
        # Desativa temporariamente as interrupções para ler o contador com segurança
        # Isso evita que o valor de 'pulse_count' mude no meio de um cálculo
        irq_state = machine.disable_irq()
        
        # Calcula a frequência (pulsos por segundo = Hz)
        frequency = pulse_count - last_pulse_check
        
        # Lê o valor total de pulsos
        total_pulses = pulse_count
        
        # Reativa as interrupções
        machine.enable_irq(irq_state)
        
        # Atualiza as variáveis para o próximo segundo
        last_pulse_check = total_pulses
        last_time_check = current_time
        
        # Calcula o volume total dispensado
        total_volume_ml = total_pulses * ML_PER_PULSE
        
        # Imprime os dados no console do Thonny (ótimo para debugging)
        print(f"Vazao: {frequency} Hz | Pulsos Totais: {total_pulses} | Volume Total: {total_volume_ml:.2f} ml")
        
        # Atualiza o display OLED
        if display_ok:
            display.fill(0)
            display.text("Monitor de Vazao", 0, 0)
            display.text("----------------", 0, 10)
            display.text(f"Vazao: {frequency} Hz", 0, 25)
            display.text(f"Volume: {total_volume_ml:.1f}ml", 0, 40)
            display.text(f"Pulsos: {total_pulses}", 0, 50)
            display.show()
            
    # Pequena pausa para o loop não consumir 100% da CPU
    utime.sleep_ms(10)
