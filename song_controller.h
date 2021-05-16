#pragma once

#include "ff.h"
#include <stddef.h> // size_t get size of any object
#include <stdio.h>
#include <string.h>

typedef char song_memory_t[128];

void song_list__populate(void);
size_t song_list__get_item_count(void);
const char *song_list__get_name_for_item(size_t item_number);
void song_list__handle_filename(const char *filename);