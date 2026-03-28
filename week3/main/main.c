#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"

const char* TAG = "MAIN";

#define GREEN_LED_PIN 27
#define RED_LED_PIN 14
#define BLUE_LED_PIN 12

#define ARM_PIN 18
#define DISARM_PIN 19
#define CODE0_PIN 21
#define CODE1_PIN 22
#define SETCODE_PIN 23

#define DEBOUNCE 250 * 1000
#define ALARM_PERIOD 200 * 1000

// Input pin config
gpio_config_t input_pin = {
    .pin_bit_mask = (1ULL << ARM_PIN) |
                    (1ULL << DISARM_PIN) |
                    (1ULL << CODE0_PIN) |
                    (1ULL << CODE1_PIN) |
                    (1ULL << SETCODE_PIN),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = //negative edge interrupt
};

// Output pin config
gpio_config_t output_pin = {
    .pin_bit_mask = (1ULL << RED_LED_PIN) |
                    (1ULL << GREEN_LED_PIN) |
                    (1ULL << BLUE_LED_PIN),
    .mode = GPIO_MODE_OUTPUT,
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE
};

// Alarm variables
bool alarm_triggered = false;
bool alarm_flash = false;
bool alarm_armed = false;
bool setting_code = false;
uint8_t code = 0b00000000;
uint8_t temp_code = 0b00000000;
uint8_t guess_code = 0b00000000;

// Debounce variables
intmax_t previous_alarm_beep = 0;
intmax_t previous_arm_press = 0;
intmax_t previous_disarm_press = 0;
intmax_t previous_setcode_press = 0;
intmax_t previous_code1_press = 0;
intmax_t previous_code0_press = 0;

//Flags for print statements
volatile bool newly_armed = false;
volatile bool newly_disarmed = false;
volatile bool new_code_set = false;
volatile bool newly_triggered = false;

// ------------------------ Helper Functions ------------------------

void print_code(uint8_t val, const char* msg) {
    ESP_LOGI(TAG, "%s: %c%c%c%c%c%c%c%c", msg,
        val & 0x80 ? '1' : '0',
        val & 0x40 ? '1' : '0',
        val & 0x20 ? '1' : '0',
        val & 0x10 ? '1' : '0',
        val & 0x08 ? '1' : '0',
        val & 0x04 ? '1' : '0',
        val & 0x02 ? '1' : '0',
        val & 0x01 ? '1' : '0');
}

bool debounce_check(intmax_t* last_press) {
    int64_t now = esp_timer_get_time();
    if ((now - *last_press) > DEBOUNCE) {
        *last_press = now;
        return true;
    }
    return false;
}

// ------------------------ Interrupt Handlers -----------------------

// ARM
static void handle_arm_press(void *arg) {
    if (!debounce_check(&previous_arm_press)) {return;}

    //if the alarm is unarmed and we aren't setting the code
    //1. set armed = true
    //2. set newly_armed = true
    //3. reset guess code to 0
}

// DISARM
static void handle_disarm_press(void *arg) {
    if (!debounce_check(&previous_arm_press)) {return;}

    if (alarm_armed) {
        // if guess code is correct
        // disarm the alarm
        // set alarm_triggered = false
        // set newly_disarmed flag = true
        // reset guess code to 0
        
        // if guess code is wrong
        // set newly_triggered flag = true
        // set alarm_triggered = true
    }
}

// SETCODE
static void handle_setcode_press(void *arg) {
    if (!debounce_check(&previous_arm_press)) {return;}

    // first press
    // if alarm is not armed and setting_code is false
    // set setting_code = true
    // reset temp code to 0
    
    // second press
    // if setting_code is true
    // set code = temp_code
    // set setting_code = false
    // set new_code_set flag = true
}

// CODE 0
static void handle_code0_press(void *arg) {
    if (!debounce_check(&previous_arm_press)) {return;}

    // if setting code is true
    // shift a 0 into temp_code
    
    // if alarm is armed
    // shift a 0 into guess_code
}

// CODE 1
static void handle_code1_press(void *arg) {
    if (!debounce_check(&previous_arm_press)) {return;}

    // if setting code is true
    // shift a 1 into temp_code
    
    // if alarm is armed
    // shift a 1 into guess_code
}

void update_leds(void) {
    // Armed/Disarmed LEDs
    if (!alarm_triggered) {
        gpio_set_level(RED_LED_PIN, alarm_armed ? 1 : 0);
        gpio_set_level(GREEN_LED_PIN, alarm_armed ? 0 : 1);
    }

    // Triggered alarm flashing
    if (alarm_triggered && (esp_timer_get_time() - previous_alarm_beep > ALARM_PERIOD)) {
        alarm_flash = !alarm_flash;
        gpio_set_level(RED_LED_PIN, alarm_flash);
        previous_alarm_beep = esp_timer_get_time();
    }

    // Setting code LED
    gpio_set_level(BLUE_LED_PIN, setting_code ? 1 : 0);
}


void setup_gpio_irq() {
    //call gpio_install_isr_service for ESP_INTR_FLAG_EDGE (since we're using negative edge triggered interrupts)

    // call gpio_isr_handler_add() 5 times, once for each of the 5 buttons
    // each call has 3 arguments,
    // 1. the pin number (use the macros defined above)
    // 2. the function name 
    //3 . NULL
}

// ------------------------ Main Application ------------------------

void app_main(void) {
    // Setup
    gpio_config(&input_pin);
    gpio_config(&output_pin);

    setup_gpio_irq();

    gpio_set_level(RED_LED_PIN, 0);
    gpio_set_level(BLUE_LED_PIN, 0);
    gpio_set_level(GREEN_LED_PIN, 0);

    vTaskDelay(pdMS_TO_TICKS(100));

    // Main loop
    while (1) {
        update_leds();

        if (newly_armed) {
            newly_armed = false;
            ESP_LOGI(TAG, "SYSTEM ARMED!\n");
        }
        if (newly_disarmed) {
            newly_disarmed = false;
            ESP_LOGI(TAG, "SYSTEM DISARMED!\n");
        }
        if (newly_triggered) {
            newly_triggered = false;
            ESP_LOGI(TAG, "ALARM TRIGGERED!\n");
        }
        if (new_code_set) {
            new_code_set = false;
            print_code(code, "New Code Set:");
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}