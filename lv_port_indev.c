/**
 * @file lv_port_indev.c
 * @brief LVGL Input Device Driver Porting Layer
 * @note Calls gt911.c hardware driver for touch functionality
 * @author NIGHT
 * @date 2025-10-27
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"
#include "lvgl.h"
#include "gt911.h"

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void touchpad_init(void);
static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t *indev_touchpad;

/* Store last touch coordinates */
static int16_t last_x = 0;
static int16_t last_y = 0;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Initialize LVGL input device driver
 * @note Currently only touchpad (GT911) is implemented
 */
void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv;

    /* Initialize and register touchpad */
    touchpad_init();
    
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief Initialize GT911 touch driver
 */
static void touchpad_init(void)
{
    if (!gt911_init()) {
        // TODO: Add error handling if needed
    }
}

/**
 * @brief Read touch data and update LVGL input state
 * @param indev_drv Input device driver pointer
 * @param data Output data structure for LVGL
 * @note Called periodically by LVGL to poll touch state
 */
static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    uint16_t x, y;
    bool pressed;
    
    data->continue_reading = false;
    
    if (gt911_read_touch(&x, &y, &pressed)) {
        if (pressed) {
            // Touch detected: update coordinates and state
            data->point.x = x;
            data->point.y = y;
            data->state = LV_INDEV_STATE_PR;
            
            last_x = x;
            last_y = y;
        } else {
            // No touch: return last coordinates with released state
            data->point.x = last_x;
            data->point.y = last_y;
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        // Read failed: return last coordinates with released state
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
    }
}