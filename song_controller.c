#include "song_controller.h"

song_memory_t list_of_songs[32];
size_t number_of_songs;

void song_list__populate(void) {
  FRESULT res;
  static FILINFO file_info;
  const char *root_path = "/";

  DIR dir;
  res = f_opendir(&dir, root_path);

  if (res == FR_OK) {
    for (;;) {
      res = f_readdir(&dir, &file_info); // Read a directory item
      if (res != FR_OK || file_info.fname[0] == 0) {
        break; // Break on error or end of dir
      }

      if (file_info.fattrib & AM_DIR) {
        // Skip nested directories, only focus on MP3 songs at the root
      } else { // It is a file.
        song_list__handle_filename(file_info.fname);
      }
    }
    f_closedir(&dir);
  }
}

void song_list__handle_filename(const char *filename) {
  if (NULL != strstr(filename, ".mp3")) {
    if (!strstr(filename, "._")) {
      strncpy(list_of_songs[number_of_songs], filename, sizeof(song_memory_t) - 1);
      ++number_of_songs;
    }
  }
}

size_t song_list__get_item_count(void) { return number_of_songs; }

const char *song_list__get_name_for_item(size_t item_number) {
  const char *return_pointer = "";

  if (item_number >= number_of_songs) {
    return_pointer = "";
  } else {
    return_pointer = list_of_songs[item_number];
  }

  return return_pointer;
}
