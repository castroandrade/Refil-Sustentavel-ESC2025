# 💧 Refil Sustentável - ESC 2025

![Badge C++](https://img.shields.io/badge/Language-C++-blue)
![Badge Hardware](https://img.shields.io/badge/Hardware-BitDogLab%20%7C%20Pico%20W-red)
![Badge IoT](https://img.shields.io/badge/IoT-Webserver%20%2B%20API-green)
![Badge Payment](https://img.shields.io/badge/Integration-Mercado%20Pago%20Pix-lightblue)

> **Projeto desenvolvido para a ESC 2025 (Embedded Systems Competition).**
> Uma solução IoT para automação de dispensers de líquidos a granel, integrando controle de fluxo preciso, interface web local e pagamentos via Pix em tempo real.

---

## 📋 Sobre o Projeto

O **Refil Sustentável** é um protótipo de *Vending Machine* inteligente focado na redução de resíduos plásticos. A ideia é permitir que o usuário reutilize seus próprios recipientes para comprar produtos de limpeza ou higiene (água, detergente, álcool) a granel.

O sistema é controlado pela placa de desenvolvimento **BitDogLab** (baseada no Raspberry Pi Pico W), oferecendo uma experiência de usuário dupla: através de um **Dashboard Web Responsivo** (acessível via celular sem instalar apps) e controles físicos na própria máquina.

### 🚀 Diferenciais Técnicos
*   **Integração Fintech:** Geração dinâmica de QR Codes Pix via API do Mercado Pago (HTTPS/SSL).
*   **Dual UI/UX:** Sincronização em tempo real entre o Display OLED físico e a Interface Web (Single Page Application com AJAX).
*   **Gestão de Estoque:** Monitoramento do nível do tanque e bloqueio automático em caso de volume insuficiente.
*   **Multitarefa:** Arquitetura não-bloqueante (sem `delay()`), permitindo manter o Wi-Fi, servidor web, leitura de sensores e atualização de display simultaneamente.

---

## 🛠️ Hardware Utilizado

O projeto foi desenvolvido utilizando a placa educacional **BitDogLab**, composta por:

*   **Microcontrolador:** Raspberry Pi Pico W (Dual-core ARM Cortex M0+ com Wi-Fi).
*   **Display:** OLED 0.96" I2C (SSD1306).
*   **Interface de Entrada:** Joystick Analógico (eixos X/Y + botão) e Botão Tátil A.
*   **Indicação Visual:** LED RGB.
*   **Atuador:** Módulo Relé (acionamento de bomba peristáltica ou solenoide).
*   **Conectividade:** Cabo Micro-USB para alimentação e debug.

---

## 💻 Tecnologias e Bibliotecas

O firmware foi desenvolvido em C++ utilizando a **Arduino IDE** com o Core **Raspberry Pi Pico (Earle F. Philhower)**.

| Biblioteca | Função |
| :--- | :--- |
| `WiFi.h` | Conectividade de rede. |
| `WebServer.h` | Servidor HTTP local para hospedar a aplicação web. |
| `ArduinoJson` (v7) | Processamento de respostas JSON da API do Mercado Pago e comunicação AJAX. |
| `Adafruit_SSD1306` | Controle gráfico do display OLED. |
| `WiFiClientSecure` | Comunicação segura (HTTPS) para transações financeiras. |

---

## ⚙️ Funcionalidades

### 1. Interface Web (Mobile First)
*   **Dashboard:** Mostra o fluido selecionado, volume atual, nível do tanque e status da conexão.
*   **Controles:** Seleção de volume (25ml, 50ml, 100ml, 150ml), troca de fluido e início da operação.
*   **Pagamento:** Botão para alternar entre "Modo Grátis" e "Modo Pago".
*   **Pix:** Exibe o QR Code Pix gerado e o código "Copia e Cola" diretamente na tela do celular.

### 2. Interface Física (BitDogLab)
*   **Joystick Y:** Aumenta/Diminui o volume desejado.
*   **Joystick X:** Alterna entre os fluidos disponíveis.
*   **Display OLED:** Feedback visual imediato de todas as ações.
*   **LED RGB:** Feedback de status (Verde: Pronto / Azul: Enchendo / Amarelo: Aguardando Pagamento / Vermelho: Erro).

### 3. Sistema de Pagamento
*   Conecta-se à API do Mercado Pago via TLS.
*   Gera uma cobrança com valor dinâmico baseada no volume selecionado (`Volume * Preço/ml`).
*   Monitora o status do pagamento a cada 3 segundos.
*   Libera o relé automaticamente após a aprovação ("approved").

---

## 🚀 Como Executar o Projeto

### Pré-requisitos
1.  **Arduino IDE** instalada.
2.  Adicionar URL do gerenciador de placas: `https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json`
3.  Instalar o pacote **Raspberry Pi Pico/RP2040** via Boards Manager.
4.  Instalar as bibliotecas listadas acima via Library Manager.

### Configuração
1.  Clone este repositório:
    ```bash
    git clone https://github.com/seu-usuario/refil-sustentavel-esc2025.git
    ```
2.  Abra o arquivo `.ino` na Arduino IDE.
3.  Edite as seguintes linhas com suas credenciais:
    ```cpp
    const char* ssid = "SUA_REDE_WIFI";
    const char* password = "SUA_SENHA_WIFI";
    const char* mp_access_token = "SEU_ACCESS_TOKEN_MERCADO_PAGO";
    ```
    > **Nota:** Para obter o token, crie uma aplicação em [Mercado Pago Developers](https://www.mercadopago.com.br/developers).

4.  Selecione a placa **Raspberry Pi Pico W** e a porta correta.
5.  Faça o upload.

### Uso
1.  Após o upload, o IP da placa aparecerá no Display OLED.
2.  Conecte seu celular na mesma rede Wi-Fi.
3.  Digite o IP no navegador (ex: `http://192.168.1.105`).
4.  Utilize a interface para controlar o dispenser.

---

## 🛡️ Tratamento de Erros e Segurança

*   **Watchdog de Upload:** Delay inicial de 3s no boot para garantir que a porta USB seja montada antes de qualquer travamento lógico, facilitando re-uploads.
*   **Fail-safe de I2C:** Verificação de inicialização do display para não travar o loop principal.
*   **Anti-Fraude Pix:** Geração de e-mails de pagador dinâmicos para evitar bloqueios de duplicidade na API do Mercado Pago durante testes repetitivos.
*   **Validação de Tanque:** O sistema impede a venda se o volume solicitado for maior que o volume disponível no tanque virtual.

---

## 👥 Equipe

Projeto desenvolvido para a ESC 2025 por:

*   **José Henrique Castro Andrade** - Desenvolvedor Firmware & Integração
*   **Jean Lucas** - Design da manufatura
*   **Victor Henrick** - Desenvolvedor Firmware & Integração

---

## 📄 Licença

Este projeto está sob a licença MIT. Sinta-se livre para usar e modificar para fins educacionais e competições.

---
*Desenvolvido com 💙 e C++ na BitDogLab.*
