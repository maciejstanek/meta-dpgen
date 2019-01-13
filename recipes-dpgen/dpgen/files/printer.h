#ifndef _PRINTER_H_
#define _PRINTER_H_
#include <stdio.h>

#define MSG_BUF_MAX 1000

char msg_buf[MSG_BUF_MAX];

typedef enum {
  MSG_INFO = 0,
  MSG_WARNING = 1,
  MSG_ERROR = 2,
  MSG_DEBUG = 3,
} msg_type;

void print_msg(msg_type type, char *msg);

#endif
