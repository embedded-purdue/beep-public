#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "driver/rmt_tx.h"  
#include "driver/gpio.h"
#include "helpers.h"

const char* TAG = "MAIN";

//STEP MACROS
#define STEP1
//#define BONUS

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
    .intr_type = GPIO_INTR_NEGEDGE
};

// Alarm State
bool alarm_triggered = false;
bool alarm_armed = false;
bool setting_code = false;
uint8_t code = 0b00000000;
uint8_t temp_code = 0b00000000;
uint8_t guess_code = 0b00000000;

//RGB LED
int brightness = 0;
int step = 50;

// Button Debounce Timestamps
intmax_t previous_arm_press = 0;
intmax_t previous_disarm_press = 0;
intmax_t previous_setcode_press = 0;
intmax_t previous_code1_press = 0;
intmax_t previous_code0_press = 0;

//ISR Flags for Print Statements
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
    if ((now - *last_press) > DEBOUNCE_TIME) {
        *last_press = now;
        return true;
    }
    return false;
}

void update_leds(void) {
    //Base Set Conditions (use ledc_set_duty with high speed mode and the corresponding ledc_channel from your pwm_setup function for each color)
    //Note for this section: you may find ternary operators useful here, they work like this: (condition) ? value if true : value if false

    //BLUE: update the duty cycle to DUTY_CYCLE_MAX if we are setting the code and the alarm is neither armed nor triggered, update to 0 otherwise
    //GREEN: update the duty cycle to DUTY_CYCLE_MAX if we aren't setting the code and the alarm is neither armed nor triggered, update to 0 otherwise
    //RED: update the duty cycle to DUTY_CYCLE_MAX if the alarm is armed, update to 0 otherwise

    //Red Alarm Flashing
    /*  if (alarm is triggered and the timer is triggered) {
            set timer_triggered = false
            increment the global brightness variable by step
            if (brightness is 0 or DUTY_CYCLE_MAX) {
                flip the count direction of the step variable
            }
            set the duty cycle of the RED LED to brightness
        }                
    */

    //call ledc_update_duty() once for each of the 3 color channels
}

// ------------------------ Interrupt Handlers -----------------------

// ARM
static void handle_arm_press(void *arg) {
    if (!debounce_check(&previous_arm_press)) {return;}

    if (!alarm_armed && !setting_code) {
        alarm_armed = true;
        newly_armed = true;
        guess_code = 0b00000000;
    }
}

// DISARM
static void handle_disarm_press(void *arg) {
    if (!debounce_check(&previous_disarm_press)) {return;}

    if (alarm_armed) {
        if (guess_code == code) {
            alarm_armed = false;
            alarm_triggered = false;
            newly_disarmed = true;
            guess_code = 0b00000000;
        } else {
            newly_triggered = true;
            alarm_triggered = true;
        }
    }
}

// SETCODE
static void handle_setcode_press(void *arg) {
    if (!debounce_check(&previous_setcode_press)) {return;}

    // first press
    if (!alarm_armed && !setting_code) {
        setting_code = true;
        temp_code = 0;
    }
    // second press
    else if (setting_code) {
        code = temp_code;
        setting_code = false;
        new_code_set = true;
    }
}

// CODE 0
static void handle_code0_press(void *arg) {
    if (!debounce_check(&previous_code0_press)) {return;}

    // Code set press
    if (setting_code) {
        temp_code <<= 1;
    }
    // Code guess press
    else if (alarm_armed) {
        guess_code <<= 1;
    }
}

// CODE 1
static void handle_code1_press(void *arg) {
    if (!debounce_check(&previous_code1_press)) {return;}

    // Code set press
    if (setting_code) {
        temp_code = (temp_code << 1) | 1;
    }
    // Code guess press
    else if (alarm_armed) {
        guess_code = (guess_code << 1) | 1;
    }
}

// ------------------------ Setup -----------------------

void setup_gpio_irq() {
    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);

    gpio_isr_handler_add(ARM_PIN,handle_arm_press,NULL);
    gpio_isr_handler_add(DISARM_PIN,handle_disarm_press,NULL);
    gpio_isr_handler_add(SETCODE_PIN,handle_setcode_press,NULL);
    gpio_isr_handler_add(CODE0_PIN,handle_code0_press,NULL);
    gpio_isr_handler_add(CODE1_PIN,handle_code1_press,NULL);
}

// ------------------------ Main Application ------------------------

void app_main(void) {
    //Setup
    #ifdef STEP1  
        gpio_config(&input_pin);
        setup_gpio_irq();
        pwm_setup();
        timer_setup(1000000, 10000); //1us ticks, 100Hz alarm
    #endif
    #ifdef BONUS
        int8_t strip_brightness[8] = {32, 64, 96, 128, 160, 192, 224, 255};
        int offset = 0;
        int direction = 1;
        int pixel = 0;
        int target = 0;
        rgb_strip_setup();
    #endif

    vTaskDelay(pdMS_TO_TICKS(100));

    // Main loop
    while (1) {
        #ifdef STEP1
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
        #endif
        #ifdef BONUS
            //build current iteration pattern array
            int8_t pattern[24] = {0};
            target = (pixel*3) + offset;
            pattern[target] = strip_brightness[pixel];
            //transmit
            rmt_transmit(rmt_handle,encoder_handle,pattern,24,&rmt_tx_config);
            //increment
            pixel = pixel + direction;
            if (pixel==7) {
                direction = -1;
            }
            if (pixel==0) {
                direction = 1;
                offset = (offset+1) % 3;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        #endif
    }
}