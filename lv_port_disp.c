/**
 * @file lv_port_disp.c
 * @brief LVGL Display Driver Porting Layer
 * @note Calls st7796.c hardware driver for display functionality
 * @author NIGHT
 * @date 2025-10-27
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include "st7796.h"
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES    320
#define MY_DISP_VER_RES    480

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

/**********************
 *  STATIC VARIABLES
 **********************/
/* Display flush enable/disable flag */
static volatile bool disp_flush_enabled = true;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Initialize LVGL display driver
 */
void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize display hardware
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create LVGL draw buffer
     *----------------------------*/

    /**
     * LVGL requires a buffer for internal widget drawing.
     * This buffer will be passed to the display driver's `flush_cb` to copy content to the display.
     * Buffer must be larger than 1 display row.
     *
     * There are 3 buffer configurations:
     * 1. Single buffer:
     *    LVGL will draw content here and write to display
     *
     * 2. Double buffer:
     *    LVGL draws content to buffer and writes to display.
     *    Should use DMA to transfer buffer content to display.
     *    This allows LVGL to draw to second buffer while transferring first buffer (parallel operation).
     *
     * 3. Full-screen double buffer:
     *    Set 2 screen-sized buffers and set disp_drv.full_refresh = 1.
     *    LVGL will always provide complete rendered screen in `flush_cb`, only need to change framebuffer address.
     */

    /* Example 1: Single buffer configuration (saves memory) */
    static lv_disp_draw_buf_t draw_buf_dsc_1;
    static lv_color_t buf_1[MY_DISP_HOR_RES * 10];  // 10-row buffer
    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 10);

    /* Example 2: Double buffer configuration (better performance, but requires more memory)
    static lv_disp_draw_buf_t draw_buf_dsc_2;
    static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];  // First buffer
    static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];  // Second buffer
    lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 10);
    */

    /*-----------------------------------
     * Register display driver in LVGL
     *----------------------------------*/
    static lv_disp_drv_t disp_drv;              // Display driver descriptor
    lv_disp_drv_init(&disp_drv);                // Basic initialization

    /* Set display resolution */
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;
    
    /* Set callback function to copy buffer content to display */
    disp_drv.flush_cb = disp_flush;

    /* Set display buffer */
    disp_drv.draw_buf = &draw_buf_dsc_1;

    /* If using Example 3 full-screen double buffer, enable this option
    disp_drv.full_refresh = 1;
    */

    /* GPU fill callback (if hardware acceleration available)
    disp_drv.gpu_fill_cb = gpu_fill;
    */

    /* Finally register the driver */
    lv_disp_drv_register(&disp_drv);
}

/**
 * @brief Enable screen refresh
 * @note When enabled, disp_flush() will write data to display
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/**
 * @brief Disable screen refresh
 * @note When disabled, disp_flush() won't write to display (useful for screenshot scenarios)
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief Initialize display hardware
 * @note Calls underlying ST7796 driver for initialization
 */
static void disp_init(void)
{
    // Call ST7796 hardware driver initialization function
    // This completes: SPI initialization, GPIO configuration, screen reset, initialization command sequence
    st7796_init();
}

/**
 * @brief Flush internal buffer content to specified display area
 * @param disp_drv Display driver pointer
 * @param area Area to refresh
 * @param color_p Color data pointer (RGB565 format)
 * @note Can use DMA or hardware acceleration for this operation, but must call lv_disp_flush_ready() when complete
 */
static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    // Check if refresh is allowed
    if (!disp_flush_enabled) {
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    // 1. Set display window (rectangular area to draw)
    st7796_set_window(area->x1, area->y1, area->x2, area->y2);
    
    // 2. Calculate pixel count
    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);
    
    // 3. Write color data
    // LVGL's lv_color_t is configured as RGB565 (16-bit) in lv_conf.h
    // This is compatible with ST7796's RGB565 format, can be transferred directly
    st7796_write_color((uint16_t *)color_p, size);
    
    // 4. Notify LVGL that flush is complete
    // Important: Must call this function to tell LVGL it can continue rendering next frame
    lv_disp_flush_ready(disp_drv);
}

/*
 * Optional GPU acceleration callback function examples below
 * Can be implemented to improve performance if hardware supports it
 */

#if 0  // Change to 1 if GPU fill support is needed

/**
 * @brief GPU fill callback (optional)
 * @note Can implement this function to accelerate fill operations if GPU or DMA support available
 */
static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
        const lv_area_t * fill_area, lv_color_t color)
{
    /* Implement GPU fill logic */
    /* Example: Use DMA to quickly fill with single color */
}

#endif