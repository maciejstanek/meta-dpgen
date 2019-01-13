#include "pattern.h"

#include <string.h>
#include <stdlib.h>

#include "printer.h"

static bool pattern[PATTERN_MAX];
static int pattern_len = 0;

int get_pattern_len() {
  return pattern_len;
}

bool get_pattern_at(int index) {
  return pattern[index];
}

int load_pattern(const char *file_name) {
  FILE *f = fopen(file_name, "r");
  if (!f) {
    snprintf(msg_buf, MSG_BUF_MAX, "Could not open '%s' for reading.", file_name);
    print_msg(MSG_ERROR, msg_buf);
    return EXIT_FAILURE;
  }
  char line[LINE_MAX];
  char line_count = 0;
  while (fgets(line, sizeof line, f)) {
    if (line[0] == '#') {
      continue; // Skip comments
    }
    char c;
    int ci = 0;
    while (c = line[ci++]) { // While not null at the end
      if (c == '0' || c == '1') {
        pattern[pattern_len++] = c - '0';
      }
    }
    line_count++;
  }
  if (!feof(f)) {
    snprintf(msg_buf, MSG_BUF_MAX, "Reading '%s' failed at line %d.", file_name, line_count);
    print_msg(MSG_ERROR, msg_buf);
    return EXIT_FAILURE;
  }
  if (fclose(f)) {
    snprintf(msg_buf, MSG_BUF_MAX, "Could not close file '%s' after reading.", file_name);
    print_msg(MSG_WARNING, msg_buf);
  }
  return EXIT_SUCCESS;
}

void print_pattern(int chars_per_line) {
  int msg_buf_offset = 0;
  for (int i = 0; i < pattern_len; i++) {
    msg_buf_offset += sprintf(msg_buf + msg_buf_offset, "%d ", pattern[i]);
    if (i % chars_per_line == chars_per_line - 1) {
      msg_buf_offset = 0;
      print_msg(MSG_DEBUG, msg_buf);
    }
  }
  if (msg_buf > 0) {
    print_msg(MSG_DEBUG, msg_buf);
  }
}
