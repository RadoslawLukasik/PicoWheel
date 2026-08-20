#ifndef PCF8575_H
#define PCF8575_H

#include <inttypes.h>
#include "hardware/i2c.h"

#define PCF8575_ADDR_DEF 0x20 /**< The default I2C address for PCF8575. */

// the struct for BMP280 sensor
typedef struct PCF8575 {
  i2c_inst_t * i2c;
  uint16_t last_write;
  uint8_t addr;
} PCF8575;

void PCF8575_init(PCF8575 *s, i2c_inst_t * i2c, uint8_t address);
uint16_t PCF8575_read_all(PCF8575 *s);
uint16_t PCF8575_read_mask(PCF8575 *s, uint16_t mask);
void PCF8575_write_all(PCF8575 *s, uint16_t mask_val);
void PCF8575_write_mask(PCF8575 *s, uint16_t mask, uint16_t mask_val);

#endif
