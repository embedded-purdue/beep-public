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

/*  
The easiest way to fill in these configs is to take advantage of intellisense.
By using either ctrl+left click or right click + selecting go to definition on the 
config type or specific .member fields you can see the definition and some helpful docs
for each piece present in driver/gpio.h. This applies for any function included from a
header file, even ones you will write later! These headers often have their own global 
macros defined which you can use here to fill in the blanks.
*/
// Input pin config
gpio_config_t input_pin = {
    .pin_bit_mask = (1 << ARM_PIN) | // we will ask you to do this part yourself in future weeks
                    (1 << DISARM_PIN) | // this is called a bitmask, it works by setting the bits corresponding 
                    (1 << CODE0_PIN) | // to the pins you want to apply these settings to equal to 1
                    (1 << CODE1_PIN) | // thus we use the left shift operator to shift a binary 1 into the bits
                    (1 << SETCODE_PIN), // specified by each of the pin macros, and we use a logical OR to combine them all
    .mode = //input mode,
    .pull_up_en = //pullup enable,
    .pull_down_en = //pulldown disable,
    .intr_type = //interrupt disable
};

// Output pin config
gpio_config_t output_pin = {
    .pin_bit_mask = (1 << RED_LED_PIN) |
                    (1 << GREEN_LED_PIN) |
                    (1 << BLUE_LED_PIN),
    .mode = //output mode,
    .pull_up_en = //pullup disable,
    .pull_down_en = //pulldown disable,
    .intr_type = //interrupt disable
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

void handle_arm_disarm(void) {
    // ARM
    if (/*alarm is armed*/ && /*not setting code*/ && /*arm pin level is low*/ && /*arm press debounce check is true*/) {
        //set alarm_armed to true
        //reset guess code to 0
        ESP_LOGI(TAG, "Alarm Armed");
    }

    // DISARM
    if (alarm_armed && !gpio_get_level(DISARM_PIN) && debounce_check(&previous_disarm_press)) {
        if (guess_code == code) {
            //set alarm_armed and alarm_triggered to false
            //reset guess code to 0
            ESP_LOGI(TAG, "Alarm Disarmed");
        } else {
            //set alarm triggered to true
            ESP_LOGI(TAG, "Alarm Triggered!");
        }
    }
}

void handle_set_code(void) {
    // SETCODE first press
    if (!alarm_armed && !setting_code && !gpio_get_level(SETCODE_PIN) && debounce_check(&previous_setcode_press)) {
        //set setting_code to true
        //reset temp code to 0
        ESP_LOGI(TAG, "Setting New Passcode");
    }

    // CODE0 press
    if (setting_code && !gpio_get_level(CODE0_PIN) && debounce_check(&previous_code0_press)) {
        //use the left shift operator to left shift temp_code by 1
        print_code(temp_code, "Temp Code");
    }

    // CODE1 press
    if (setting_code && !gpio_get_level(CODE1_PIN) && debounce_check(&previous_code1_press)) {
        //use the left shift operator to left shift temp_code by 1
        //but this time add in a logical OR with a 1 to set the rightmost bit equal to 1
        print_code(temp_code, "Temp Code");
    }

    // SETCODE second press (finish setting)
    if (setting_code && !gpio_get_level(SETCODE_PIN) && debounce_check(&previous_setcode_press)) {
        //set code equal to temp_code
        //set setting_code to false
        print_code(code, "New Code");
    }
}

void handle_guess_code(void) {
    if (!alarm_armed) return;

    // CODE0 press
    if (!gpio_get_level(CODE0_PIN) && debounce_check(&previous_code0_press)) {
        //use the left shift operator to left shift guess_code by 1
        print_code(guess_code, "Guess Code");
    }

    // CODE1 press
    if (!gpio_get_level(CODE1_PIN) && debounce_check(&previous_code1_press)) {
        //use the left shift operator to left shift guess_code by 1
        //but this time add in a logical OR with a 1 to set the rightmost bit equal to 1
        print_code(guess_code, "Guess Code");
    }
}

void update_leds(void) {
    // Armed/Disarmed LEDs
    
    // if the alarm isnt triggered and is armed
    // use gpio_set_level to turn the red led on and the green led off
    // if the alarm isnt triggered and isnt armed
    // use gpio_set_level to turn the red led off and the green led on

    // Triggered alarm flashing
    if (alarm_triggered && (esp_timer_get_time() - previous_alarm_beep > ALARM_PERIOD)) {
        alarm_flash = !alarm_flash;
        gpio_set_level(RED_LED_PIN, alarm_flash);
        previous_alarm_beep = esp_timer_get_time();
    }

    // Setting code LED
    gpio_set_level(BLUE_LED_PIN, setting_code ? 1 : 0);
}

// ------------------------ Main Application ------------------------

void app_main(void) {
    // Setup
    //call gpio_config() twice, once for the input_pin config and once for the output_pin config
    //make sure to pass the address of these configs as the argument to gpio_config()

    gpio_set_level(RED_LED_PIN, 0);
    gpio_set_level(BLUE_LED_PIN, 0);
    gpio_set_level(GREEN_LED_PIN, 0);

    vTaskDelay(pdMS_TO_TICKS(100));

    // Main loop
    while (1) {
        handle_arm_disarm();
        handle_set_code();
        handle_guess_code();
        update_leds();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}