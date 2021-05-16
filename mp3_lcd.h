#pragma once

#include "FreeRTOS.h"
#include "delay.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "queue.h"
#include "song_controller.h"
#include "task.h"
#include "uart.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void uart_init(uint32_t peripheral_clock, uint32_t baud_rate);
bool uart_write(uint8_t output_byte);

void lcd_init(void);
void lcd_clear_display(void);
void lcd_set_cursor_to_line(void);
void print_cursor_on_lcd(int list_of_songs_index);

void lcd_meta(char title, char artist);
void lcd_song_list_upperbound(int song_list_upperbound);
void lcd_song_list_lowerbound(int song_list_lowerbound);
void lcd_print_upper_to_lower_songs(int song_list_upperbound, int song_list_lowerbound);

void lcd_cursor(void);
void lcd_clear_row1(void);
void lcd_clear_row2(void);
void lcd_clear_row3(void);

void lcd_show_all_songs(void);
void lcd_write_string(char *string);

void volume_up_print(int volume_counter);
void volume_down_print(int volume_counter);
