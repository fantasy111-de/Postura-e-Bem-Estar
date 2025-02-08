#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "hardware/dma.h"
#include <stdint.h>


//Definição dos pinos
#define JOY_X_PIN 26  // ADC0 (GPIO26)
#define JOY_Y_PIN 27  // ADC1 (GPIO27)
#define BUZZER_PIN 21  
#define LED_PIN 7
#define LED_COUNT 25

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

//buzzer
void setup_buzzer() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice, 12500);  // Ajuste de acordo com a frequência desejada
    pwm_set_enabled(slice, true);
}

void trigger_alarm() {
    for (int i = 0; i < 3; i++) {  // Repetindo o alerta 2 vezes
        pwm_set_gpio_level(BUZZER_PIN, 6250);  // Ativando o som
        sleep_ms(500);  // Som por 500ms
        pwm_set_gpio_level(BUZZER_PIN, 0);  // Desligando o som
        sleep_ms(500);  // Pausa de 500ms

        pwm_set_gpio_level(BUZZER_PIN, 6250);  // Novo som
        sleep_ms(1000);  // Som por 1000ms
        pwm_set_gpio_level(BUZZER_PIN, 0);  // Desligando o som
        sleep_ms(1000);  // Pausa maior
    }
}




// Limites ajustáveis do joystick
#define POSTURE_MIN 1800  
#define POSTURE_MAX 2300 
PIO np_pio;
uint sm;

void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, true);
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
}




// simulação da qualidade do ar e luminosidade
int luminosity = 50;  // Valor de luminosidade (simulado)
int air_quality = 75;

int read_luminosity() {
    return rand() % 101;  // Simulação de valor de luminosidade (0 a 100)
}

int read_air_quality() {
    return rand() % 101;  // Simulação de qualidade do ar (0 a 100)
}



// Configuração da matriz de leds
typedef struct {
    uint8_t G, R, B;
} pixel_t;


pixel_t leds[LED_COUNT]; // Buffer de LEDs

void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}
void npWrite() {
    for (uint i = 0; i < LED_COUNT; i++) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

int getIndex(int x, int y) {
    return (y % 2 == 0) ? 24 - (y * 5 + x) : 24 - (y * 5 + (4 - x));
}
void clear_leds() {
    for (int i = 0; i < LED_COUNT; i++) {
        npSetLED(i, 0, 0, 0);
    }
  
}

void bad_posture_animation() {
    int x_pattern[][2] = {{0,0}, {4,0}, {1,1},{3,1},{2,2},{1,3},{3,3},{0,4},{4,4}};
    int num_x_leds = sizeof(x_pattern)/ sizeof(x_pattern[0]);
    for (int i = 0; i < num_x_leds; i++) {
        int x = x_pattern[i][0];
        int y = x_pattern[i][1];
        int index = getIndex(x,y);
        npSetLED(index,255,0,0);
    }
    npWrite();
    sleep_ms(500);
    clear_leds();
    npWrite(); // Atualiza os LEDs após limpar
}

void good_posture_animation() {
    int check_pattern[][2] = {{1,3},{2,4},{3,2},{3,3},{4,1}};
    int num_check_leds = sizeof(check_pattern) / sizeof(check_pattern[0]);
    
    for (int i = 0; i < num_check_leds; i++) {
        int x = check_pattern[i][0];
        int y = check_pattern[i][1];
        int index = getIndex(x, y);
        npSetLED(index, 0, 255, 0); 
    }
    npWrite();
    sleep_ms(1500);
    clear_leds();
    npWrite(); // Atualiza os LEDs após limpar
}

void sun_animation() {
    clear_leds();
    for (int step = 0; step < 5; step++) {
        uint8_t brightness = (step % 2 == 0) ? 100 : 255;
        for (int i = 0; i < LED_COUNT; i++) {
            npSetLED(i, brightness, brightness, 0); // Amarelo pulsante
        }
        npWrite();
        sleep_ms(200);
    }
    clear_leds();  // Limpa após o loop
    npWrite();     // Atualiza o hardware
}

void sun() {
    clear_leds();
    for (int step = 0; step < 5; step++) {
        uint8_t brightness = (step % 2 == 0) ? 100 : 255;
        for (int i = 0; i < LED_COUNT; i++) {
            npSetLED(i, brightness, brightness, 0); // Amarelo pulsante
        }
        npWrite();
        sleep_ms(200);
    }
    clear_leds();  // Limpa após o loop
    npWrite();     // Atualiza o hardware
}

void moon() {
    int moon_pattern[][2] = {{2,0}, {1,1}, {2,1}, {3,1}, {2,2}, {1,3}, {2,3}, {3,3}, {2,4}};
    int num_moon_leds = sizeof(moon_pattern)/sizeof(moon_pattern[0]);

    for (int step = 0; step < 5; step++) {
        clear_leds();
        uint8_t brightness = (step % 2 == 0) ? 100 : 255;
        for (int i = 0; i < num_moon_leds; i++) {
            int x = moon_pattern[i][0];
            int y = moon_pattern[i][1];
            int idx = getIndex(x, y);
            npSetLED(idx, 0, 0, brightness); // Azul pulsante
        }
        npWrite();
        sleep_ms(200);
    }
    clear_leds();  // Limpa após o loop
    npWrite();     // Atualiza o hardware
}
void air_quality_animation(int air_quality) {
    clear_leds();

    // Se a qualidade do ar for boa (acima de 70%), os LEDs piscam azul
    if (air_quality > 70) {
        printf("Qualidade do ar boa, LEDs azuis piscando...\n");
        for (int i = 0; i < 5; i++) {  // Pisca 5 vezes
            for (int j = 0; j < LED_COUNT; j++) {
                npSetLED(j, 0, 0, 255);  // Azul
            }
            npWrite();
            sleep_ms(200);
            clear_leds();
            npWrite();
            sleep_ms(200);
        }
    }
    // Se a qualidade do ar for ruim (abaixo de 30%), os LEDs piscam vermelhos
    else if (air_quality < 30) {
        printf("Qualidade do ar ruim, LEDs vermelhos piscando...\n");
        for (int i = 0; i < 5; i++) {  // Pisca 5 vezes
            for (int j = 0; j < LED_COUNT; j++) {
                npSetLED(j, 255, 0, 0);  // Vermelho
            }
            npWrite();
            sleep_ms(200);
            clear_leds();
            npWrite();
            sleep_ms(200);
        }
    } else {
        printf("Qualidade do ar média, LEDs em estado normal.\n");
        clear_leds();
        npWrite();  // Mantém LEDs apagados se a qualidade do ar for média
    }
}

// Configuração do Joystick

void setup_adc() {
    adc_init();
    adc_gpio_init(JOY_X_PIN);
    adc_gpio_init(JOY_Y_PIN);
}

uint16_t read_joystick(uint channel) {
    adc_select_input(channel);
    return adc_read();
}

bool check_posture() {
    uint16_t x = read_joystick(0);
    uint16_t y = read_joystick(1);
    return (x < POSTURE_MIN || x > POSTURE_MAX || y < POSTURE_MIN || y > POSTURE_MAX);
}

//display
struct render_area frame_area;
uint8_t ssd[ssd1306_buffer_length]; 
void display_message(const char *message, int y_pos) {
    memset(ssd, 0, ssd1306_buffer_length);  // Limpa o buffer
    ssd1306_draw_string(ssd, 5, y_pos, message);
    render_on_display(ssd, &frame_area);  // Atualizar o display
    sleep_ms(1000);  // Aumentar o tempo de exibição da mensagem
}
void display_luminosity() {
    char luminosity_str[16];
    luminosity = read_luminosity();  // Simular leitura de luminosidade
    snprintf(luminosity_str, sizeof(luminosity_str), "Lum: %d%%", luminosity);
    display_message(luminosity_str, 40);
}

void display_air_quality() {
    char air_quality_str[16];
    air_quality = read_air_quality();  // Simular leitura de qualidade do ar
    snprintf(air_quality_str, sizeof(air_quality_str), "Ar: %d%%", air_quality);
    display_message(air_quality_str, 50);
}

void display_correct_posture() {
    display_message("Postura Correta", 40);  // Exibe a mensagem de postura correta
}
void display_incorrect_posture() {
    display_message("Postura Ruim", 40);  // Exibe a mensagem de postura incorreta
}


void init_display() {
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();

    frame_area.start_column = 0;
    frame_area.end_column = ssd1306_width - 1;
    frame_area.start_page = 0;
    frame_area.end_page = ssd1306_n_pages - 1;
    calculate_render_area_buffer_length(&frame_area);

    char *text[] = {"  Postura e", "Bem-Estar"};
    int y = 0;
    for (uint i = 0; i < 2; i++) {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }
    render_on_display(ssd, &frame_area);
    sleep_ms(2000);
}

int main() {
    stdio_init_all();
    setup_adc();
    setup_buzzer();

    npInit(LED_PIN);
    init_display();

    

    while (true) {

        if (check_posture()) {
            printf("Má postura detectada!\n");
            bad_posture_animation();
            display_incorrect_posture(); 
            trigger_alarm();
        } else {
            printf("Postura correta :)\n");
            display_correct_posture();
            good_posture_animation();
        }
        luminosity = read_luminosity();  // Lê o valor de luminosidade
        display_luminosity();  // Exibe o valor de luminosidade no display
        
        // Define um limiar para alternar entre sol e lua
        if (luminosity > 50) {
            sun();  // Exibe o sol quando a luminosidade for maior que 50%
        } else {
            moon();  // Exibe a lua quando a luminosidade for menor ou igual a 50%
        }
        // Exibe a lua com base na luminosidade
        air_quality = read_air_quality();  
        display_air_quality();
        air_quality_animation(air_quality); 




        sleep_ms(500);
    }
    return 0;
}