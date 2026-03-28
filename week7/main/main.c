#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h" 
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "helpers.h"

QueueHandle_t distance_queue;

void ultrasonic_task(void *pvParameters) {
    while(1) {
        // trigger a pulse by setting the trigger pin low, waiting 2 us,
        // setting it high, waiting 10 us, and then setting it low again
        // use gpio_set_level and ets_delay_us
        
        // wait for the echo pin to go high
        // use a while loop and gpio_get_level, if the echo pin's level is low keep looping
        // only exit when echo is high
        // upon exiting the loop save the current time to a variable
        // with esp_timer_get_time

        // wait for the echo pin to go low
        // use a while loop and gpio_get_level, if the echo pin's level is high keep looping
        // only exit when echo is low
        // upon exiting the loop save the current time to a variable
        // with esp_timer_get_time

        //calculate and print distance
        float distance_cm = (/*your start time variable*/ - /*your end time variable*/) * 0.0343 / 2.0;

        //use xQueueSend() with the distance_queue, distance_cm, and 0

        //yield the cpu
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void lcd_display_task(void *pvParameters) {
    float received_distance;
    char buffer[16];

    while(1) {
        //check if the value returned by xQueueReceive with the distance queue, address of received_distance,
        //and portMAX_DELAY as the arguments is equal to pdPASS
        if () {
            snprintf(buffer, sizeof(buffer), "Dist: %.2f cm", received_distance);
            
            //printing to lcd
            lcd_set_cursor(0);
            lcd_print(buffer);   
        }
    }
}

void app_main(void) {
    setup_lcd();
    setup_ultrasonic();
  
    //initialize the distance queue with xQueueCreate
    //with a length of 5 and size of a float (hint: use sizeof())
    distance_queue = 

    //call xTaskCreate for each of the two tasks, the priority for the lcd_display task should 
    //be 6 and the ultrasonic task 5
    xTaskCreate(/**/, /**/, 2048, NULL, /**/, NULL);
    xTaskCreate(/**/, /**/, 2048, NULL, /**/, NULL);
}