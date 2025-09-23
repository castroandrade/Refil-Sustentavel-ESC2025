/*
 * Código para controlar um Módulo Relé com dois botões na Raspberry Pi Pico.
 *
 * Conexões do Relé:
 * - VCC do relé -> VBUS (Pino 40) da Pico para obter 5V
 * - GND do relé -> GND (qualquer pino GND) da Pico
 * - IN do relé  -> GP16 (Pino 21) da Pico
 *
 * Conexões dos Botões:
 * - Botão A: Um lado no GP5 (Pino 7), o outro lado em um pino GND.
 * - Botão B: Um lado no GP6 (Pino 9), o outro lado em um pino GND.
 */

// Define os pinos que serão usados
const int pino_rele = 16;
const int botao_A = 5;
const int botao_B = 6;

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

void setup() {
  // Inicializa a comunicação serial.
  Serial.begin(115200);
  Serial.println("Iniciando o controle do rele com botoes...");

  // Configura os pinos dos botões como ENTRADA com resistor de pull-up interno.
  // Isso significa que o pino lerá HIGH por padrão e LOW quando o botão for pressionado.
  pinMode(botao_A, INPUT_PULLUP);
  pinMode(botao_B, INPUT_PULLUP);

  // Começa com o sistema do relé parado para economizar energia.
  parar_rele();
}

void loop() {
  // Verifica o estado do botão A
  // digitalRead(botao_A) == LOW significa que o botão foi pressionado
  if (digitalRead(botao_A) == LOW) {
    iniciar_rele();
    // Um pequeno delay para evitar que a função seja chamada múltiplas vezes
    // enquanto o botão está pressionado (efeito "debounce").
    delay(200);
  }

  // Verifica o estado do botão B
  if (digitalRead(botao_B) == LOW) {
    parar_rele();
    delay(200);
  }
}
