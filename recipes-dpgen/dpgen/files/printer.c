#include "printer.h"

static const char* msg_type_text[4] = {"INFO", "WARNING", "ERROR", "DEBUG"};
static int msg_type_base_color[4] = {2, 3, 1, 5};

void print_msg(msg_type type, char *msg)
{
  msg_buf[MSG_BUF_MAX] = '\0'; // Ensure the last char is null.
  fprintf(stderr, "\033[1;3%dm[%7s]\033[0m %s\n", msg_type_base_color[type],
    msg_type_text[type], msg);
}
