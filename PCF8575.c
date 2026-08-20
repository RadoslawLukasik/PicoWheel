#include "PCF8575.h"

void PCF8575_init(PCF8575 *s, i2c_inst_t * i2c, uint8_t address) {
  s->i2c = i2c;
  s->addr = address;
  //uint8_t buf[2] = {0x00, 0x00}; kill the PCF if there are external pull-up
  //i2c_write_blocking(i2c, s->addr, buf, 2, false);
}

uint16_t PCF8575_read_all(PCF8575 *s) {
  uint8_t buf[2] = { 0xFF, 0xFF};// all ports desired as input should be set to logic 1.
  i2c_write_blocking(s->i2c, s->addr, buf, 2, true);  // true to keep master control of bus
  i2c_read_blocking(s->i2c, s->addr, buf, 2, false);  // false - finished with bus
  return ((((uint16_t) buf[1]) << 8) | buf[0]);
}

uint16_t PCF8575_read_mask(PCF8575 *s, uint16_t mask) {
  uint8_t buf[2] = { mask& 0xFF, mask >> 8};// all ports desired as input should be set to logic 1.
  i2c_write_blocking(s->i2c, s->addr, buf, 2, true);  // true to keep master control of bus
  i2c_read_blocking(s->i2c, s->addr, buf, 2, false);  // false - finished with bus
  return ((((uint16_t) buf[1]) << 8) | buf[0]);
}

void PCF8575_write_all(PCF8575 *s, uint16_t mask_val) {
  uint8_t buf[2] = {mask_val & 0xFF, mask_val >> 8};
  i2c_write_blocking(s->i2c, s->addr, buf, 2, false);  // false - finished with bus
  s->last_write = mask_val;
}

void PCF8575_write(PCF8575 *s, uint16_t mask, uint16_t mask_val) {
  s->last_write = (s->last_write & ~mask) | (mask & mask_val);
  uint8_t buf[2] = {mask_val & 0xFF, mask_val >> 8};
  i2c_write_blocking(s->i2c, s->addr, buf, 2, false);  // false - finished with bus
}