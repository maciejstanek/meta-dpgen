#include "output.h"

#include <string.h>

#include "mraa.h"
#include "mraa/gpio.h"
#include "printer.h"

static mraa_gpio_context clk_pin = NULL;
static mraa_gpio_context output_pin = NULL;
static bool clk_pin_value = 0;
static config_t *config = NULL;

void set_output_pin_value(bool value) {
  if (mraa_gpio_write(output_pin, value) != MRAA_SUCCESS) {
    print_msg(MSG_WARNING, "Setting output value failed.");
  }
}

void toggle_clock() {
  if (config->has_clk_pin) {
    clk_pin_value = !clk_pin_value;
    if (mraa_gpio_write(clk_pin, clk_pin_value) != MRAA_SUCCESS) {
      print_msg(MSG_WARNING, "Toggling synchronization clock failed.");
    }
  }
}

void initialize_mraa(config_t *c)
{
  config = c;
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

void cleanup_mraa()
{
  if (mraa_gpio_close(output_pin) != MRAA_SUCCESS) {
    print_msg(MSG_WARNING, "Closing output pin failed.");
  }
  if (config->has_clk_pin) {
    if (mraa_gpio_close(clk_pin) != MRAA_SUCCESS) {
      print_msg(MSG_WARNING, "Closing synchronization clock pin failed.");
    }
  }
  mraa_deinit();
}
