#include "mp3_lcd.h"
static int line_number = 1;

void uart_init(uint32_t peripheral_clock, uint32_t baud_rate) {
  LPC_IOCON->P4_28 &= ~(0b111); // U3_TXD
  LPC_IOCON->P4_29 &= ~(0b111); // U3_RXD
  LPC_IOCON->P4_28 |= (0b010);
  LPC_IOCON->P4_29 |= (0b010);

  LPC_SC->PCONP |= (1 << 25);            // Power UART3
  LPC_UART3->LCR |= (1 << 7) | (3 << 0); // Enable DLAB for baud rate

  const float fraction = 0.5;
  const uint16_t divider_16_bit = 96 * 1000 * 1000 / (16 * baud_rate) + fraction;

  LPC_UART3->FDR = (0 << 0) | (1 << 4);
  LPC_UART3->DLM = (divider_16_bit >> 8) & 0xFF;
  LPC_UART3->DLL = (divider_16_bit >> 0) & 0xFF;

  LPC_UART3->LCR &= ~(1 << 7); // Disable DLAB
  LPC_UART3->LCR = (3 << 0);   // Choosing 8 bit char length (0x3)
}

bool uart_write(uint8_t output_byte) {
  const uint8_t transmitter_holding_register_empty = (1 << 5);
  while (!((LPC_UART3->LSR) & transmitter_holding_register_empty)) {
    ;
  }
  LPC_UART3->THR = output_byte;
}

void lcd_show_all_songs(void) {
  for (size_t song_number = 0; song_number < song_list__get_item_count(); song_number++) {
    // if (song_number == song_list__get_item_count()) {
    //   song_number = 0;
    // }
    // fprintf(stderr, "SGON NUUMBER %i\n", song_number);
    lcd_write_string(song_list__get_name_for_item(song_number));
    lcd_set_cursor_to_line();
  }
}

void lcd_write_string(char *string) {
  for (int i = 0; i < strlen(string); i++) {
    uart_write((int)string[i]); // converts char to int
  }
}

void lcd_init(void) {
  lcd_clear_display();
  lcd_set_cursor_to_line();
  lcd_show_all_songs();
  lcd_cursor();
}

void lcd_clear_display(void) {
  uart_write(0xFE); // Control register
  uart_write(0x01); // Clear Display
}

void lcd_set_cursor_to_line(void) {
  uart_write(0xFE);
  uart_write(0x80);
  lcd_write_string("==== MP3 PLAYER ====");

  uart_write(0xFE);
  uart_write(0xC0);
  uart_write(0x3E); // inits arrow at first song

  if (line_number == 1) {
    uart_write(0xFE);
    uart_write(0xC1);
  } else if (line_number == 2) {
    uart_write(0xFE);
    uart_write(0x95);
  } else if (line_number == 3) {
    uart_write(0xFE);
    uart_write(0xD5);
  }
  line_number++;

  if (line_number == 4) {
    line_number = 1;
  }
}

void print_cursor_on_lcd(int list_of_songs_index) {
  if (list_of_songs_index == 0) {
    uart_write(0xFE);
    uart_write(0xD4);
    uart_write(0x20); // clear previous arrow

    uart_write(0xFE);
    uart_write(0xC0);
    uart_write(0x3E); // >
  } else if (list_of_songs_index == 1) {
    uart_write(0xFE);
    uart_write(0xC0);
    uart_write(0x20);

    uart_write(0xFE);
    uart_write(0x94);
    uart_write(0x3E);
  } else if (list_of_songs_index == 2) {
    uart_write(0xFE);
    uart_write(0x94);
    uart_write(0x20);

    uart_write(0xFE);
    uart_write(0xD4);
    uart_write(0x3E);
  }
}

void lcd_song_list_upperbound(int song_list_upperbound) { song_list_upperbound++; }

void lcd_song_list_lowerbound(int song_list_lowerbound) { song_list_lowerbound++; }

void lcd_print_upper_to_lower_songs(int song_list_upperbound, int song_list_lowerbound) {
  int song_list_middlebound = (song_list_upperbound + song_list_lowerbound) * 0.5;

  lcd_clear_row1();
  lcd_clear_row2();
  lcd_clear_row3();

  uart_write(0xFE);
  uart_write(0xC1);
  lcd_write_string(song_list__get_name_for_item(song_list_upperbound));

  uart_write(0xFE);
  uart_write(0x95);
  lcd_write_string(song_list__get_name_for_item(song_list_middlebound));

  uart_write(0xFE);
  uart_write(0xD5);
  lcd_write_string(song_list__get_name_for_item(song_list_lowerbound));
}

void lcd_meta(char title, char artist) {
  lcd_clear_display();

  uart_write(0xFE);
  uart_write(0x80);
  lcd_write_string("Title: ");
  lcd_write_string(title);

  uart_write(0xFE);
  uart_write(0xC0);
  lcd_write_string("Artist: ");
  lcd_write_string(artist);
}

void lcd_cursor(void) {
  uart_write(0xFE);
  uart_write(0xC0);
  uart_write(0x3E);
}

void lcd_clear_row1(void) {
  for (int i = 65; i < 83; i++) {
    uart_write(0xFE);
    uart_write(128 + i);
    uart_write(0x20);
  }
}

void lcd_clear_row2(void) {
  for (int i = 21; i < 39; i++) {
    uart_write(0xFE);
    uart_write(128 + i);
    uart_write(0x20);
  }
}
void lcd_clear_row3(void) {
  for (int i = 85; i < 103; i++) {
    uart_write(0xFE);
    uart_write(128 + i);
    uart_write(0x20);
  }
}

void volume_up_print(int volume_counter) {
  uart_write(0xFE);
  uart_write(0xD4);
  lcd_write_string("V:");
  char temp_string[1];
  int temp_int = volume_counter;
  sprintf(temp_string, "%i", temp_int);
  lcd_write_string(temp_string);
}

void volume_down_print(int volume_counter) {
  uart_write(0xFE);
  uart_write(0xD4);
  lcd_write_string("V:");
  char temp_string[1];
  int temp_int = volume_counter;
  sprintf(temp_string, "%i", temp_int);
  lcd_write_string(temp_string);
}

void bass_up_print(int bass_counter) {
  uart_write(0xFE);
  uart_write(0xD8);
  lcd_write_string("B:");
  char temp_string[1];
  int temp_int = bass_counter;
  sprintf(temp_string, "%i", temp_int);
  lcd_write_string(temp_string);
}

void bass_down_print(int bass_counter) {
  uart_write(0xFE);
  uart_write(0xD8);
  lcd_write_string("B:");
  char temp_string[1];
  int temp_int = bass_counter;
  sprintf(temp_string, "%i", temp_int);
  lcd_write_string(temp_string);
}

void treble_up_print(int treble_counter) {
  uart_write(0xFE);
  uart_write(0xDC);
  lcd_write_string("T:");
  char temp_string[1];
  int temp_int = treble_counter;
  sprintf(temp_string, "%i", temp_int);
  lcd_write_string(temp_string);
}

void treble_down_print(int treble_counter) {
  uart_write(0xFE);
  uart_write(0xDC);
  lcd_write_string("T:");
  char temp_string[1];
  int temp_int = treble_counter;
  sprintf(temp_string, "%i", temp_int);
  lcd_write_string(temp_string);
}