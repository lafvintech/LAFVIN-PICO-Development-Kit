/**
 * @file st7796.c
 * @brief ST7796 LCD Driver Implementation
 * @note Implementation based on ST7796 chip datasheet and Pico SDK
 * @author NIGHT
 * @date 2025-10-27
 */

/*********************
 *      INCLUDES
 *********************/
#include "st7796.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <string.h>

/**********************
 *      DEFINES
 **********************/
/* GPIO Control Macros */
#define LCD_CS_LOW()    gpio_put(ST7796_PIN_CS, 0)
#define LCD_CS_HIGH()   gpio_put(ST7796_PIN_CS, 1)
#define LCD_DC_CMD()    gpio_put(ST7796_PIN_DC, 0)
#define LCD_DC_DATA()   gpio_put(ST7796_PIN_DC, 1)
#define LCD_RST_LOW()   gpio_put(ST7796_PIN_RST, 0)
#define LCD_RST_HIGH()  gpio_put(ST7796_PIN_RST, 1)

/**********************
 *      TYPEDEFS
 **********************/
/**
 * @brief LCD Initialization Command Structure
 */
typedef struct {
    uint8_t cmd;            // Command byte
    uint8_t data[16];       // Data bytes (max 16)
    uint8_t databytes;      // Number of data bytes (bit7=1 means delay required)
} lcd_init_cmd_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void st7796_write_cmd(uint8_t cmd);
static void st7796_write_data(const uint8_t *data, uint16_t len);
static void st7796_hw_reset(void);
static void st7796_gpio_init(void);
static void st7796_spi_init(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static st7796_orientation_t current_orientation = ST7796_PORTRAIT;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Initialize ST7796 display driver
 */
void st7796_init(void)
{
    // 1. Initialize SPI interface
    st7796_spi_init();
    
    // 2. Initialize GPIO pins
    st7796_gpio_init();
    
    // 3. Hardware reset
    st7796_hw_reset();
    
    // 4. Send initialization command sequence
    // Commands from ST7796 datasheet and screen manufacturer recommended configuration
    lcd_init_cmd_t init_cmds[] = {
        {0xCF, {0x00, 0x83, 0x30}, 3},
        {0xED, {0x64, 0x03, 0x12, 0x81}, 4},
        {0xE8, {0x85, 0x01, 0x79}, 3},
        {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
        {0xF7, {0x20}, 1},
        {0xEA, {0x00, 0x00}, 2},
        
        // Power Control
        {0xC0, {0x26}, 1},          // Power Control 1
        {0xC1, {0x11}, 1},          // Power Control 2
        {0xC5, {0x35, 0x3E}, 2},    // VCOM Control 1
        {0xC7, {0xBE}, 1},          // VCOM Control 2
        
        // Display Settings
        {0x36, {0x28}, 1},          // Memory Access Control
        {0x3A, {0x05}, 1},          // Pixel Format Set (RGB565)
        
        // Frame Rate Control
        {0xB1, {0x00, 0x1B}, 2},
        {0xF2, {0x08}, 1},
        {0x26, {0x01}, 1},
        
        // Gamma Correction - affects display color quality
        {0xE0, {0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0x87, 
                0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00}, 15},  // Positive Gamma
        {0xE1, {0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 
                0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F}, 15},  // Negative Gamma
        
        // Set display area
        {0x2A, {0x00, 0x00, 0x00, 0xEF}, 4},  // Column Address Set
        {0x2B, {0x00, 0x00, 0x01, 0x3F}, 4},  // Row Address Set
        {0x2C, {0}, 0},                        // Memory Write
        
        {0xB7, {0x07}, 1},
        {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},  // Display Function Control
        
        // Exit sleep mode (requires 100ms delay)
        {0x11, {0}, 0x80},  // Sleep Out (bit7=1 means delay required)
        
        // Turn on display (requires 100ms delay)
        {0x29, {0}, 0x80},  // Display ON
        
        // End marker
        {0, {0}, 0xFF},
    };
    
    // 5. Send initialization commands sequentially
    uint16_t cmd_idx = 0;
    while (init_cmds[cmd_idx].databytes != 0xFF) {
        st7796_write_cmd(init_cmds[cmd_idx].cmd);
        
        // If there is data, send it
        uint8_t data_len = init_cmds[cmd_idx].databytes & 0x1F;  // Lower 5 bits = data length
        if (data_len > 0) {
            st7796_write_data(init_cmds[cmd_idx].data, data_len);
        }
        
        // If delay is needed (bit7=1)
        if (init_cmds[cmd_idx].databytes & 0x80) {
            sleep_ms(100);  // Delay 100ms
        }
        
        cmd_idx++;
    }
    
    // 6. Set default display orientation
    st7796_set_orientation(ST7796_PORTRAIT);
    
    // 7. Enable color inversion (may be needed depending on screen characteristics)
    st7796_write_cmd(0x21);  // Display Inversion ON
}

/**
 * @brief Set display orientation
 * @param orientation Screen orientation
 */
void st7796_set_orientation(st7796_orientation_t orientation)
{
    current_orientation = orientation;
    
    st7796_write_cmd(ST7796_CMD_MADCTL);  // 0x36
    
    uint8_t madctl_value;
    
    // Set MADCTL register according to orientation
    // MADCTL register bit definitions (refer to datasheet):
    // bit7: MY  - Row address order
    // bit6: MX  - Column address order
    // bit5: MV  - Row/Column exchange
    // bit4: ML  - Vertical refresh order
    // bit3: BGR - RGB/BGR order
    // bit2: MH  - Horizontal refresh order
    
    switch (orientation) {
        case ST7796_PORTRAIT:
            madctl_value = 0x48;  // MY=0, MX=1, MV=0, ML=0, BGR=1
            break;
        case ST7796_LANDSCAPE:
            madctl_value = 0x28;  // MY=0, MX=0, MV=1, ML=0, BGR=1
            break;
        case ST7796_PORTRAIT_INV:
            madctl_value = 0x88;  // MY=1, MX=0, MV=0, ML=0, BGR=1
            break;
        case ST7796_LANDSCAPE_INV:
            madctl_value = 0xE8;  // MY=1, MX=1, MV=1, ML=0, BGR=1
            break;
        default:
            madctl_value = 0x48;
            break;
    }
    
    LCD_CS_LOW();
    LCD_DC_DATA();
    spi_write_blocking(ST7796_SPI_PORT, &madctl_value, 1);
    LCD_CS_HIGH();
}

/**
 * @brief Set display window (drawing area)
 * @param x1 Start X coordinate
 * @param y1 Start Y coordinate
 * @param x2 End X coordinate
 * @param y2 End Y coordinate
 */
void st7796_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4];
    
    // Set column address range (X coordinate)
    st7796_write_cmd(ST7796_CMD_CASET);  // 0x2A
    data[0] = (x1 >> 8) & 0xFF;  // Start X high byte
    data[1] = x1 & 0xFF;         // Start X low byte
    data[2] = (x2 >> 8) & 0xFF;  // End X high byte
    data[3] = x2 & 0xFF;         // End X low byte
    st7796_write_data(data, 4);
    
    // Set row address range (Y coordinate)
    st7796_write_cmd(ST7796_CMD_RASET);  // 0x2B
    data[0] = (y1 >> 8) & 0xFF;  // Start Y high byte
    data[1] = y1 & 0xFF;         // Start Y low byte
    data[2] = (y2 >> 8) & 0xFF;  // End Y high byte
    data[3] = y2 & 0xFF;         // End Y low byte
    st7796_write_data(data, 4);
    
    // Prepare to write GRAM
    st7796_write_cmd(ST7796_CMD_RAMWR);  // 0x2C
}

/**
 * @brief Write color data to display area
 * @param color Color data pointer (RGB565 format, 2 bytes per pixel)
 * @param len Number of pixels
 * @note Must call st7796_set_window() to set display area before calling this function
 */
void st7796_write_color(const uint16_t *color, uint32_t len)
{
    if (len == 0 || color == NULL) {
        return;
    }
    
    LCD_CS_LOW();
    LCD_DC_DATA();
    
    // Write color data
    // RGB565 format: 2 bytes per pixel
    spi_write_blocking(ST7796_SPI_PORT, (const uint8_t *)color, len * 2);
    
    LCD_CS_HIGH();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief Send command to ST7796
 * @param cmd Command byte
 */
static void st7796_write_cmd(uint8_t cmd)
{
    LCD_CS_LOW();
    LCD_DC_CMD();       // DC=0 means sending command
    sleep_us(1);        // Brief delay to ensure signal stability
    
    spi_write_blocking(ST7796_SPI_PORT, &cmd, 1);
    
    sleep_us(1);
    LCD_CS_HIGH();
}

/**
 * @brief Send data to ST7796
 * @param data Data buffer pointer
 * @param len Data length (bytes)
 */
static void st7796_write_data(const uint8_t *data, uint16_t len)
{
    if (len == 0 || data == NULL) {
        return;
    }
    
    LCD_CS_LOW();
    LCD_DC_DATA();      // DC=1 means sending data
    sleep_us(1);
    
    spi_write_blocking(ST7796_SPI_PORT, data, len);
    
    sleep_us(1);
    LCD_CS_HIGH();
}

/**
 * @brief ST7796 hardware reset
 * @note According to datasheet, reset timing: RST low for at least 10us, then high, delay 120ms
 */
static void st7796_hw_reset(void)
{
    LCD_RST_HIGH();
    sleep_ms(100);
    
    LCD_RST_LOW();
    sleep_ms(100);
    
    LCD_RST_HIGH();
    sleep_ms(100);  // Wait for chip reset to complete
}

/**
 * @brief Initialize GPIO pins
 */
static void st7796_gpio_init(void)
{
    // Initialize CS (Chip Select) pin
    gpio_init(ST7796_PIN_CS);
    gpio_set_dir(ST7796_PIN_CS, GPIO_OUT);
    gpio_put(ST7796_PIN_CS, 1);  // Default high (not selected)
    
    // Initialize DC (Data/Command select) pin
    gpio_init(ST7796_PIN_DC);
    gpio_set_dir(ST7796_PIN_DC, GPIO_OUT);
    gpio_put(ST7796_PIN_DC, 1);  // Default high (data mode)
    
    // Initialize RST (Reset) pin
    gpio_init(ST7796_PIN_RST);
    gpio_set_dir(ST7796_PIN_RST, GPIO_OUT);
    gpio_put(ST7796_PIN_RST, 1);  // Default high (no reset)
}

/**
 * @brief Initialize SPI interface
 */
static void st7796_spi_init(void)
{
    // Initialize SPI peripheral, set baudrate
    spi_init(ST7796_SPI_PORT, ST7796_SPI_BAUDRATE);
    
    // Set SPI format
    // Parameters: 8-bit data, CPOL=0 (clock idle low), CPHA=0 (sample on first edge), MSB first
    spi_set_format(ST7796_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    // Configure GPIO pins for SPI function
    gpio_set_function(ST7796_PIN_MOSI, GPIO_FUNC_SPI);  // MOSI (data output)
    gpio_set_function(ST7796_PIN_CLK, GPIO_FUNC_SPI);   // CLK (clock)
}

