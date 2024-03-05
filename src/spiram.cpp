#include "spiram.hpp"
#include "pico/stdlib.h"
#include "hardware/spi.h"

enum Command{
  WRITE = 0x02,
  READ = 0x03,
  READ_FAST = 0x0b,
  READ_FAST_QUAD = 0xeb,
  WRITE_QUAD = 0x38,
};

uint8_t *buf_write_byte(uint8_t *buf, uint8_t val) {
  *buf = val;
  return ++buf;
}

uint8_t *buf_write_word(uint8_t *buf, uint16_t val) {
  buf = buf_write_byte(buf, val);
  buf = buf_write_byte(buf, val>>8);
  return buf;
}

uint8_t *buf_write_tribyte(uint8_t *buf, uint32_t val) {
  buf = buf_write_byte(buf, val);
  buf = buf_write_byte(buf, val>>8);
  buf = buf_write_byte(buf, val>>16);
  return buf;
}

uint8_t *buf_write_dword(uint8_t *buf, uint32_t val) {
  buf = buf_write_word(buf, val);
  buf = buf_write_word(buf, val>>16);
  return buf;
}

uint8_t buf_read_byte(uint8_t *buf) {
  return buf[0];
}

uint16_t buf_read_word(uint8_t *buf) {
  return buf[0] | (buf[1]<<8);
}

uint32_t buf_read_tribyte(uint8_t *buf) {
  return buf[0] | (buf[1]<<8) | (buf[2]<<16);
}

uint32_t buf_read_dword(uint8_t *buf) {
  return buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);
}


SpiRam::SpiRam(uint mosi, uint miso, uint sclk, uint cs) : cs{cs} {
  gpio_set_dir(mosi, GPIO_OUT);
  gpio_set_dir(miso, GPIO_IN);
  gpio_set_dir(sclk, GPIO_OUT);
  gpio_set_dir(cs, GPIO_OUT);
  gpio_put(cs, 1);
  gpio_set_function(mosi, GPIO_FUNC_SPI);
  gpio_set_function(miso, GPIO_FUNC_SPI);
  gpio_set_function(sclk, GPIO_FUNC_SPI);
  gpio_set_function(cs, GPIO_FUNC_SIO);
  spi_init(spi0, 31'250'000);

  sleep_ms(1);
  gpio_put(cs, 0);
  spi_write_blocking(spi0, (uint8_t*)"\x66\x99", 2);
  gpio_put(cs, 1);
}

uint8_t * make_cmd(uint8_t cmd, uintptr_t addr, uint8_t *buf) {
  buf = buf_write_byte(buf, cmd);
  buf = buf_write_tribyte(buf, addr);
  return buf;
}

uint8_t SpiRam::read_byte(uintptr_t addr) {
  uint8_t buf[5];
  gpio_put(cs, 0);
  make_cmd(READ, addr, buf);
  spi_write_read_blocking(spi0, buf, buf, 5);
  gpio_put(cs, 1);
  return buf[4];
}

uint16_t SpiRam::read_word(uintptr_t addr) {
  uint8_t buf[6];
  gpio_put(cs, 0);
  make_cmd(READ, addr, buf);
  spi_write_read_blocking(spi0, buf, buf, 6);
  gpio_put(cs, 1);
  return buf_read_word(&buf[4]);
}

uint32_t SpiRam::read_dword(uintptr_t addr) {
  uint8_t buf[8];
  gpio_put(cs, 0);
  make_cmd(READ, addr, buf);
  spi_write_read_blocking(spi0, buf, buf, 8);
  gpio_put(cs, 1);
  return buf_read_dword(&buf[4]);
}

void SpiRam::read_data(uintptr_t addr, uint32_t nbytes, uint8_t *data) {
  uint8_t buf[4];
  gpio_put(cs, 0);
  make_cmd(READ, addr, buf);
  spi_write_blocking(spi0, buf, 6);
  spi_read_blocking(spi0, 0, data, nbytes);
  gpio_put(cs, 1);
}

void SpiRam::write_byte(uintptr_t addr, uint8_t value) {
  uint8_t buf[5];
  uint8_t *p;
  gpio_put(cs, 0);
  p = make_cmd(WRITE, addr, buf);
  p = buf_write_byte(p, value);
  spi_write_blocking(spi0, buf, 5);
  sleep_ms(1);
  gpio_put(cs, 1);
}

void SpiRam::write_word(uintptr_t addr, uint16_t value) {
  uint8_t buf[6];
  uint8_t *p;
  p = make_cmd(WRITE, addr, buf);
  p = buf_write_word(p, value);
  gpio_put(cs, 0);
  spi_write_blocking(spi0, buf, 6);
  gpio_put(cs, 1);
}

void SpiRam::write_dword(uintptr_t addr, uint32_t value) {
  uint8_t buf[8];
  uint8_t *p;
  p = make_cmd(WRITE, addr, buf);
  p = buf_write_dword(p, value);
  gpio_put(cs, 0);
  spi_write_blocking(spi0, buf, 8);
  gpio_put(cs, 1);
}

void SpiRam::write_data(uintptr_t addr, uint32_t nbytes, const uint8_t *data) {
  uint8_t buf[4];
  make_cmd(WRITE, addr, buf);
  gpio_put(cs, 0);
  spi_write_blocking(spi0, buf, 4);
  spi_write_blocking(spi0, data, nbytes);
  gpio_put(cs, 1);
}
