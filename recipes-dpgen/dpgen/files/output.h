#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include <stdbool.h>

#include "arg_parser.h"

void set_output_pin_value(bool value);
void toggle_clock();
void initialize_mraa(config_t *c);
void cleanup_mraa();

#endif
