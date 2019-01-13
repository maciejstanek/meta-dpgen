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
#include "mraa/gpio.h"
#include "printer.h"
#include "pattern.h"
#include "arg_parser.h"

static config_t config;

// OUTPUT CONTROL /////////////////////////////////////////////////////////////

static mraa_gpio_context clk_pin = NULL;
static mraa_gpio_context output_pin = NULL;
static bool clk_pin_value = 0;

static void set_output_pin_value(bool value) {
  if (mraa_gpio_write(output_pin, value) != MRAA_SUCCESS) {
    print_msg(MSG_WARNING, "Setting output value failed.");
  }
}

static void toggle_clock() {
  if (config.has_clk_pin) {
    clk_pin_value = !clk_pin_value;
    if (mraa_gpio_write(clk_pin, clk_pin_value) != MRAA_SUCCESS) {
      print_msg(MSG_WARNING, "Toggling synchronization clock failed.");
    }
  }
}

static void initialize_mraa(config_t *config)
{
  snprintf(msg_buf, MSG_BUF_MAX, "Using 'mraa' %s (%s detected).",
    mraa_get_version(), mraa_get_platform_name());
  print_msg(MSG_INFO, msg_buf);
  mraa_init();
  if (!(output_pin = mraa_gpio_init(config->output_pin_number))) {
    print_msg(MSG_WARNING, "Initializing output pin failed.");
  }
  else if (mraa_gpio_dir(output_pin, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
    print_msg(MSG_WARNING, "Forcing output pin direction failed.");
  }
  if (config->has_clk_pin) {
    if (!(clk_pin = mraa_gpio_init(config->clk_pin_number))) {
      print_msg(MSG_WARNING, "Initializing synchronization clock pin failed.");
    }
    else if (mraa_gpio_dir(clk_pin, MRAA_GPIO_OUT) != MRAA_SUCCESS) {
      print_msg(MSG_WARNING, "Forcing synchronization clock pin direction failed.");
    }
  }
}

static void cleanup_mraa()
{
  if (mraa_gpio_close(output_pin) != MRAA_SUCCESS) {
    print_msg(MSG_WARNING, "Closing output pin failed.");
  }
  if (config.has_clk_pin) {
    if (mraa_gpio_close(clk_pin) != MRAA_SUCCESS) {
      print_msg(MSG_WARNING, "Closing synchronization clock pin failed.");
    }
  }
  mraa_deinit();
}

// GENERATOR FUNCTIONS ////////////////////////////////////////////////////////

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

static void finalize(int exit_code)
{
  print_msg(MSG_INFO, "Exiting.");
  cleanup_mraa();
  exit(exit_code);
}

static void sigint_response(int _ignored)
{
  printf("\n"); // Add a new line so that "^C" is in a separate line.
  finalize(EXIT_SUCCESS);
}

static void initialize(int argc, char *argv[], struct timespec *ts, config_t *config)
{
  signal(SIGINT, sigint_response);
  parse_args(argc, argv, config);
  if (load_pattern(config->file_name)) {
    exit(EXIT_FAILURE);
  }
  if (config->debug) {
    print_pattern(30);
  }
  initialize_mraa(config);
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

#define MINIMAL_PERIOD_FOR_LOGGING 100000000 // 100 [ms]
bool printed_minimal_period_warning = false;

static void log_action(config_t *config, bool value)
{
  if (config->period >= MINIMAL_PERIOD_FOR_LOGGING) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    snprintf(msg_buf, MSG_BUF_MAX, "%d.%09d -> %d", (int)ts.tv_sec,
      (int)ts.tv_nsec, value);
    print_msg(MSG_DEBUG, msg_buf);
  }
  else {
    if (!printed_minimal_period_warning) {
      print_msg(MSG_DEBUG, "The requested frequency is too high to enable safe"
        " actions logging. It can be 10 [Hz] max.");
      printed_minimal_period_warning = true;
    }
  }
}

static void update_output(struct timespec *ts, config_t *config, int *index)
{
  bool do_toggle_clock = (*index == 0); // Do it after writing to output to eliminate jitter.
  bool value = get_pattern_at((*index)++);
  set_output_pin_value(value);
  if (do_toggle_clock) {
    toggle_clock();
  }
  if (config->debug) {
    log_action(config, value);
  }
  sleep_until(ts, config->period);
  if (*index >= get_pattern_len()) {
    if (config->repeat) {
      *index = 0;
      if (config->debug && config->period >= MINIMAL_PERIOD_FOR_LOGGING) {
        print_msg(MSG_DEBUG, "Repeating pattern.");
      }
    }
    else {
      finalize(EXIT_SUCCESS);
    }
  }
}

static void loop(struct timespec *ts, config_t *config)
{
  print_msg(MSG_INFO, "Press Ctrl-C to exit.");
  int pattern_index = 0;
  for (;;) {
    update_output(ts, config, &pattern_index);
  }
}

// MAIN ROUTINE ///////////////////////////////////////////////////////////////

int
main(int argc, char *argv[])
{
  struct timespec ts;

  initialize(argc, argv, &ts, &config);
  loop(&ts, &config);
}
