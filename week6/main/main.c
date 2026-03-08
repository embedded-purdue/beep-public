#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "helpers.h"

const char* TAG = "MAIN";

//Game State
char row_top[17] =    "                ";
char row_bottom[17] = "                ";
int ship_y = 0;
int score = 0;
bool game_over = false;

void app_main(void) {
    //call setup_lcd and setup_joystick
    //call timer_setup with 1000000 and 200000 as the arguments (5Hz game ticks, you can speed this up by lowering the alarm_hz and slow it down by raising it)
    

    int adc_x = 2048;
    int adc_y = 2048;

    float smoothed_y = 2048.0f;
    float alpha = 0.3f;        

    while (1) {
        //joystick reads
        //use adc_oneshot_read() with the adc handle, adc channels that match the VRX and VRY pins, and the adc_x and adc_y variables
        //to read in both axes of the joystick

        //exponential moving average time
        //set smoothed_y equal to alpha times adc_y plus 1-alpha times the value already stored in smoothed_y

        //calculate new ship location
        // if smoothed_y is > 3200 set ship_y to 1
        // if smoothed_y is < 800 set ship_y to 0

        //button press check
        if (game_over && gpio_get_level(SW_PIN) == 0) {\
            //reset game state
            game_over = false;
            score = 0;

            //clear screen
            snprintf(row_top, 17, "                ");
            snprintf(row_bottom, 17, "                ");

            lcd_set_cursor(false);
            lcd_print(row_top);
            lcd_set_cursor(true);
            lcd_print(row_bottom);
        }

        //game ticks
        if (timer_triggered) {
            timer_triggered = false; 

            if (!game_over) {
                //shift the rocks
                for (int i = 0; i < 15; i++) {
                    row_top[i] = row_top[i+1];
                    row_bottom[i] = row_bottom[i+1];
                }
                
                //new rock spawning
                row_top[15] = ' ';
                row_bottom[15] = ' ';
                
                static int rock_cooldown = 1; 
                if (rock_cooldown > 0) {
                    rock_cooldown--;
                } else {
                    //70% chance to spawn a rock when the cooldown is 0
                    if (rand() % 10 < 7) { 
                        if (rand() % 2 == 0) {
                            row_top[15] = '*';
                        } else {
                            row_bottom[15] = '*';
                        }
                        //reset cooldown
                        rock_cooldown = 1; 
                    }
                }

                //detect collisions with rocks
                if ((ship_y == 0 && row_top[1] == '*') || 
                    (ship_y == 1 && row_bottom[1] == '*')) {
                    game_over = true;
                } else {
                    score++;
                }

                //update ship location
                char temp_top = row_top[1];
                char temp_bottom = row_bottom[1];
                row_top[1] = (ship_y == 0) ? '>' : row_top[1];
                row_bottom[1] = (ship_y == 1) ? '>' : row_bottom[1];

                //update screen
                lcd_set_cursor(false);
                lcd_print(row_top);
                lcd_set_cursor(true);
                lcd_print(row_bottom);

                row_top[1] = temp_top;
                row_bottom[1] = temp_bottom;
            } 
            else { //game over screen
                lcd_set_cursor(false);
                lcd_print("    CRASHED!    ");
                
                char score_str[17];
                snprintf(score_str, sizeof(score_str), "Score: %d", score);
                lcd_set_cursor(true);
                lcd_print(score_str);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}