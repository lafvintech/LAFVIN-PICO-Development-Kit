/**
 * @file st7796.h
 * @brief ST7796 LCD Driver Header
 * @note Implementation based on ST7796 chip datasheet
 * @author NIGHT
 * @date 2025-10-27
 */

 #ifndef ST7796_H
 #define ST7796_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 /**********************
  *      DEFINES
  **********************/
 /* Screen Resolution */
 #define ST7796_WIDTH    320
 #define ST7796_HEIGHT   480

/* Hardware Pin Configuration */
#define ST7796_SPI_PORT     spi0
#define ST7796_PIN_CLK      2
#define ST7796_PIN_MOSI     3
#define ST7796_PIN_CS       5
#define ST7796_PIN_DC       6
#define ST7796_PIN_RST      7

/* SPI Clock Frequency (Hz) */
#define ST7796_SPI_BAUDRATE (62500000)  // 62.5MHz

/* ST7796 Command Definitions - from datasheet */
#define ST7796_CMD_SWRESET      0x01
#define ST7796_CMD_SLPIN        0x10
#define ST7796_CMD_SLPOUT       0x11
#define ST7796_CMD_INVOFF       0x20
#define ST7796_CMD_INVON        0x21
#define ST7796_CMD_DISPOFF      0x28
#define ST7796_CMD_DISPON       0x29
#define ST7796_CMD_CASET        0x2A  // Column Address Set
#define ST7796_CMD_RASET        0x2B  // Row Address Set
#define ST7796_CMD_RAMWR        0x2C  // Memory Write
#define ST7796_CMD_MADCTL       0x36  // Memory Access Control
#define ST7796_CMD_COLMOD       0x3A  // Pixel Format Set

/**********************
 *      TYPEDEFS
 **********************/
/* Screen Orientation Definitions */
typedef enum {
    ST7796_PORTRAIT         = 0,  // Portrait mode
    ST7796_LANDSCAPE        = 1,  // Landscape mode
    ST7796_PORTRAIT_INV     = 2,  // Portrait mode inverted
    ST7796_LANDSCAPE_INV    = 3   // Landscape mode inverted
} st7796_orientation_t;

/**********************
 * FUNCTION PROTOTYPES
 **********************/
/**
 * @brief Initialize ST7796 display driver
 * @note Must be called before using other functions
 */
void st7796_init(void);

/**
 * @brief Set display orientation
 * @param orientation Screen orientation
 */
void st7796_set_orientation(st7796_orientation_t orientation);

/**
 * @brief Set display window (drawing area)
 * @param x1 Start X coordinate
 * @param y1 Start Y coordinate
 * @param x2 End X coordinate
 * @param y2 End Y coordinate
 */
void st7796_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * @brief Write color data to display area
 * @param color Color data pointer (RGB565 format)
 * @param len Number of pixels
 */
void st7796_write_color(const uint16_t *color, uint32_t len);

#endif /* ST7796_H */