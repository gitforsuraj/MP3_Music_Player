#pragma once

#include "board_io.h"
#include "gpio.h"
#include "lpc_peripherals.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ssp0_init(uint32_t max_clock_khz);
void ssp0_set_max_clock(uint32_t max_clock_khz);
uint8_t ssp0_exchange_byte(uint8_t data_out);
