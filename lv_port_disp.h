/**
 * @file lv_port_disp.h
 * @brief LVGL Display Driver Porting Layer for ST7796
 * @author NIGHT
 * @date 2025-10-27
 */
#if 1

#ifndef LV_PORT_DISP_TEMPL_H
#define LV_PORT_DISP_TEMPL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#if defined(LV_LVGL_H_INCLUDE_SIMPLE)
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * @brief Initialize display driver
 * @note Must be called before using LVGL
 */
void lv_port_disp_init(void);

/**
 * @brief Enable screen refresh
 */
void disp_enable_update(void);

/**
 * @brief Disable screen refresh
 */
void disp_disable_update(void);

/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_PORT_DISP_TEMPL_H*/

#endif /*Disable/Enable content*/
