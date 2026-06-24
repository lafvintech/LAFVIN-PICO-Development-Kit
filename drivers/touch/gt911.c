/**
 * @file gt911.c
 * @brief GT911 Capacitive Touch Screen Driver Implementation
 * @note Implementation based on GT911 chip datasheet and Pico SDK
 * @author NIGHT
 * @date 2025-01-27
 */

/*********************
 *      INCLUDES
 *********************/
#include "gt911.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <string.h>

/**********************
 *  STATIC PROTOTYPES
 **********************/
static bool gt911_i2c_init(void);
static bool gt911_i2c_read_reg(uint16_t reg, uint8_t *data, uint8_t len);
static bool gt911_i2c_write_reg(uint16_t reg, uint8_t *data, uint8_t len);
static void gt911_clear_status(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static gt911_dev_t gt911_dev = {
    .initialized = false,
    .product_id = {0},
    .max_x = 0,
    .max_y = 0,
    .i2c_addr = GT911_I2C_ADDR
};

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * @brief Initialize GT911 touch driver
 * @return true on success, false on failure
 */
bool gt911_init(void)
{
    uint8_t data;
    
    // Prevent duplicate initialization
    if (gt911_dev.initialized) {
        return true;
    }
    
    // 1. Initialize I2C interface
    if (!gt911_i2c_init()) {
        return false;
    }
    
    // 2. Try to read Product ID to verify communication
    if (!gt911_i2c_read_reg(GT911_REG_PRODUCT_ID1, &data, 1)) {
        return false;  // I2C communication failed
    }
    
    // 3. Read complete Product ID (4 ASCII bytes)
    // Example: GT911 returns "911" (0x39, 0x31, 0x31)
    for (int i = 0; i < GT911_PRODUCT_ID_LEN; i++) {
        if (!gt911_i2c_read_reg(GT911_REG_PRODUCT_ID1 + i, (uint8_t *)&gt911_dev.product_id[i], 1)) {
            return false;
        }
    }
    gt911_dev.product_id[GT911_PRODUCT_ID_LEN] = '\0';  // Null terminator
    
    // 4. Read Vendor ID (optional, for verification)
    if (!gt911_i2c_read_reg(GT911_REG_VENDOR_ID, &data, 1)) {
        return false;
    }
    
    // 5. Read touchscreen resolution configuration
    // X resolution (16-bit, low byte first)
    if (!gt911_i2c_read_reg(GT911_REG_X_RES_L, &data, 1)) {
        return false;
    }
    gt911_dev.max_x = data;
    
    if (!gt911_i2c_read_reg(GT911_REG_X_RES_H, &data, 1)) {
        return false;
    }
    gt911_dev.max_x |= ((uint16_t)data << 8);
    
    // Y resolution (16-bit, low byte first)
    if (!gt911_i2c_read_reg(GT911_REG_Y_RES_L, &data, 1)) {
        return false;
    }
    gt911_dev.max_y = data;
    
    if (!gt911_i2c_read_reg(GT911_REG_Y_RES_H, &data, 1)) {
        return false;
    }
    gt911_dev.max_y |= ((uint16_t)data << 8);
    
    // 6. Initialization complete
    gt911_dev.initialized = true;
    
    return true;
}

/**
 * @brief Read touch data
 * @param x Output parameter: X coordinate
 * @param y Output parameter: Y coordinate
 * @param pressed Output parameter: Touch state
 * @return true on success, false on failure
 */
bool gt911_read_touch(uint16_t *x, uint16_t *y, bool *pressed)
{
    uint8_t status_reg;
    uint8_t data;
    static uint16_t last_x = 0;  // Save last coordinates
    static uint16_t last_y = 0;
    
    // Check if initialized
    if (!gt911_dev.initialized) {
        return false;
    }
    
    // 1. Read status register
    if (!gt911_i2c_read_reg(GT911_REG_STATUS, &status_reg, 1)) {
        return false;
    }
    
    // 2. Get touch point count (lower 4 bits)
    uint8_t touch_count = status_reg & GT911_STATUS_PT_MASK;
    
    // 3. Check if data is ready and clear status register
    // bit7=1 indicates new touch data, need to clear status register after reading
    if ((status_reg & GT911_STATUS_BUF_READY) || (touch_count < 6)) {
        gt911_clear_status();  // Clear status to tell GT911 we have read the data
    }
    
    // 4. Process touch data
    if (touch_count == 1) {
        // Single touch: read first touch point coordinates
        
        // Read X coordinate (16-bit, low byte first)
        if (!gt911_i2c_read_reg(GT911_REG_PT1_X_L, &data, 1)) {
            return false;
        }
        last_x = data;
        
        if (!gt911_i2c_read_reg(GT911_REG_PT1_X_H, &data, 1)) {
            return false;
        }
        last_x |= ((uint16_t)data << 8);
        
        // Read Y coordinate (16-bit, low byte first)
        if (!gt911_i2c_read_reg(GT911_REG_PT1_Y_L, &data, 1)) {
            return false;
        }
        last_y = data;
        
        if (!gt911_i2c_read_reg(GT911_REG_PT1_Y_H, &data, 1)) {
            return false;
        }
        last_y |= ((uint16_t)data << 8);
        
        // Set output parameters
        *x = last_x;
        *y = last_y;
        *pressed = true;
        
    } else {
        // No touch or multi-touch (multi-touch not supported yet)
        // Return last coordinates with released state
        *x = last_x;
        *y = last_y;
        *pressed = false;
    }
    
    return true;
}

/**
 * @brief Get device information
 * @return Pointer to device information structure
 */
gt911_dev_t* gt911_get_dev_info(void)
{
    return &gt911_dev;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * @brief Initialize I2C interface
 * @return true on success, false on failure
 */
static bool gt911_i2c_init(void)
{
    // 1. Initialize I2C peripheral, set baudrate to 100kHz
    uint32_t actual_baudrate = i2c_init(GT911_I2C_PORT, GT911_I2C_BAUDRATE);
    
    if (actual_baudrate == 0) {
        return false;  // I2C initialization failed
    }
    
    // 2. Configure GPIO pins for I2C function
    gpio_set_function(GT911_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(GT911_PIN_SCL, GPIO_FUNC_I2C);
    
    // 3. Enable internal pull-up resistors
    // I2C bus requires pull-up resistors to work properly
    gpio_pull_up(GT911_PIN_SDA);
    gpio_pull_up(GT911_PIN_SCL);
    
    // Brief delay to wait for I2C bus stabilization
    sleep_ms(10);
    
    return true;
}

/**
 * @brief Read data from GT911 register
 * @param reg Register address (16-bit)
 * @param data Data buffer pointer
 * @param len Number of bytes to read
 * @return true on success, false on failure
 */
static bool gt911_i2c_read_reg(uint16_t reg, uint8_t *data, uint8_t len)
{
    if (data == NULL || len == 0) {
        return false;
    }
    
    // GT911 uses 16-bit register address, need to send high and low bytes
    uint8_t reg_addr[2] = {
        (reg >> 8) & 0xFF,  // Register address high byte
        reg & 0xFF          // Register address low byte
    };
    
    // I2C read operation in two steps:
    // 1. Write register address (keep bus, no STOP signal)
    int ret = i2c_write_blocking(GT911_I2C_PORT, gt911_dev.i2c_addr, reg_addr, 2, true);
    if (ret < 0) {
        return false;  // Write failed
    }
    
    // 2. Read data
    ret = i2c_read_blocking(GT911_I2C_PORT, gt911_dev.i2c_addr, data, len, false);
    if (ret < 0) {
        return false;  // Read failed
    }
    
    return true;
}

/**
 * @brief Write data to GT911 register
 * @param reg Register address (16-bit)
 * @param data Data buffer pointer
 * @param len Number of bytes to write
 * @return true on success, false on failure
 */
static bool gt911_i2c_write_reg(uint16_t reg, uint8_t *data, uint8_t len)
{
    if (data == NULL || len == 0) {
        return false;
    }
    
    // Combine register address and data
    uint8_t buffer[32];  // Temporary buffer
    
    if (len + 2 > sizeof(buffer)) {
        return false;  // Data too long
    }
    
    // Register address (high byte first)
    buffer[0] = (reg >> 8) & 0xFF;
    buffer[1] = reg & 0xFF;
    
    // Copy data
    memcpy(&buffer[2], data, len);
    
    // Send data
    int ret = i2c_write_blocking(GT911_I2C_PORT, gt911_dev.i2c_addr, buffer, len + 2, false);
    
    return (ret > 0);
}

/**
 * @brief Clear GT911 status register
 * @note Must clear status register after reading touch data, otherwise GT911 won't update new touch data
 */
static void gt911_clear_status(void)
{
    // Write 0x00 to status register (0x814E)
    // This tells GT911 chip: "I have read the touch data, you can prepare next frame"
    uint8_t clear_data = 0x00;
    
    // Method 1: Use wrapped write register function
    // gt911_i2c_write_reg(GT911_REG_STATUS, &clear_data, 1);
    
    // Method 2: Direct send (more efficient)
    uint8_t buffer[3] = {
        0x81,       // Register address high byte (0x814E >> 8)
        0x4E,       // Register address low byte (0x814E & 0xFF)
        0x00        // Clear data
    };
    
    i2c_write_blocking(GT911_I2C_PORT, gt911_dev.i2c_addr, buffer, 3, false);
}

