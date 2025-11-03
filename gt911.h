/**
 * @file gt911.h
 * @brief GT911 Capacitive Touch Screen Driver Header
 * @note Implementation based on GT911 chip datasheet
 * @author NIGHT
 * @date 2025-10-27
 */

 #ifndef GT911_H
 #define GT911_H
 
 #include <stdint.h>
 #include <stdbool.h>
 
 /**********************
  *      DEFINES
  **********************/
 /* I2C Slave Address */
 #define GT911_I2C_ADDR          0x5D
 
 /* Product ID Length */
 #define GT911_PRODUCT_ID_LEN    4
 
 /* Hardware Pin Configuration */
 #define GT911_I2C_PORT          i2c0
 #define GT911_PIN_SDA           8
 #define GT911_PIN_SCL           9
 #define GT911_I2C_BAUDRATE      100000  // 100kHz
 
 /* GT911 Register Addresses - from chip datasheet */
 #define GT911_REG_PRODUCT_ID1       0x8140
 #define GT911_REG_PRODUCT_ID2       0x8141
 #define GT911_REG_PRODUCT_ID3       0x8142
 #define GT911_REG_PRODUCT_ID4       0x8143
 #define GT911_REG_FIRMWARE_VER_L    0x8144
 #define GT911_REG_FIRMWARE_VER_H    0x8145
 #define GT911_REG_X_RES_L           0x8146  // X resolution low byte
 #define GT911_REG_X_RES_H           0x8147  // X resolution high byte
 #define GT911_REG_Y_RES_L           0x8148  // Y resolution low byte
 #define GT911_REG_Y_RES_H           0x8149  // Y resolution high byte
 #define GT911_REG_VENDOR_ID         0x814A
 
 #define GT911_REG_STATUS            0x814E  // Touch status register
 #define GT911_REG_TRACK_ID1         0x814F
 #define GT911_REG_PT1_X_L           0x8150  // Touch point 1 X coordinate low byte
 #define GT911_REG_PT1_X_H           0x8151  // Touch point 1 X coordinate high byte
 #define GT911_REG_PT1_Y_L           0x8152  // Touch point 1 Y coordinate low byte
 #define GT911_REG_PT1_Y_H           0x8153  // Touch point 1 Y coordinate high byte
 #define GT911_REG_PT1_SIZE_L        0x8154
 #define GT911_REG_PT1_SIZE_H        0x8155
 
 /* Status Register Bit Definitions */
 #define GT911_STATUS_BUF_READY      0x80  // Data ready flag
 #define GT911_STATUS_LARGE          0x40
 #define GT911_STATUS_PROX_VALID     0x20
 #define GT911_STATUS_HAVE_KEY       0x10
 #define GT911_STATUS_PT_MASK        0x0F  // Touch point count mask
 
 /**********************
  *      TYPEDEFS
  **********************/
 /**
  * @brief GT911 Device Status Structure
  */
 typedef struct {
     bool initialized;               // Initialization flag
     char product_id[GT911_PRODUCT_ID_LEN + 1];  // Product ID (string)
     uint16_t max_x;                 // Maximum X coordinate
     uint16_t max_y;                 // Maximum Y coordinate
     uint8_t i2c_addr;               // I2C address
 } gt911_dev_t;
 
 /**********************
  * FUNCTION PROTOTYPES
  **********************/
 /**
  * @brief Initialize GT911 touch driver
  * @return true on success, false on failure
  */
 bool gt911_init(void);
 
 /**
  * @brief Read touch data
  * @param x Output parameter: X coordinate
  * @param y Output parameter: Y coordinate
  * @param pressed Output parameter: Touch state
  * @return true on success, false on failure
  */
 bool gt911_read_touch(uint16_t *x, uint16_t *y, bool *pressed);
 
 /**
  * @brief Get device information (optional)
  * @return Pointer to device information structure
  */
 gt911_dev_t* gt911_get_dev_info(void);
 
 #endif /* GT911_H */