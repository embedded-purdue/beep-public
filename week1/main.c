#include "driver/gpio.h"
#include "rom/ets_sys.h"

#define LED_PIN 27
#define BUTTON_PIN 26

void app_main(void) {
  // Configure LED pin
  gpio_reset_pin(LED_PIN);
  gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

  // Configure Button pin
  gpio_reset_pin(BUTTON_PIN);
  gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);

  int led_state = 0;
  while (1) {
    int button_state = gpio_get_level(BUTTON_PIN);

    // Button is active low (pulled up)
    if (button_state == 0) {
      led_state = !led_state;
      gpio_set_level(LED_PIN, led_state);
      ets_delay_us(1000000);
    } else {
      // Hold state (do nothing)
      ets_delay_us(10000);
    }
  }
}