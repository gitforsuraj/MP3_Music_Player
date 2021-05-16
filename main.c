#include "FreeRTOS.h"
#include "board_io.h"
#include "cli_handlers.h"
#include "common_macros.h"
#include "delay.h"
#include "ff.h"
#include "gpio_isr.h"
#include "mp3_decoder.h"
#include "mp3_lcd.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "sj2_cli.h"
#include "song_controller.h"
#include "task.h"
#include "uart.h"
#include "uart_lab.h"
#include <stdio.h>

typedef char songdata_t[512];
typedef char songname_t[32];

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;
QueueHandle_t Q_songlist;

// QueueHandle_t setting_control_up_pressed; // new
// QueueHandle_t setting_control_down_pressed; // new

TaskHandle_t player_handle;

SemaphoreHandle_t select_button_pressed;
SemaphoreHandle_t cursor_button_pressed;
SemaphoreHandle_t pause_button_pressed;
SemaphoreHandle_t previous_button_pressed;
SemaphoreHandle_t next_button_pressed;
SemaphoreHandle_t setting_button_pressed;
SemaphoreHandle_t setting_control_up_pressed;
SemaphoreHandle_t setting_control_down_pressed;
SemaphoreHandle_t menu_button_pressed;

typedef struct mp3_functions {
  gpio_s select;
  gpio_s cursor;
  gpio_s pause;
  gpio_s previous;
  gpio_s next;
  gpio_s setting;
  gpio_s setting_control_up;
  gpio_s setting_control_down;
  gpio_s menu;
} mp3_buttons;

static mp3_buttons button;

volatile int list_of_songs_index = 0;
volatile int setting_counter = 0;

volatile int song_list_upperbound = 0; // new
volatile int song_list_lowerbound = 2; // new

volatile bool pause = true;

void select_button_isr(void) {
  fprintf(stderr, "I am in select ISR\n");
  xSemaphoreGiveFromISR(select_button_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 6);
}

void cursor_button_isr(void) {
  fprintf(stderr, "I am in cursor ISR\n");
  xSemaphoreGiveFromISR(cursor_button_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 26);
}

void pause_button_isr(void) {
  fprintf(stderr, "I am in pause function ISR\n");
  xSemaphoreGiveFromISR(pause_button_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 25);
}

void play_previous_isr(void) {
  fprintf(stderr, "I am in previous function ISR\n");
  xSemaphoreGiveFromISR(previous_button_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 1);
}

void play_next_isr(void) {
  fprintf(stderr, "I am in next function ISR\n");
  xSemaphoreGiveFromISR(next_button_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 0);
}

void setting_button_isr(void) {
  fprintf(stderr, "i am in setting button isr\n");
  xSemaphoreGiveFromISR(setting_button_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 16);
}

void setting_control_down_isr(void) {
  fprintf(stderr, "I am in vol down function ISR\n");
  xSemaphoreGiveFromISR(setting_control_down_pressed, NULL);
  // LPC_GPIOINT->IO2IntClr = (1 << 5);
  LPC_GPIOINT->IO0IntClr = (1 << 29);
}

void setting_control_up_isr(void) {
  fprintf(stderr, "I am in vol up function ISR\n");
  xSemaphoreGiveFromISR(setting_control_up_pressed, NULL);
  // LPC_GPIOINT->IO2IntClr = (1 << 6);
  LPC_GPIOINT->IO0IntClr = (1 << 30);
}

void menu_button_isr(void) {
  fprintf(stderr, "I am in menu ISR\n");
  xSemaphoreGiveFromISR(menu_button_pressed, NULL);
  LPC_GPIOINT->IO0IntClr = (1 << 22);
}

void mp3_play_previous(void *p) {
  while (1) {
    if (xSemaphoreTake(previous_button_pressed, portMAX_DELAY)) {
      fprintf(stderr, "I am in previous function \n");
      if (list_of_songs_index > 0) {
        list_of_songs_index--;

        xQueueSend(Q_songname, song_list__get_name_for_item(list_of_songs_index), 0);
        fprintf(stderr, "Song: %s \n", song_list__get_name_for_item(list_of_songs_index));

      } else {
        fprintf(stderr, "Gone out of the scope for MP3 Song List \n");
      }
    }
  }
}

void mp3_play_next(void *p) {
  while (1) {
    if (xSemaphoreTake(next_button_pressed, portMAX_DELAY)) {
      fprintf(stderr, "I am in next function \n");
      if (list_of_songs_index < song_list__get_item_count() - 1) {
        list_of_songs_index++;

        xQueueSend(Q_songname, song_list__get_name_for_item(list_of_songs_index), 0);
        fprintf(stderr, "Song: %s \n", song_list__get_name_for_item(list_of_songs_index));

      } else {
        fprintf(stderr, "Gone out of the scope for MP3 Song List \n");
      }
    }
  }
}

void pause_function(void) {
  while (1) {
    if (xSemaphoreTake(pause_button_pressed, portMAX_DELAY)) {
      if (pause) {
        fprintf(stderr, "I am in pause function \n");
        vTaskSuspend(player_handle);
        pause = false;
      } else {
        vTaskResume(player_handle);
        pause = true;
      }
    } else {
      fprintf(stderr, "Fail to get semaphore signal from ISR \n");
    }
  }
}

void menu_cursor(void) {
  while (1) {
    if (xSemaphoreTake(cursor_button_pressed, portMAX_DELAY)) {
      fprintf(stderr, "I am in menu cursor \n");
      if (list_of_songs_index < song_list__get_item_count() - 1) {
        list_of_songs_index++;
        print_cursor_on_lcd(list_of_songs_index);
      } else {
        list_of_songs_index = 0;
        print_cursor_on_lcd(list_of_songs_index);
      }
      fprintf(stderr, "Song List index: %d \n", list_of_songs_index);
    }
  }
}

void play_song(void) {
  while (1) {
    if (xSemaphoreTake(select_button_pressed, portMAX_DELAY)) {
      xQueueSend(Q_songname, song_list__get_name_for_item(list_of_songs_index), 0);
    }
  }
}

typedef struct {
  char title[32];
  char artist[32];
} current_song;

void mp3_meta_display(char *song_meta) {
  current_song info = {0};
  int count_name = 0;
  int count_artist = 0;
  for (int i = 0; i < 128; i++) {
    if (i > 2 && i < 33) {
      info.title[count_name++] = (int)(song_meta[i]);
    } else if (i > 32 && i < 63) {
      info.artist[count_artist++] = (int)(song_meta[i]);
    }
  }
  lcd_clear_display();

  uart_write(0xFE);
  uart_write(0x80);
  lcd_write_string("Title: ");
  lcd_write_string(info.title);

  uart_write(0xFE);
  uart_write(0xC0);
  lcd_write_string("Artist: ");
  lcd_write_string(info.artist);
}

void mp3_reader_task(void *p) {
  songname_t songname = {};
  while (1) {
    fprintf(stderr, "Repeat \n");
    if (xQueueReceive(Q_songname, songname, portMAX_DELAY)) {
      printf("Received Song: %s\n", songname);
    }
    fprintf(stderr, "after\n");

    const char *filename = songname;
    FIL file;
    songdata_t buffer = {};
    UINT num_of_bytes_read;
    char song_meta[128];

    FRESULT result = f_open(&file, filename, FA_READ); // open

    if (FR_OK == result) {
      f_lseek(&file, f_size(&file) - (sizeof(char) * 128));
      f_read(&file, song_meta, sizeof(song_meta), &num_of_bytes_read);
      mp3_meta_display(song_meta);
      f_lseek(&file, 0);
      f_close(&file);

      FRESULT result = f_open(&file, filename, FA_READ);

      f_read(&file, buffer, sizeof(buffer), &num_of_bytes_read);
      while (num_of_bytes_read != 0) {
        f_read(&file, buffer, sizeof(buffer), &num_of_bytes_read);
        xQueueSend(Q_songdata, buffer, portMAX_DELAY);
        if (uxQueueMessagesWaiting(Q_songname)) {
          break;
        }
      }

      f_close(&file);
    } else {
      fprintf(stderr, "Error in opening file %s\n", songname);
    }
  }
}

void mp3_player_task(void *p) {
  int counter = 1;
  songdata_t songdata = {};
  while (1) {
    if (xQueueReceive(Q_songdata, &songdata[0], portMAX_DELAY)) {
      for (int i = 0; i < 512; i++) {
        while (!stream_buffer_full) {
          vTaskDelay(1);
        }
        mp3_ssp_write(songdata[i]);
      }
    }
    // printf("buffer transmit time %d\n", counter);
    // counter++;
  }
}

uint16_t current_volume = 0x10;
uint8_t current_treble = 0x01;
uint8_t current_bass = 0x09;
uint8_t volume_counter = 4; // 1 to 5
uint8_t bass_counter = 3;   // 1 to 5
uint8_t treble_counter = 3; // 1 to 5

void settings_control(void) {
  while (1) {
    if (xSemaphoreTake(setting_button_pressed, portMAX_DELAY)) {
      if (setting_counter < 2) {
        setting_counter++;
      } else {
        setting_counter = 0;
      }
      fprintf(stderr, "setting counter: %i\n", setting_counter);
    }
  }
}

void setting_control_up(void) {
  while (1) {
    if (xSemaphoreTake(setting_control_up_pressed, portMAX_DELAY)) {
      if (setting_counter == 0) {
        volume_up();
        fprintf(stderr, "volume up.\n");
      } else if (setting_counter == 1) {
        bass_up();
        fprintf(stderr, "bass up.\n");
      } else {
        treble_up();
        fprintf(stderr, "treble up.\n");
      }
    }
  }
}

void setting_control_down(void) {
  while (1) {
    if (xSemaphoreTake(setting_control_down_pressed, portMAX_DELAY)) {
      if (setting_counter == 0) {
        volume_down();
        fprintf(stderr, "volume down.\n");
      } else if (setting_counter == 1) {
        bass_down();
        fprintf(stderr, "bass down.\n");
      } else {
        treble_down();
        fprintf(stderr, "treble down.\n");
      }
    }
  }
}

void volume_up(void) {
  if (volume_counter == 5) {
    fprintf(stderr, "bass max count.\n");

  } else {
    current_volume = current_volume - 0x10;
    sci_write_reg(0x0B, current_volume, current_volume);
    volume_counter++;
  }
  volume_up_print(volume_counter);
}

void volume_down(void) {
  if (volume_counter == 0) {
    fprintf(stderr, "bass min count.\n");

  } else {
    current_volume = current_volume + 0x10;
    sci_write_reg(SCI_VOL_ADDR, current_volume, current_volume);
    volume_counter--;
  }
  volume_down_print(volume_counter);
}

void bass_up(void) {
  if (bass_counter == 5) {
    fprintf(stderr, "bass max count.\n");

  } else {
    current_bass = current_bass - 0x03;
    uint16_t original_bass = sci_read_reg(SCI_BASS_ADDR);
    sci_write_reg(SCI_BASS_ADDR, (original_bass >> 8), current_bass);
    bass_counter++;
  }
  bass_up_print(bass_counter);
}

void bass_down(void) {
  if (bass_counter == 1) {
    fprintf(stderr, "bass min count.\n");

  } else {
    current_bass = current_bass + 0x03;
    uint16_t original_bass = sci_read_reg(SCI_BASS_ADDR);
    sci_write_reg(SCI_BASS_ADDR, (original_bass >> 8), current_bass);
    bass_counter--;
  }
  bass_down_print(bass_counter);
}

void treble_up(void) {
  if (treble_counter == 5) {
    fprintf(stderr, "bass max count.\n");

  } else {
    current_treble = current_treble - 0x03;
    uint16_t original_treble = sci_read_reg(SCI_BASS_ADDR);
    sci_write_reg(SCI_BASS_ADDR, current_treble, (original_treble & 0xFF));
    treble_counter++;
  }
  treble_up_print(treble_counter);
}

void treble_down(void) {
  if (treble_counter == 1) {
    fprintf(stderr, "bass min count.\n");

  } else {
    current_treble = current_treble + 0x03;
    uint16_t original_treble = sci_read_reg(SCI_BASS_ADDR);
    sci_write_reg(SCI_BASS_ADDR, current_treble, (original_treble & 0xFF));
    treble_counter--;
  }
  treble_down_print(treble_counter);
}

void mp3_menu(void) {
  while (1) {
    if (xSemaphoreTake(menu_button_pressed, portMAX_DELAY)) {
      lcd_clear_display();
      lcd_set_cursor_to_line();
      lcd_show_all_songs();
      fprintf(stderr, "Entered menu\n");
    }
  }
}

void main(void) {
  select_button_pressed = xSemaphoreCreateBinary();
  cursor_button_pressed = xSemaphoreCreateBinary();
  pause_button_pressed = xSemaphoreCreateBinary();
  previous_button_pressed = xSemaphoreCreateBinary();
  next_button_pressed = xSemaphoreCreateBinary();
  setting_button_pressed = xSemaphoreCreateBinary();
  setting_control_up_pressed = xSemaphoreCreateBinary();
  setting_control_down_pressed = xSemaphoreCreateBinary();
  menu_button_pressed = xSemaphoreCreateBinary();

  button.select = gpio__construct_as_input(0, 6);
  button.cursor = gpio__construct_as_input(0, 26);
  button.pause = gpio__construct_as_input(0, 25); // PAUSE Function
  button.previous = gpio__construct_as_input(0, 1);
  button.next = gpio__construct_as_input(0, 0); // NEXT Function
  button.setting = gpio__construct_as_input(0, 16);
  button.setting_control_down = gpio__construct_as_input(0, 29);
  button.setting_control_up = gpio__construct_as_input(0, 30);
  button.menu = gpio__construct_as_input(0, 22);

  mp3_startup();

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, "name");
  gpio0__attach_interrupt(6, GPIO_INTR__RISING_EDGE, select_button_isr);
  gpio0__attach_interrupt(26, GPIO_INTR__RISING_EDGE, cursor_button_isr);
  gpio0__attach_interrupt(25, GPIO_INTR__RISING_EDGE, pause_button_isr);
  gpio0__attach_interrupt(1, GPIO_INTR__RISING_EDGE, play_previous_isr);
  gpio0__attach_interrupt(0, GPIO_INTR__RISING_EDGE, play_next_isr);
  gpio0__attach_interrupt(16, GPIO_INTR__RISING_EDGE, setting_button_isr);
  gpio0__attach_interrupt(29, GPIO_INTR__RISING_EDGE, setting_control_down_isr);
  gpio0__attach_interrupt(30, GPIO_INTR__RISING_EDGE, setting_control_up_isr);
  gpio0__attach_interrupt(22, GPIO_INTR__RISING_EDGE, menu_button_isr);

  song_list__populate();

  uart_init(clock__get_peripheral_clock_hz(), 9600);
  lcd_init();
  // lcd_print_upper_to_lower_songs(song_list_upperbound, song_list_lowerbound); // new

  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, sizeof(songdata_t));
  Q_songlist = xQueueCreate(1, sizeof(int));

  xTaskCreate(mp3_reader_task, "read task", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "play task", (512U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, &player_handle);

  xTaskCreate(menu_cursor, "menu cursor", (128U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(play_song, "play song", (128U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(pause_function, "pause", (128U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  xTaskCreate(mp3_play_previous, "play previous", (128U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_play_next, "play next", (128U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  xTaskCreate(settings_control, "settings", (128U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(setting_control_up, "settings up", (128U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(setting_control_down, "settings down", (128U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  // xTaskCreate(mp3_menu, "menu", (128U * 4) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  sj2_cli__init();
  vTaskStartScheduler();
}
