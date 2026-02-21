#include <stdio.h>
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/rmt_tx.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "helpers.h"

//Timer Handle
gptimer_handle_t timer_handle;

//Timer ISR Flag
volatile bool timer_triggered = false;

//BONUS: RMT Config
rmt_channel_handle_t rmt_handle;
rmt_encoder_handle_t encoder_handle;
rmt_transmit_config_t rmt_tx_config = {
    .loop_count = 0,
    .flags.eot_level = 0,
};

// ----------------- Timer ISR -----------------

static bool timer_handler(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    timer_triggered = true;
    return false;
}

// ----------------- Setup Functions -----------------
void pwm_setup() {
    ledc_timer_config_t timer_config = {
        .speed_mode = //high speed mode
        .duty_resolution = // bit number is X where 2^X = 1024
        .timer_num = //timer 0
        .freq_hz = //10KHz
        .clk_cfg = //auto clock
    };
    // Setup individual channels
    // use the ledc_channel_config_t type once for each LED color
    // and initialize the gpio_num to whichever XXX_LED_PIN macro,
    // speed_mode to high speed mode, give each a unique channel (use 0-2)
    // disable interrupts, select timer 0, and set the duty and hpoint equal to 0

    //call ledc_timer_config with the above struct
    //call ledc_channel config 3 times, one for each of the channel structs above
}


void timer_setup(uint32_t res_hz, uint64_t alarm_hz) {
    gptimer_config_t timer_config = {
        .clk_src = //select APB as the source
        .direction = //count up
        .resolution_hz = //resolution argument to this function
        .intr_priority = 0
    };
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = //alarm argument to this function
        .reload_count = //reset back to 0 
        .flags.auto_reload_on_alarm = //you should auto reload on alarm
    };
    gptimer_event_callbacks_t cbs = {
        .on_alarm = //the timer ISR listed above
    };

    //add a new timer with gptimer_new_timer(&config, &handle), use the config and handle structs we've defined here
    //set the alarm action with gptimer_set_alarm_action(handle, &config), use the structs we've defined here
    //register the callback function with gptimer_register_event_callbacks(handle, &callback struct, NULL)
    //enable the timer with gptimer_enable
    //start the timer with gptimer_start
}

void rgb_strip_setup() {
    rmt_tx_channel_config_t rgb_strip_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, 
        .gpio_num = RGB_STRIP_PIN,
        .mem_block_symbols = 64,
        .resolution_hz = STRIP_TICK_HZ,
        .trans_queue_depth = 4,
    };
    rmt_symbol_word_t bit0 = {
        .duration0 = T0L_TICKS,
        .level0 = 0,
        .duration1 = T0H_TICKS,
        .level1 = 1
    };
    rmt_symbol_word_t bit1 = {
        .duration0 = T1L_TICKS,
        .level0 = 0,
        .duration1 = T1H_TICKS,
        .level1 = 1
    };
    rmt_bytes_encoder_config_t encoder_config = {
        .bit0 = bit0,
        .bit1 = bit1,
        .flags.msb_first = 1
    };

    rmt_new_tx_channel(&rgb_strip_config,&rmt_handle);
    rmt_enable(rmt_handle);
    rmt_new_bytes_encoder(&encoder_config,&encoder_handle);
}