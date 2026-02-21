# BEEP WEEK 4 README
 
This week is focused on methods for controlling an RGB LED, adjusting the brightness and color. Through another extension of the alarm system we'll demonstrate what's possible when utilizing internal hardware timers, interrupts, and pulse width modulation (PWM) controllers.  

![Board Setup](images/board.jpg)  

---
## 4.1 Content Overview

### RGB LED

![RGB LED](images/rgb.png)

RGB LEDs typically have 4 pins, one each for red, green, and blue and a common anode or cathode. The model we are using is common cathode, which means the color pins share a ground connection and must be connected to Vdd in order to light up. By varying the voltages across each pin with PWM you can mix the colors to create almost any combination.

### Hardware Timers (GPTimer)
The ESP32 model we are using's timer peripheral contains two general-purpose timer modules with two timers per module. All 4 are identical 64-bit, up/down configurable counters. While they are module pairs, they have independent configuration registers and interrupt generation. These timers base their count off of the APB_CLK source (80 MHz) and divide that with a 16-bit prescaler register to produce each 'tick' of the timer. The valid prescaler values range between 2 and 65536 which means the shortest tick granularity is `1 / (80,000,000 / 2) = 25ns` and the longest is `1 / (80,000,000 / 65536) = 0.8192ms`.   

After setting the tick speed, usually to some round number like 1us, we need to setup an alarm. Each timer has its own alarm which can be setup to trigger when the counter reaches a specified value. Usually alarms trigger an interrupt, which calls a user-defined callback where we can set flags, flip gpio pins, etc. These alarms can be configured in two modes: single shot, where they fire and then leave the counter running, and periodic, where the alarm fires and then resets the timer's counter in a loop until disabled. The alarm threshold and reset value are also configurable.

The ESP32 also has two system timers the **RTC TIMER** for low-power and sleep modes, and the **High Resolution Timer** which is used by the esp_timer software driver. The esp_timer driver allows us to measure the time since boot in us with `esp_timer_get_time()`, which is useful for things like software debouncing.

> Example setup for an alarm that fires every 100ms:  
> `Prescaler = 80` >> `1 / (80,000,000 / 80) = 1us`  
> `Alarm threshold = 100000` >> `1us * 100000 = 100ms`  
>  `Alarm reset value = 0`

### PWM

Pulse-width modulation is the process of rapidly oscillating a digital signal to produce a square wave, varying the *duty cycle* to alter the signal.

![PWM](images/pwm_diagram.png)

Duty cycle describes the percentage of the period that the voltage is high, and varying this percentage changes the average voltage across the pin. It is varied by changing the threshold value of a counter, which will toggle the pin on/off when the threshold is reached. In most configurations the pin also toggles when the counter reaches its max (or wrap) value and resets to zero (some PWM peripherals allow configurable count directions, reset values, and wrap behavior). The PWM peripheral can be used to control all kinds of devices that require varying voltage, such as LEDs and motors. Its also possible to produce rudimentary audio by feeding the PWM output through a low-pass filter.

### LED Control PWM (LEDC)

The ESP32 has two PWM peripherals, MCPWM for motor control and LEDC for LEDs. 

![ledc diagram](images/ledc.png)

The LEDC peripheral has two channels, high and low speed. The low speed channel applies duty cycle changes whenever told to by software, potentially causing glitchy output if the threshold value is changed during the middle of a period. The high speed channel uses register buffers to wait until the counter wraps before changing the threshold, and thus the duty cycle. This provides glitchless output regardless of software timing.  

Each speed channel has 4 timers and 8 channels, configured like so:

![ledc_detail](images/ledc_detail.png)
To setup PWM channels you first need to setup the base timer for its counter, similar to how you would setup a gptimer with tick resolution and frequency. After this, you have to setup the channel specifically to configure its interrupt (if needed), set the duty cycle, and connect it to a gpio pin.

## 4.2 Walkthrough

### 4.2.1 Wiring

![Board Setup](images/schematic.png)

### 4.2.2 Setup

Before you begin, please remember to create a new project:
1. Press ```ctrl+shift+p``` to open up the command panel
2. Look for ```ESP-IDF: Create New Empty Project```

![new-project](images/new-project.png)

3. Enter a folder name in the popup window

![popup](images/popup.png)

4. Select a location for the new folder (organize however you like!)

5. Replace the  ```main``` folder of your new project with the template main folder provided in this week's github folder.

![main-folder](images/main-folder.png)

### 4.2.3 Included Libraries
```C
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"  
#include "driver/ledc.h"
```
For peripheral structs and functions.

```C
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
```
For FreeRTOS task support and delay function to prevent timeout.

```C
#include "esp_timer.h"
```
For getting current time, used to debounce buttons.  

```C 
#include "esp_log.h"
```
For logging messages to serial monitor.

### 4.2.4 Global Variables

#### Constants
```C
#define GREEN_LED_PIN 27
#define RED_LED_PIN 14
#define BLUE_LED_PIN 12

#define ARM_PIN 18
#define DISARM_PIN 19
#define CODE0_PIN 21
#define CODE1_PIN 22
#define SETCODE_PIN 23
```
Pin macros

```C
#define DEBOUNCE_TIME 100000
#define DUTY_CYCLE_MAX 1000
```  
Maximum duty cycle is set to 1000 (even though it could go up to 1023) to make brightness math simpler. Debouncing time is measured in us, this is 100ms you may change it if needed.

#### RGB LED State
```C
int brightness = 0;
int step = 50;
```
We use global variables to store the current brightness and step increment for flashing the red alarm LED to allow them to persist across calls to update_leds.

#### Timer Handle
```C
gptimer_handle_t timer_handle;
``` 
Handle for the FreeRTOS timer object, not needed outside of the setup function, but should we to need to change/stop it, it becomes necessary. Good practice to make this a global or local within app_main (or whatever task is using it).

### 4.2.5 Coding

As seen in the schematic, the only hardware change this week was replacing the 3 separate LEDs with the single RGB LED. The activity goal hasn't changed, and as such most of the code from week 3 will be reused as is. The software changes this week take two forms, setup functions such as:
```C
void timer_setup(uint32_t res_hz, uint64_t alarm_hz)
void pwm_setup()
void rgb_strip_setup()
```
and the periodic function
```C
void update_leds(void)
``` 
With this update we will have completed the overhaul of the original alarm system code from week 2!

#### Function Descriptions

```C
void timer_setup(uint32_t res_hz, uint64_t alarm_hz)
```
This function does the following:
1. Configures the timer (resolution, clock source, etc.)
2. Configures the alarm (frequency, auto-reload behavior, etc.)
3. Sets a callback function for the alarm interrupt
4. Starts the timer

```C
void pwm_setup()
```
This function does the following:
1. Configures the PWM peripheral's base timer
2. Configures each of the 3 channels

```C
void rgb_strip_setup()
```
This function does the following:
1. Configures the RMT TX channel
2. Builds the bit encoder
3. Enables the RMT channel

```C
void update_leds()
```
This function does the following:
1. Basic color set conditions based on alarm state
2. Red alarm flashing check
3. Update all 3 channels

## 4.3 Bonus

### RMT
Typically used for IR communications like a TV remote, the [remote control transceiver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/rmt.html) encodes data in a similar manner to PWM by periodically alternating between digital low and high. The general setup steps are as follows:
1. Set a tick resolution (# of nanoseconds per tick)
2. Configure binary encoding by setting tick count for logic high and low for 0 and 1.
```C
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
```
3. Set MSB or LSB first and choose gpio pin.

This peripheral can also be used to control RGB LED strips such as the WS2812B. For a strip of 8 LEDs, an array of 24 8-bit integers is required, 1 byte per color per LED. 
```C
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
```
This pattern walks back and forth over the 8 LEDs, one color at a time. Experiment with the increment choices and color mixing to see what other patterns you can create!

## 4.4 Helpful Links

#### Documentation
* [ESP32 WROOM 32E Pinout](https://docs.sunfounder.com/projects/umsk/en/latest/07_appendix/esp32_wroom_32e.html)
* [ESP32 Technical Reference Manual](https://documentation.espressif.com/esp32_technical_reference_manual_en.pdf#iomuxgpio)
* [ESP-IDF Docs](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/index.html)

#### Environment Setup
* [IDF Frontend (if you're curious)](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-py.html)
* [Dev Container Setup](https://docs.espressif.com/projects/vscode-esp-idf-extension/en/latest/additionalfeatures/docker-container.html)
* [WSL](https://learn.microsoft.com/en-us/windows/wsl/basic-commands)
* [USBIPD](https://github.com/dorssel/usbipd-win)