#ifndef _PATTERN_H_
#define _PATTERN_H_

#include <stdbool.h>

#define PATTERN_MAX 2000
#define LINE_MAX 200

int get_pattern_len();
bool get_pattern_at(int index);
int load_pattern(const char *file_name);
void print_pattern(int chars_per_line);

#endif
