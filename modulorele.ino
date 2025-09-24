#define JOYSTICK_BUTTON 22
#define JOYSTICK_X 26
#define JOYSTICK_Y 27
#define pino_rele  16
#define botao_A  5
#define botao_B  6

const int vazao_ml_30s = 2000;
const int tempo_vazao_100ml = 6000000/vazao_ml_30s;
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
    // Deley para preenchimento medio de 100ml a uma vazão de 4L por min 
    delay(tempo_vazao_100ml);
    parar_rele();
  }
  // Verifica o estado do botão B
  if (digitalRead(botao_B) == LOW) {
    parar_rele();
    delay(200);
  }
}
