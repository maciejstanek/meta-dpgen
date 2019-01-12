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
  snprintf(msg_buf, MSG_BUF_SIZE, "Using 'mraa' %s (%s detected)\n",
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

static void parse_args(int argc, char* argv[])
{
  int flags, opt;
  int nsecs, tfnd;

  nsecs = 0;
  tfnd = 0;
  flags = 0;
  while ((opt = getopt(argc, argv, "nt:")) != -1) {
    switch (opt) {
    case 'n':
      flags = 1;
      break;
    case 't':
      nsecs = atoi(optarg);
      tfnd = 1;
      break;
    default: /* '?' */
      fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
        argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  printf("flags=%d; tfnd=%d; nsecs=%d; optind=%d\n",
          flags, tfnd, nsecs, optind);

  if (optind >= argc) {
      fprintf(stderr, "Expected argument after options\n");
      exit(EXIT_FAILURE);
  }

  printf("name argument = %s\n", argv[optind]);
}

static void finalize(int _ignored)
{
  print_msg(MSG_INFO, "Finalizing.");
  // TODO: deinit mraa and free memory (samples memory)
  mraa_deinit();
  exit(MRAA_SUCCESS);
}


static void initialize(int argc, char* argv[])
{
  signal(SIGINT, finalize);
  parse_args(argc, argv); // pass config via a struct
  initialize_mraa();
}

static void sleep_until(struct timespec *ts, int delay)
{
  ts->tv_nsec += delay;
  if (ts->tv_nsec >= 1000*1000*1000) {
    ts->tv_nsec -= 1000*1000*1000;
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

// MAIN ROUTINE ///////////////////////////////////////////////////////////////

int
main(int argc, char* argv[])
{
  initialize(argc, argv); // Probably pass parameters here from parsing args
  unsigned int delay = 1000*1000; // Note: Delay in ns, will be an input arg

  print_msg(MSG_WARNING, "Test warning");
  print_msg(MSG_ERROR, "Test error");

  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);

  print_msg(MSG_INFO, "Press Ctrl-C to exit.");
  for (;;) {
    sleep_until(&ts, delay);
    logtimestamp();
  }
}
