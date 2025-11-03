#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/mutex.h"
#include "pico/sem.h"

#include "FreeRTOS.h" /* Must come first. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"

#include "ws2812.pio.h"

// ========================================
// GPIO Pin Definitions
// ========================================
#define GPIO_WS2812         12  // WS2812 RGB LED data pin
#define GPIO_BUZZER         13  // Buzzer output
#define GPIO_BUTTON_1       15  // Button 1 input
#define GPIO_BUTTON_2       14  // Button 2 input
#define GPIO_BUTTON_RESET   22  // Reset button input
#define GPIO_LED_1          16  // LED 1 output
#define GPIO_LED_2          17  // LED 2 output
#define GPIO_ADC_X          26  // Joystick X-axis ADC
#define GPIO_ADC_Y          27  // Joystick Y-axis ADC

// LVGL Mutex - Ensures thread safety (required by LVGL official documentation)
SemaphoreHandle_t lvgl_mutex = NULL;

void vApplicationTickHook(void)
{
    lv_tick_inc(1);
}

lv_obj_t *splash_image = NULL;  // Startup splash image

lv_obj_t *led1 = NULL;
lv_obj_t *led2 = NULL;

lv_obj_t *joystick_circle = NULL;  // Joystick outer circle
lv_obj_t *joystick_ball = NULL;    // Joystick inner ball

bool joystick_enabled = false;     // Joystick ADC enable flag

// WS2812 RGB LED configuration
static PIO rgb_pio = NULL;
static uint rgb_sm = 0;

// Buzzer state
static bool buzzer_state = false;

// Button debounce tracking
static uint32_t btn1_last_time = 0;
static uint32_t btn2_last_time = 0;
#define BTN_DEBOUNCE_MS 50

// Joystick ADC configuration
#define ADC_MAX_VALUE       4095        // 12-bit ADC max value
#define ADC_CENTER          2048        // ADC center position
#define ADC_DEADZONE        150         // Deadzone threshold (prevents drift)

// Forward function declarations
static void reboot_handler(lv_event_t *e);
void on_button_interrupt(uint gpio_pin, uint32_t events);

// Calculator related variables
lv_obj_t *calc_display = NULL;
char calc_buffer[32] = "0";
double calc_num1 = 0;
double calc_num2 = 0;
char calc_operator = 0;
uint8_t calc_new_number = 1;

/**
 * @brief Format calculator result (remove trailing zeros and decimal point)
 */
static void format_calc_number(void)
{
    snprintf(calc_buffer, 32, "%.2f", calc_num1);
    
    // Remove trailing zeros
    char *p = calc_buffer + strlen(calc_buffer) - 1;
    while (*p == '0' && p > calc_buffer) {
        *p-- = '\0';
    }
    
    // Remove decimal point if no fractional part
    if (*p == '.') {
        *p = '\0';
    }
}

// Calculator button event handler
static void calc_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        lv_obj_t *btn = lv_event_get_target(e);
        const char *txt = lv_label_get_text(lv_obj_get_child(btn, 0));
        
        if (txt[0] >= '0' && txt[0] <= '9') {
            // Number button
            if (calc_new_number) {
                calc_buffer[0] = txt[0];
                calc_buffer[1] = '\0';
                calc_new_number = 0;
            } else {
                if (strlen(calc_buffer) < 15) {
                    strcat(calc_buffer, txt);
                }
            }
        } else if (txt[0] == '.') {
            // Decimal point
            if (strchr(calc_buffer, '.') == NULL && strlen(calc_buffer) < 15) {
                strcat(calc_buffer, ".");
            }
        } else if (txt[0] == 'C') {
            // Clear
            strcpy(calc_buffer, "0");
            calc_num1 = 0;
            calc_num2 = 0;
            calc_operator = 0;
            calc_new_number = 1;
        } else if (txt[0] == '=') {
            // Equals
            if (calc_operator) {
                calc_num2 = atof(calc_buffer);
                switch (calc_operator) {
                    case '+': calc_num1 = calc_num1 + calc_num2; break;
                    case '-': calc_num1 = calc_num1 - calc_num2; break;
                    case '*': calc_num1 = calc_num1 * calc_num2; break;
                    case '/': if (calc_num2 != 0) calc_num1 = calc_num1 / calc_num2; break;
                }
                format_calc_number();
                calc_operator = 0;
                calc_new_number = 1;
            }
        } else {
            // Operator
            if (calc_operator && !calc_new_number) {
                calc_num2 = atof(calc_buffer);
                switch (calc_operator) {
                    case '+': calc_num1 = calc_num1 + calc_num2; break;
                    case '-': calc_num1 = calc_num1 - calc_num2; break;
                    case '*': calc_num1 = calc_num1 * calc_num2; break;
                    case '/': if (calc_num2 != 0) calc_num1 = calc_num1 / calc_num2; break;
                }
                format_calc_number();
            } else {
                calc_num1 = atof(calc_buffer);
            }
            calc_operator = txt[0];
            calc_new_number = 1;
        }
        
        lv_label_set_text(calc_display, calc_buffer);
    }
}

static void calculator_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {  // Prevent duplicate deletion of splash_image
        if (splash_image != NULL){
            lv_obj_del(splash_image);
            splash_image = NULL;
        }
        lv_obj_clean(lv_scr_act());
        // vTaskDelay(100 / portTICK_PERIOD_MS);
        
        // Create display screen
        calc_display = lv_label_create(lv_scr_act());
        lv_label_set_text(calc_display, "0");
        lv_obj_set_style_text_font(calc_display, &lv_font_montserrat_16, 0);  // Use 16pt font (enabled in config)
        //lv_obj_set_style_text_color(calc_display, lv_color_white(), 0);  // White text
        lv_obj_set_style_text_align(calc_display, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_width(calc_display, 300);
        lv_obj_align(calc_display, LV_ALIGN_TOP_MID, 0, 20);
        
        // Button layout: 4x4 grid + bottom equals button
        const char *btnm_map[] = {
            "7", "8", "9", "/",
            "4", "5", "6", "*",
            "1", "2", "3", "-",
            "C", "0", ".", "+"
        };
        
        int btn_w = 70;
        int btn_h = 60;
        int start_x = 10;
        int start_y = 80;
        int gap = 10;
        
        // Create 4x4 button grid
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                int idx = row * 4 + col;
                
                lv_obj_t *btn = lv_btn_create(lv_scr_act());
                lv_obj_set_size(btn, btn_w, btn_h);
                lv_obj_set_pos(btn, start_x + col * (btn_w + gap), start_y + row * (btn_h + gap));
                lv_obj_add_event_cb(btn, calc_btn_event_handler, LV_EVENT_ALL, NULL);
                
                lv_obj_t *label = lv_label_create(btn);
                lv_label_set_text(label, btnm_map[idx]);
                lv_obj_center(label);
                
                // Number buttons: white background + black text
                if (btnm_map[idx][0] >= '0' && btnm_map[idx][0] <= '9') {
                    lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
                    lv_obj_set_style_text_color(label, lv_color_black(), 0);
                } 
                // Decimal point: white background + black text
                else if (btnm_map[idx][0] == '.') {
                    lv_obj_set_style_bg_color(btn, lv_color_white(), 0);
                    lv_obj_set_style_text_color(label, lv_color_black(), 0);
                } 
                // Operators and clear: black background + white text
                else {
                    lv_obj_set_style_bg_color(btn, lv_color_black(), 0);
                    lv_obj_set_style_text_color(label, lv_color_white(), 0);
                }
            }
        }
        
        // Equals button spans full width: blue background + white text
        lv_obj_t *btn_eq = lv_btn_create(lv_scr_act());
        lv_obj_set_size(btn_eq, btn_w * 4 + gap * 3, btn_h);
        lv_obj_set_pos(btn_eq, start_x, start_y + 4 * (btn_h + gap));
        lv_obj_add_event_cb(btn_eq, calc_btn_event_handler, LV_EVENT_ALL, NULL);
        lv_obj_set_style_bg_color(btn_eq, lv_color_make(0, 120, 215), 0);  // Blue background
        
        lv_obj_t *label_eq = lv_label_create(btn_eq);
        lv_label_set_text(label_eq, "=");
        lv_obj_center(label_eq);
        lv_obj_set_style_text_color(label_eq, lv_color_white(), 0);  // White text
        
        // Reset button - bottom, red background + white text
        lv_obj_t *calc_reboot_btn = lv_btn_create(lv_scr_act());
        lv_obj_set_size(calc_reboot_btn, btn_w * 4 + gap * 3, btn_h);
        lv_obj_set_pos(calc_reboot_btn, start_x, start_y + 5 * (btn_h + gap));
        lv_obj_add_event_cb(calc_reboot_btn, reboot_handler, LV_EVENT_ALL, NULL);
        lv_obj_set_style_bg_color(calc_reboot_btn, lv_color_make(220, 53, 69), 0);  // Red background
        
        lv_obj_t *calc_reboot_label = lv_label_create(calc_reboot_btn);
        lv_label_set_text(calc_reboot_label, "RESET");
        lv_obj_center(calc_reboot_label);
        lv_obj_set_style_text_color(calc_reboot_label, lv_color_white(), 0);  // White text
    }
}

static void reboot_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        // Force reboot using Pico SDK watchdog
        // Enable watchdog with 1ms timeout
        watchdog_enable(1, false);
        
        // Wait for watchdog to trigger reboot (after ~1ms)
        while(1) {
            tight_loop_contents();
        }
    }
}

/**
 * @brief Buzzer toggle handler
 */
static void on_buzzer_toggle(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    
    buzzer_state = !buzzer_state;
    gpio_put(GPIO_BUZZER, buzzer_state);
}

/**
 * @brief Write RGB color to WS2812 LED
 */
static void ws2812_write(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t grb = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
    pio_sm_put_blocking(rgb_pio, rgb_sm, grb << 8u);
}

/**
 * @brief Convert LVGL color to RGB values
 */
static void lvgl_color_to_rgb(lv_color_t color, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = (color.ch.red * 255) / 31;
    uint8_t green_full = (color.ch.green_h << 3) | color.ch.green_l;
    *g = (green_full * 255) / 63;
    *b = (color.ch.blue * 255) / 31;
}

/**
 * @brief Colorwheel change event handler
 */
static void on_colorwheel_changed(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) return;
    
    lv_color_t color = lv_colorwheel_get_rgb(lv_event_get_target(e));
    uint8_t r, g, b;
    lvgl_color_to_rgb(color, &r, &g, &b);
    ws2812_write(r, g, b);
}

/**
 * @brief RGB LED off button handler
 */
static void on_rgb_off_clicked(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    ws2812_write(0, 0, 0);
}

/**
 * @brief Map ADC value to joystick position with deadzone
 * @param adc_raw Raw ADC reading (0-4095)
 * @param max_pos Maximum position value (0-88)
 * @param invert true to invert axis direction
 * @return Mapped position value
 */
static int map_adc_with_deadzone(uint adc_raw, int max_pos, bool invert)
{
    int offset = (int)adc_raw - ADC_CENTER;
    
    // Apply deadzone - if within threshold, return center position
    if (abs(offset) < ADC_DEADZONE) {
        return max_pos / 2;  // Center position
    }
    
    // Map outside deadzone to full range
    int mapped;
    if (offset > 0) {
        // Upper half: map (DEADZONE to ADC_MAX_VALUE-CENTER) -> (center to max_pos)
        int range = ADC_CENTER - ADC_DEADZONE;
        int scaled = ((offset - ADC_DEADZONE) * (max_pos / 2)) / range;
        mapped = (max_pos / 2) + scaled;
    } else {
        // Lower half: map (-CENTER+DEADZONE to -DEADZONE) -> (0 to center)
        int range = ADC_CENTER - ADC_DEADZONE;
        int scaled = ((offset + ADC_DEADZONE) * (max_pos / 2)) / range;
        mapped = (max_pos / 2) + scaled;
    }
    
    // Clamp to valid range
    if (mapped < 0) mapped = 0;
    if (mapped > max_pos) mapped = max_pos;
    
    return invert ? (max_pos - mapped) : mapped;
}

/**
 * @brief Initialize all hardware peripherals
 */
static void init_hardware_peripherals(void)
{
    // Buzzer GPIO setup
    gpio_init(GPIO_BUZZER);
    gpio_set_dir(GPIO_BUZZER, GPIO_OUT);
    gpio_put(GPIO_BUZZER, 0);
    
    // WS2812 RGB LED via PIO
    rgb_pio = pio0;
    rgb_sm = pio_claim_unused_sm(rgb_pio, true);
    uint ws2812_offset = pio_add_program(rgb_pio, &ws2812_program);
    ws2812_program_init(rgb_pio, rgb_sm, ws2812_offset, GPIO_WS2812, 800000, true);
    ws2812_write(0, 0, 0);  // Turn off LED initially
    
    // Button interrupts with debouncing
    gpio_set_irq_enabled_with_callback(GPIO_BUTTON_1, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &on_button_interrupt);
    gpio_set_irq_enabled_with_callback(GPIO_BUTTON_2, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &on_button_interrupt);
    gpio_set_irq_enabled_with_callback(GPIO_BUTTON_RESET, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &on_button_interrupt);
    
    // Physical LED GPIOs
    gpio_init(GPIO_LED_1);
    gpio_init(GPIO_LED_2);
    gpio_set_dir(GPIO_LED_1, GPIO_OUT);
    gpio_set_dir(GPIO_LED_2, GPIO_OUT);
    gpio_put(GPIO_LED_1, 0);
    gpio_put(GPIO_LED_2, 0);
    
    // Enable joystick ADC reading
    joystick_enabled = true;
}

/**
 * @brief Create hardware demo UI elements
 */
static void create_hardware_ui(void)
{
    // Top-left RESET button (red)
    lv_obj_t *reset_btn = lv_btn_create(lv_scr_act());
    lv_obj_set_size(reset_btn, 80, 35);
    lv_obj_align(reset_btn, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_event_cb(reset_btn, reboot_handler, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(reset_btn, lv_color_make(220, 53, 69), 0);
    
    lv_obj_t *reset_label = lv_label_create(reset_btn);
    lv_label_set_text(reset_label, "RESET");
    lv_obj_center(reset_label);
    lv_obj_set_style_text_color(reset_label, lv_color_white(), 0);
    
    // Buzzer toggle button
    lv_obj_t *buzzer_toggle = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(buzzer_toggle, on_buzzer_toggle, LV_EVENT_ALL, NULL);
    lv_obj_align(buzzer_toggle, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_flag(buzzer_toggle, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(buzzer_toggle, LV_SIZE_CONTENT);
    
    lv_obj_t *buzzer_label = lv_label_create(buzzer_toggle);
    lv_label_set_text(buzzer_label, "Buzzer");
    lv_obj_center(buzzer_label);
    
    // RGB LED off button
    lv_obj_t *rgb_clear_btn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(rgb_clear_btn, on_rgb_off_clicked, LV_EVENT_ALL, NULL);
    lv_obj_align(rgb_clear_btn, LV_ALIGN_TOP_MID, 0, 80);
    
    lv_obj_t *rgb_clear_label = lv_label_create(rgb_clear_btn);
    lv_label_set_text(rgb_clear_label, "RGB LED Off");
    lv_obj_center(rgb_clear_label);
    
    // Color picker wheel
    lv_obj_t *color_picker = lv_colorwheel_create(lv_scr_act(), true);
    lv_obj_set_size(color_picker, 200, 200);
    lv_obj_center(color_picker);
    lv_obj_add_event_cb(color_picker, on_colorwheel_changed, LV_EVENT_VALUE_CHANGED, NULL);
    
    // LED status indicators
    led1 = lv_led_create(lv_scr_act());
    lv_obj_align(led1, LV_ALIGN_TOP_MID, -30, 400);
    lv_led_set_color(led1, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_off(led1);
    
    led2 = lv_led_create(lv_scr_act());
    lv_obj_align(led2, LV_ALIGN_TOP_MID, 30, 400);
    lv_led_set_color(led2, lv_palette_main(LV_PALETTE_BLUE));
    lv_led_off(led2);
    
    // Joystick visualization container
    joystick_circle = lv_obj_create(lv_scr_act());
    lv_obj_set_size(joystick_circle, 100, 100);
    lv_obj_align(joystick_circle, LV_ALIGN_TOP_MID, 0, 190);
    lv_obj_set_style_bg_color(joystick_circle, lv_color_white(), 0);
    lv_obj_set_style_border_color(joystick_circle, lv_color_black(), 0);
    lv_obj_set_style_border_width(joystick_circle, 2, 0);
    lv_obj_set_style_radius(joystick_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(joystick_circle, 0, 0);
    lv_obj_clear_flag(joystick_circle, LV_OBJ_FLAG_SCROLLABLE);
    
    // Joystick position indicator
    joystick_ball = lv_obj_create(joystick_circle);
    lv_obj_set_size(joystick_ball, 12, 12);
    lv_obj_set_pos(joystick_ball, 44, 44);  // Center: (100-12)/2 = 44
    lv_obj_set_style_bg_color(joystick_ball, lv_color_make(0, 0, 255), 0);
    lv_obj_set_style_border_width(joystick_ball, 0, 0);
    lv_obj_set_style_radius(joystick_ball, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(joystick_ball, 0, 0);
    
    // Instruction label
    lv_obj_t *instruction_label = lv_label_create(lv_scr_act());
    lv_label_set_text(instruction_label, "Press Buttons to Control LEDs");
    lv_obj_set_style_text_align(instruction_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(instruction_label, LV_ALIGN_TOP_MID, 0, 380);
}

/**
 * @brief Button interrupt handler with debouncing
 */
void on_button_interrupt(uint gpio_pin, uint32_t events)
{
    // Only handle button press (rising edge)
    if (!(events & GPIO_IRQ_EDGE_RISE)) return;
    
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    if (gpio_pin == GPIO_BUTTON_1) {
        // Debounce check for button 1
        if (now - btn1_last_time > BTN_DEBOUNCE_MS) {
            btn1_last_time = now;
            lv_led_toggle(led1);
            gpio_put(GPIO_LED_1, !gpio_get(GPIO_LED_1));  // Toggle LED GPIO
        }
    } 
    else if (gpio_pin == GPIO_BUTTON_2) {
        // Debounce check for button 2
        if (now - btn2_last_time > BTN_DEBOUNCE_MS) {
            btn2_last_time = now;
            lv_led_toggle(led2);
            gpio_put(GPIO_LED_2, !gpio_get(GPIO_LED_2));  // Toggle LED GPIO
        }
    }
}

static void hw_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    // Clean up splash screen
    if (splash_image != NULL) {
        lv_obj_del(splash_image);
        splash_image = NULL;
    }
    lv_obj_clean(lv_scr_act());
    
    // Initialize hardware peripherals
    init_hardware_peripherals();
    
    // Create UI elements
    create_hardware_ui();
}

void lv_example_btn_1(void)
{
    lv_obj_t *label;

    // Hardware Demo button
    lv_obj_t *hw_btn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(hw_btn, hw_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(hw_btn, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_bg_color(hw_btn, lv_color_white(), 0);  // White background
    
    label = lv_label_create(hw_btn);
    lv_label_set_text(label, "Hardware Demo");
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);  // Black text
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);  // Larger font (default is 14)
    lv_obj_set_style_text_letter_space(label, 1, 0);  // Letter spacing for bolder look

    // Calculator button
    lv_obj_t *calc_btn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(calc_btn, calculator_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(calc_btn, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_bg_color(calc_btn, lv_color_white(), 0);  // White background

    label = lv_label_create(calc_btn);
    lv_label_set_text(label, "Calculator");
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);  // Black text
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);  // Larger font (default is 14)
    lv_obj_set_style_text_letter_space(label, 1, 0);  // Letter spacing for bolder look
}

void task0(void *pvParam)
{
    // Lock mutex when initializing UI
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    lv_obj_clean(lv_scr_act());
    xSemaphoreGive(lvgl_mutex);
    
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Lock mutex when creating initial UI
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    splash_image = lv_img_create(lv_scr_act());
    LV_IMG_DECLARE(sea);
    lv_img_set_src(splash_image, &sea);
    lv_obj_align(splash_image, LV_ALIGN_DEFAULT, 0, 0);
    lv_example_btn_1();
    xSemaphoreGive(lvgl_mutex);

    for (;;)
    {
        if (joystick_enabled)
        {
            adc_init();
            // Make sure GPIO is high-impedance, no pullups etc
            adc_gpio_init(GPIO_ADC_X);
            adc_gpio_init(GPIO_ADC_Y);

            for (;;)
            {
                char buf[50];

                // ADC read doesn't need mutex lock
                adc_select_input(0);
                uint adc_x_raw = adc_read();
                adc_select_input(1);
                uint adc_y_raw = adc_read();

                // Map ADC values with deadzone handling
                const int max_pos = 88;  // 100-12=88 (outer frame 100, ball 12)
                int ball_x = map_adc_with_deadzone(adc_x_raw, max_pos, false);
                int ball_y = map_adc_with_deadzone(adc_y_raw, max_pos, true);  // Y-axis inverted

                // Lock mutex when updating LVGL objects
                xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
                lv_obj_set_pos(joystick_ball, ball_x, ball_y);
                xSemaphoreGive(lvgl_mutex);

                vTaskDelay(200 / portTICK_PERIOD_MS);
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void task1(void *pvParam)
{
    for (;;)
    {
        // Must lock mutex before/after lv_task_handler (LVGL official requirement)
        xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
        lv_task_handler();
        xSemaphoreGive(lvgl_mutex);
        
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

int main()
{
    stdio_init_all();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();

    // Create LVGL mutex (must be created before task startup)
    lvgl_mutex = xSemaphoreCreateMutex();
    if (lvgl_mutex == NULL) {
        // Mutex creation failed, system cannot run
        while(1) {
            tight_loop_contents();
        }
    }

    UBaseType_t task0_CoreAffinityMask = (1 << 0);
    UBaseType_t task1_CoreAffinityMask = (1 << 1);

    TaskHandle_t task0_Handle = NULL;

    xTaskCreate(task0, "task0", 2048, NULL, 1, &task0_Handle);
    vTaskCoreAffinitySet(task0_Handle, task0_CoreAffinityMask);

    TaskHandle_t task1_Handle = NULL;
    xTaskCreate(task1, "task1", 2048, NULL, 2, &task1_Handle);
    vTaskCoreAffinitySet(task1_Handle, task1_CoreAffinityMask);

    vTaskStartScheduler();

    return 0;
}