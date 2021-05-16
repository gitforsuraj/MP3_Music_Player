#include "mp3_ssp.h"
#include "FreeRTOS.h"
#include "board_io.h"
#include "clock.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "mp3_decoder.h"
#include "task.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ssp0_init(uint32_t max_clock_khz) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP0);
  LPC_SSP0->CR0 = 7;
  LPC_SSP0->CR1 = (1 << 1);

  ssp0_set_max_clock(max_clock_khz);
}

void ssp0_set_max_clock(uint32_t max_clock_khz) {
  uint8_t divider = 2;
  const uint32_t cpu_clock_mhz = clock__get_core_clock_hz(); // 96 mhz
  printf("cpu_clock_mhz = %li\n", cpu_clock_mhz);
  while ((cpu_clock_mhz / divider) > max_clock_khz && divider <= 254) {
    divider += 2;
  }
  printf("divider = %i\n", divider);
  LPC_SSP0->CPSR = 227;
}

uint8_t ssp0_exchange_byte(uint8_t data_out) {
  LPC_SSP0->DR = data_out;
  while (LPC_SSP0->SR & (1 << 4)) {
    ;
  }
  return (uint8_t)(LPC_SSP0->DR & 0xFF);
}
