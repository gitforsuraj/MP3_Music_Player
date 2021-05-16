#include "gpio_isr.h"
#include "FreeRTOS.h"
#include "delay.h"
#include "gpio_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "task.h"
#include <stdio.h>

// static function_pointer_t gpio0_callbacks[32];

// void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
//   gpio0__set_as_input(pin);
//   NVIC_EnableIRQ(GPIO_IRQn);

//   if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
//     LPC_GPIOINT->IO0IntEnF |= (1 << pin);
//   } else if (interrupt_type == GPIO_INTR__RISING_EDGE) {
//     LPC_GPIOINT->IO0IntEnR |= (1 << pin);
//   }
//   gpio0_callbacks[pin] = callback;
// }

// void gpio0__interrupt_dispatcher(void) {
//   int pin_that_generated_interrupt = 0;
//   for (int index = 0; index < sizeof(gpio0_callbacks) / sizeof(int); index++) {
//     if (LPC_GPIOINT->IO0IntStatR & (1 << index) || LPC_GPIOINT->IO0IntStatF & (1 << index)) {
//       pin_that_generated_interrupt = index;
//     }
//   }
//   function_pointer_t attached_user_handler = gpio0_callbacks[pin_that_generated_interrupt];
//   attached_user_handler();
//   LPC_GPIOINT->IO0IntClr = (1 << pin_that_generated_interrupt);
// }

// Note: You may want another separate array for falling vs. rising edge callbacks

static function_pointer_t gpio0_callbacks[32];
static function_pointer_t gpio2_callbacks[32];
gpio_interrupt_e Interrupt_edge_type[32];
// static  current_channel_array =

// static enum
// {

// }GPIO_CHANNEL

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  // 1) Store the callback based on the pin at gpio0_callbacks
  // 2) Configure GPIO 0 pin for rising or falling edge
  uint32_t pin_N = (1 << pin);
  LPC_GPIO0->DIR &= ~pin_N;

  fprintf(stderr, "in gpio0_attach_interrupt \n");
  gpio0_callbacks[pin] = callback;
  Interrupt_edge_type[pin] = interrupt_type;
  if (Interrupt_edge_type[pin]) {
    LPC_GPIOINT->IO0IntEnR |= pin_N;
  } else {
    LPC_GPIOINT->IO0IntEnF |= pin_N;
  }

  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, callback, "name");
  NVIC_EnableIRQ(GPIO_IRQn);
}

// We wrote some of the implementation for you
void gpio0__interrupt_dispatcher(void) {
  // Check which pin generated the interrupt

  // int pin_that_generated_interrupt = 30;

  for (int pin_num = 0; pin_num < 32; pin_num++) {
    if (LPC_GPIOINT->IO0IntStatR & (1 << pin_num) || LPC_GPIOINT->IO0IntStatF & (1 << pin_num)) {
      const int pin_that_generated_interrupt = pin_num;
      function_pointer_t attached_user_handler = gpio0_callbacks[pin_that_generated_interrupt];
      attached_user_handler();
      LPC_GPIOINT->IO0IntClr = (1 << pin_that_generated_interrupt);
      break;
    }
  }
}

void gpio2__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  uint32_t pin_N = (1 << pin);
  LPC_GPIO2->DIR &= ~pin_N;

  fprintf(stderr, "in gpio2_attach_interrupt \n");
  gpio2_callbacks[pin] = callback;
  Interrupt_edge_type[pin] = interrupt_type;
  if (interrupt_type) {
    LPC_GPIOINT->IO2IntEnR |= pin_N;
  } else {
    LPC_GPIOINT->IO2IntEnF |= pin_N;
  }

  // lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, callback, "name");
  NVIC_EnableIRQ(GPIO_IRQn);
}

void gpio2__interrupt_dispatcher(void) {
  for (int pin_num = 0; pin_num < 32; pin_num++) {
    if (LPC_GPIOINT->IO2IntStatR & (1 << pin_num) || LPC_GPIOINT->IO2IntStatF & (1 << pin_num)) {
      const int pin_that_generated_interrupt = pin_num;
      function_pointer_t attached_user_handler = gpio2_callbacks[pin_that_generated_interrupt];
      attached_user_handler();
      LPC_GPIOINT->IO2IntClr = (1 << pin_that_generated_interrupt);
      break;
    }
  }
}
