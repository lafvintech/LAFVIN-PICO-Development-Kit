/**
 * @file lv_port_indev.h
 * @brief LVGL Input Device Driver Porting Layer Interface
 * @author NIGHT
 * @date 2025-10-27
 */

#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * @brief Initialize input device driver
 * @note Must be called before using LVGL touch functionality
 */
void lv_port_indev_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_INDEV_H*/
