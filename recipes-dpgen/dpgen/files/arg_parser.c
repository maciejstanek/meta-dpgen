#include "arg_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "printer.h"

void print_help(char *argv0) {
  fprintf(stderr, "Usage: %s [OPTION...] FILE\n", argv0);
  fprintf(stderr, "Generate a pattern defined in FILE with a GPIO.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "OPTION(s):\n");
  fprintf(stderr, "  -f <frequency>   The frequency of the pattern in [Hz].\n");
  fprintf(stderr, "                   Defaults to 1[Hz]. Mutually exclusive with -t.\n");
  fprintf(stderr, "  -t <period>      The period of the pattern in [ns].\n");
  fprintf(stderr, "                   Defaults to 1[s]. Mutually exclusive with -f.\n");
  fprintf(stderr, "  -r               Repeat pattern indefinitely. In case\n");
  fprintf(stderr, "                   it is not present, the pattern is sent\n");
  fprintf(stderr, "                   only once.\n");
  fprintf(stderr, "  -o <pin number>  Output pin number. Defaults to 2.\n");
  fprintf(stderr, "  -s <pin number>  Synchronization clock pin number. Disabled\n");
  fprintf(stderr, "                   if option not provided.\n");
  fprintf(stderr, "  -d               Print debug information.\n");
  fprintf(stderr, "  -h               Print this help message.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "The pattern FILE contains a sequence of ones and zeros.\n");
  fprintf(stderr, "All other characters are ignored. Example pattern FILE(s)\n");
  fprintf(stderr, "are located at '/usr/share/dpgen/pattern'.\n");
}

void parse_args(int argc, char *argv[], config_t *config)
{
  config->period = 1000000000; // 10^9 [ns] = 1 [s]
  config->repeat = false;
  config->debug = false;
  config->has_clk_pin = false;
  config->output_pin_number = 2; // Arduino D2
  bool has_f = false, has_t = false, has_s = false;
  long double f;
  int opt;
  while ((opt = getopt(argc, argv, "hdrf:t:o:s:")) != -1) {
    switch (opt) {
    case 'r':
      config->repeat = true;
      break;
    case 'd':
      config->debug = true;
      break;
    case 'o':
      config->output_pin_number = atoi(optarg);
      break;
    case 's':
      config->has_clk_pin = true;
      config->clk_pin_number = atoi(optarg);
      break;
    case 't':
      has_t = true;
      config->period = atoi(optarg);
      break;
    case 'f':
      has_f = true;
      f = atof(optarg);
      printf("DBG: %Lf\n", f);
      break;
    case 'h':
      print_help(argv[0]);
      exit(EXIT_SUCCESS);
    default:
      print_help(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if (has_t && has_f) {
    print_msg(MSG_ERROR, "Options -f and -t are mutually exclusive.");
    print_help(argv[0]);
    exit(EXIT_FAILURE);
  }
  if (!has_t && has_f) {
    if (f == 0) {
      print_msg(MSG_ERROR, "Frequency cannot be zero.");
      print_help(argv[0]);
      exit(EXIT_FAILURE);
    }
    config->period = (int)(1.0L/(f/1000000000.0L)); // 10^9
  }
  if (config->period < (int)(1000000000.0L/230.0L)) {
    print_msg(MSG_WARNING, "According to Intel, GPIO max speed is 230 [Hz].");
  }

  if (config->has_clk_pin) {
    if (config->clk_pin_number == config->output_pin_number) {
      print_msg(MSG_ERROR, "Options -o and -s cannot be equal.");
      print_help(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  if (optind >= argc) {
    print_msg(MSG_ERROR, "Expected argument after options.");
    print_help(argv[0]);
    exit(EXIT_FAILURE);
  }
  config->file_name = argv[optind];

  if (config->debug) {
    print_msg(MSG_DEBUG, "Using the following configuration:");
    snprintf(msg_buf, MSG_BUF_MAX, "  * repeat: %s", config->repeat ? "yes" : "no");
    print_msg(MSG_DEBUG, msg_buf);
    snprintf(msg_buf, MSG_BUF_MAX, "  * period: %d [ns]", config->period);
    print_msg(MSG_DEBUG, msg_buf);
    snprintf(msg_buf, MSG_BUF_MAX, "  * file name: %s", config->file_name);
    print_msg(MSG_DEBUG, msg_buf);
  }
}
