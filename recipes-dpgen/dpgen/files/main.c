#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdbool.h>
#include "mraa.h"

// MESSAGE PRINTER ////////////////////////////////////////////////////////////

#define MSG_BUF_MAX 1000
static char msg_buf[MSG_BUF_MAX];
static const char* msg_type_text[4] = {"INFO", "WARNING", "ERROR", "DEBUG"};
static int msg_type_base_color[4] = {2, 3, 1, 5};

typedef enum {
  MSG_INFO = 0,
  MSG_WARNING = 1,
  MSG_ERROR = 2,
  MSG_DEBUG = 3,
} msg_type;

static void print_msg(msg_type type, char *msg)
{
  msg_buf[MSG_BUF_MAX] = '\0'; // Ensure the last char is null.
  fprintf(stderr, "\033[1;3%dm[%7s]\033[0m %s\n", msg_type_base_color[type],
    msg_type_text[type], msg);
}

// PATTERN LOADER /////////////////////////////////////////////////////////////

#define PATTERN_MAX 2000
#define LINE_MAX 200
static bool pattern[PATTERN_MAX];
static int pattern_len = 0;

static int load_pattern(const char *file_name) {
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
    print_msg(MSG_ERROR, msg_buf);
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

static void print_pattern(int chars_per_line) {
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

// GENERATOR FUNCTIONS ////////////////////////////////////////////////////////

static void initialize_mraa(void)
{
  snprintf(msg_buf, MSG_BUF_MAX, "Using 'mraa' %s (%s detected).",
    mraa_get_version(), mraa_get_platform_name());
  print_msg(MSG_INFO, msg_buf);
  mraa_init();
}

static void configure_real_time(void)
{
  // Lock memory to ensure no swapping is done.
  if (mlockall(MCL_FUTURE | MCL_CURRENT)) {
    print_msg(MSG_WARNING, "Failed to lock memory.");
  }

  // Set our thread to real time priority
  struct sched_param sp;
  sp.sched_priority = 30;
  if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp)) {
    print_msg(MSG_WARNING, "Failed to enable real-time priority.");
  }
}

static void print_help(char *argv0) {
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
  fprintf(stderr, "  -d               Print debug information.\n");
  fprintf(stderr, "  -h               Print this help message.\n");
  // TODO: Output pin selection?
  fprintf(stderr, "\n");
  fprintf(stderr, "The pattern FILE contains a sequence of ones and zeros.\n");
  fprintf(stderr, "All other characters are ignored. Example pattern FILE(s)\n");
  fprintf(stderr, "are located at '/usr/share/dpgen/pattern'.\n");
}

typedef struct config_t {
  bool repeat;
  bool debug;
  char *file_name;
  int period; // [ns]
} config_t;

static void parse_args(int argc, char *argv[], config_t *config)
{
  config->period = 1000000000; // 10^9 [ns] = 1 [s]
  config->repeat = false;
  config->debug = false;
  bool has_f = false, has_t = false;
  double f;
  int opt;
  while ((opt = getopt(argc, argv, "hdrf:t:")) != -1) {
    switch (opt) {
    case 'r':
      config->repeat = true;
      break;
    case 'd':
      config->debug = true;
      break;
    case 't':
      has_t = true;
      config->period = atoi(optarg);
      break;
    case 'f':
      has_f = true;
      f = atof(optarg);
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
    config->period = (int)(1/(f/1000000000)); // 10^9
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

static void finalize(int _ignored)
{
  printf("\n"); // Add a new line so that "^C" is in a separate line.
  print_msg(MSG_INFO, "Finalizing.");
  // TODO: free any allocated memory (patterns?)
  mraa_deinit();
  exit(EXIT_SUCCESS);
}


static void initialize(int argc, char *argv[], struct timespec *ts, config_t *config)
{
  signal(SIGINT, finalize);
  parse_args(argc, argv, config);
  if (load_pattern(config->file_name)) {
    exit(EXIT_FAILURE);
  }
  if (config->debug) {
    print_pattern(30);
  }
  // TODO
  initialize_mraa();
  clock_gettime(CLOCK_MONOTONIC, ts);
}

static void sleep_until(struct timespec *ts, int delay)
{
  ts->tv_nsec += delay;
  while (ts->tv_nsec >= 1000000000) { // 10^9
    ts->tv_nsec -= 1000000000; // 10^9
    ts->tv_sec++;
  }
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ts, NULL);
}

static void logtimestamp(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  printf("%d.%09d\n", (int)ts.tv_sec, (int)ts.tv_nsec);
}

static void loop(struct timespec *ts, config_t *config)
{
  print_msg(MSG_INFO, "Press Ctrl-C to exit.");
  for (;;) {
    sleep_until(ts, config->period);
    /* logtimestamp(); */
  }
}

// MAIN ROUTINE ///////////////////////////////////////////////////////////////

int
main(int argc, char *argv[])
{
  struct timespec ts;
  config_t config;

  initialize(argc, argv, &ts, &config);
  loop(&ts, &config);
}
