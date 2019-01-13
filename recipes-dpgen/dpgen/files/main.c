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

#define MSG_BUF_SIZE 1000
static char msg_buf[MSG_BUF_SIZE];
static const char* msg_type_text[3] = {"INFO", "WARNING", "ERROR"};
static int msg_type_base_color[3] = {2, 3, 1};

typedef enum {
  MSG_INFO = 0,
  MSG_WARNING = 1,
  MSG_ERROR = 2,
} msg_type;

static void print_msg(msg_type type, char *msg)
{
  fprintf(stderr, "\033[1;3%dm[%7s]\033[0m %s\n", msg_type_base_color[type],
    msg_type_text[type], msg);
}

// GENERATOR FUNCTIONS ////////////////////////////////////////////////////////

static void initialize_mraa(void)
{
  snprintf(msg_buf, MSG_BUF_SIZE, "Using 'mraa' %s (%s detected).",
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
    print_msg(MSG_WARNING, "[WARNING] Failed to set stepper thread "
      "to real-time priority.");
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
  fprintf(stderr, "  -h               Print this help message.\n");
  // TODO: Output pin selection?
  fprintf(stderr, "\n");
  fprintf(stderr, "The pattern FILE contains a sequence of ones and zeros.\n");
  fprintf(stderr, "All other characters are ignored. Example pattern FILE(s)\n");
  fprintf(stderr, "are located at '/usr/share/dpgen/pattern'.\n");
}

typedef struct config_t {
  bool repeat;
  char *file_name;
  int period; // [ns]
} config_t;

static void parse_args(int argc, char *argv[], config_t *config)
{
  config->period = 1000000000; // 10^9 [ns] = 1 [s]
  config->repeat = false;
  bool has_f = false, has_t = false;
  double f;
  int opt;
  while ((opt = getopt(argc, argv, "hrf:t:")) != -1) {
    switch (opt) {
    case 'r':
      config->repeat = true;
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

  print_msg(MSG_INFO, "Using the following configuration:");
  snprintf(msg_buf, MSG_BUF_SIZE, "  * repeat: %s", config->repeat ? "yes" : "no");
  print_msg(MSG_INFO, msg_buf);
  snprintf(msg_buf, MSG_BUF_SIZE, "  * period: %d [ns]", config->period);
  print_msg(MSG_INFO, msg_buf);
  snprintf(msg_buf, MSG_BUF_SIZE, "  * file name: %s", config->file_name);
  print_msg(MSG_INFO, msg_buf);
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
  // TODO: Load pattern
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
