from machine import Pin, I2C, PWM
import utime
from ssd1306 import SSD1306_I2C

# --- 1. Configuração dos Pinos (CONFORME DOCUMENTAÇÃO V6.3) ---

# Pinos do Display OLED (I2C)
# De acordo com a documentação, SDA é GP14 e SCL é GP15.
# Estes pinos correspondem ao barramento I2C(1) no Pico.
I2C_SDA_PIN = 14
I2C_SCL_PIN = 15

# Pinos dos Botões de Usuário
BUTTON_A_PIN = 5
BUTTON_B_PIN = 6

# Pinos do LED RGB (controlado via PWM)
LED_R_PIN = 13
LED_G_PIN = 11
LED_B_PIN = 12

# Pino para o LED externo (usando um pino livre)
EXTERNAL_LED_PIN = 16 

# Pino do LED integrado na placa Raspberry Pi Pico W
ONBOARD_LED_PIN = "LED"

# --- 2. Inicialização dos Componentes ---

print("Inicializando componentes da BitDogLab v6.3...")

# Inicializa o Display OLED
try:
    # Usando I2C(1) para os pinos GP14 e GP15
    i2c = I2C(1, sda=Pin(I2C_SDA_PIN), scl=Pin(I2C_SCL_PIN), freq=400000)
    display = SSD1306_I2C(128, 64, i2c)
    display_ok = True
    print("Display OLED inicializado com sucesso.")
except Exception as e:
    print(f"Erro ao inicializar o display: {e}")
    display_ok = False

# Inicializa os Botões A e B
button_a = Pin(BUTTON_A_PIN, Pin.IN, Pin.PULL_UP)
button_b = Pin(BUTTON_B_PIN, Pin.IN, Pin.PULL_UP)

# Inicializa o LED RGB com PWM
pwm_r = PWM(Pin(LED_R_PIN))
pwm_g = PWM(Pin(LED_G_PIN))
pwm_b = PWM(Pin(LED_B_PIN))
pwm_r.freq(1000)
pwm_g.freq(1000)
pwm_b.freq(1000)

# Inicializa o LED Externo e o LED Integrado
external_led = Pin(EXTERNAL_LED_PIN, Pin.OUT)
onboard_led = Pin(ONBOARD_LED_PIN, Pin.OUT)

# --- 3. Variáveis e Funções Auxiliares ---

# Cores para o LED RGB. O valor vai de 0 (desligado) a 65535 (máximo brilho).
# Tuplas no formato (Vermelho, Verde, Azul)
COLORS = [
    (65535, 0, 0),       # Vermelho
    (0, 65535, 0),       # Verde
    (0, 0, 65535),       # Azul
    (65535, 65535, 0),   # Amarelo
    (0, 0, 0)            # Desligado
]
color_index = 0

def set_rgb_color(r, g, b):
    """Define a cor do LED RGB usando valores de PWM."""
    pwm_r.duty_u16(r)
    pwm_g.duty_u16(g)
    pwm_b.duty_u16(b)

def update_display(bA_state, bB_state, led_state):
    """Função para desenhar as informações no display."""
    if not display_ok: return
    
    display.fill(0)
    display.text("BitDogLab v6.3", 0, 0)
    display.text("---------------", 0, 10)
    display.text(f"Botao A: {bA_state}", 0, 25)
    display.text(f"Botao B: {bB_state}", 0, 35)
    display.text(f"LED Ext: {led_state}", 0, 45)
    display.show()

# --- 4. Loop Principal de Teste ---

print("Teste iniciado. Pressione os botões A e B.")
if display_ok:
    display.fill(0)
    display.text("Teste Iniciado!", 5, 30)
    display.show()
    utime.sleep(1)

# Desliga o RGB no início
set_rgb_color(0, 0, 0)
led_externo_ligado = False

while True:
    onboard_led.toggle()

    # Leitura do Botão A (controla o LED externo)
    if not button_a.value():
        estado_bA = "Pressionado!"
        led_externo_ligado = not led_externo_ligado
        external_led.value(1 if led_externo_ligado else 0)
    else:
        estado_bA = "Solto"

    # Leitura do Botão B (controla o LED RGB)
    if not button_b.value():
        estado_bB = "Pressionado!"
        color_index = (color_index + 1) % len(COLORS)
        r, g, b = COLORS[color_index]
        set_rgb_color(r, g, b)
    else:
        estado_bB = "Solto"
        
    estado_led = "Ligado" if led_externo_ligado else "Desligado"
    
    update_display(estado_bA, estado_bB, estado_led)
    
    utime.sleep_ms(150)
