#pragma once

#include "board_io.h"
#include "gpio.h"
#include "mp3_ssp.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define WRITE_OP_CODE 0x02
#define READ_OP_CODE 0x03

#define SCI_MODE_ADDR 0x00
#define SCI_STATUS_ADDR 0x01
#define SCI_BASS_ADDR 0x02
#define SCI_CLKF_ADDR 0x03
#define SCI_AUDATA_ADDR 0x05
#define SCI_VOL_ADDR 0x0B
#define SCI_AIADDR_ADDR 0x0A

#define default_bass_treble 0x00F6
#define default_vol 0x2424
#define default_sample_rate 0xAC45 // 44.1 kHz
#define default_clk_freq 0x9800

#define dummy_data 0xFF

typedef struct mp3_wires {
  gpio_s decoder_XDCS; // XDCS
  gpio_s decoder_XCS;  // XCS
  gpio_s decoder_DREQ; // DREQ
  gpio_s decoder_RST;  // RST
} mp3_pins;

void mp3_startup(void);
void mp3_pins_setup(void);
void mp3_decoder_ds(gpio_s wire);
void mp3_decoder_cs(gpio_s wire);
void mp3_ssp_write(uint8_t bytes_from_mp3_file);
void sci_write_reg(uint16_t addr, uint8_t first_byte, uint8_t second_byte);
uint16_t sci_read_reg(uint16_t addr);
bool stream_buffer_full();
