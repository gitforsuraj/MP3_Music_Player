#include "mp3_decoder.h"
#include "FreeRTOS.h"
#include "board_io.h"
#include "clock.h"
#include "delay.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "mp3_ssp.h"
#include "task.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static mp3_pins pin;

void mp3_startup(void) {
  mp3_pins_setup();
  mp3_decoder_set(pin.decoder_RST);
  printf("Expected value is high, %i\n", LPC_GPIO2->PIN & (1 << 4) ? true : false);

  ssp0_init(1000);

  mp3_decoder_set(pin.decoder_XCS);
  mp3_decoder_set(pin.decoder_XDCS);

  sci_write_reg(SCI_VOL_ADDR, 0x10, 0x10);
  sci_write_reg(SCI_BASS_ADDR, 0x09, 0x01);
  sci_write_reg(SCI_CLKF_ADDR, 0x60, 0x00);

  uint16_t mode = sci_read_reg(SCI_VOL_ADDR);
  printf("VOLUME: 0x%x\n", mode);
  mode = sci_read_reg(SCI_CLKF_ADDR);
  printf("CLK: 0x%x\n", mode);
}

void mp3_pins_setup(void) {
  pin.decoder_XCS = gpio__construct_as_output(2, 1);  // XCS low active
  pin.decoder_XDCS = gpio__construct_as_output(2, 2); // XDCS low active
  pin.decoder_DREQ = gpio__construct_as_input(2, 0);  // DREQ
  pin.decoder_RST = gpio__construct_as_output(2, 4);  // XRESET low active

  gpio__construct_with_function(0, 15, GPIO__FUNCTION_2); // SSP0 SCK
  gpio__construct_with_function(0, 18, GPIO__FUNCTION_2); // SSP0 MOSI
  gpio__construct_with_function(0, 17, GPIO__FUNCTION_2); // SSP0 MISO

  gpio__construct_with_function(2, 1, GPIO__FUNCITON_0_IO_PIN);
  gpio__construct_with_function(2, 2, GPIO__FUNCITON_0_IO_PIN);
  gpio__construct_with_function(2, 0, GPIO__FUNCITON_0_IO_PIN);
  gpio__construct_with_function(2, 4, GPIO__FUNCITON_0_IO_PIN);
}

void mp3_decoder_set(gpio_s wire) { gpio__set(wire); }     // SET
void mp3_decoder_reset(gpio_s wire) { gpio__reset(wire); } // RESET

void mp3_ssp_write(uint8_t bytes_from_mp3_file) {
  mp3_decoder_reset(pin.decoder_XDCS);

  ssp0_exchange_byte(bytes_from_mp3_file);

  mp3_decoder_set(pin.decoder_XDCS);
}

void sci_write_reg(uint16_t addr, uint8_t first_byte, uint8_t second_byte) {
  while (!stream_buffer_full) {
    ;
  }
  mp3_decoder_reset(pin.decoder_XCS);

  ssp0_exchange_byte(WRITE_OP_CODE);
  ssp0_exchange_byte(addr);
  ssp0_exchange_byte(first_byte);
  ssp0_exchange_byte(second_byte);

  while (!stream_buffer_full) {
    ;
  }

  mp3_decoder_set(pin.decoder_XCS);
}

uint16_t sci_read_reg(uint16_t addr) {
  uint16_t register_read = 0;
  while (!stream_buffer_full) {
    ;
  }

  mp3_decoder_reset(pin.decoder_XCS);

  ssp0_exchange_byte(READ_OP_CODE);
  ssp0_exchange_byte(addr);

  uint8_t first_byte = ssp0_exchange_byte(dummy_data);
  while (!stream_buffer_full) {
    ;
  }

  uint8_t second_byte = ssp0_exchange_byte(dummy_data);
  while (!stream_buffer_full) {
    ;
  }

  mp3_decoder_set(pin.decoder_XCS);

  register_read |= ((first_byte << 8) | (second_byte << 0));
  return register_read;
}

bool stream_buffer_full() {
  uint32_t pin_dreq = (1 << 0);
  int dreq_status = LPC_GPIO2->PIN & pin_dreq;

  if (dreq_status) {
    return true;
  } else {
    return false;
  }
}
