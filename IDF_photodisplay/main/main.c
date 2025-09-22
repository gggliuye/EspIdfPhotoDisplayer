

#include <stdio.h>
#include "axp2101_driver.h"
#include "display_sh86001.h"
#include "image_loader.h"
#include "lvgl_panel.h"
#include "sdmmc_driver.h"

static const char* TAG = "MAIN";

void app_main(void) {
  InitializeI2C();
  ParameterManagerInit();
  InitializeAXP2101();
  InitializePower();

  InitializeDisplay();
  LoadAndTestSDMMC();
  InitializeLVGL();

  InitializeImageLoader();

  ESP_LOGI(TAG, "Display LVGL demos");
  // Lock the mutex due to the LVGL APIs are not thread-safe
  if (example_lvgl_lock(-1)) {
    // initialize LVGL render page
    CreateLvglPanel();

    // lv_demo_widgets(); /* A widgets example */
    // lv_demo_music(); /* A modern, smartphone-like music player demo. */
    // lv_demo_stress();       /* A stress test for LVGL. */
    // lv_demo_benchmark(); /* A demo to measure the performance of LVGL or to compare different
    // settings. */

    // Release the mutex
    example_lvgl_unlock();
  }
}
