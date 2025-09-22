
#include "sdmmc_driver.h"
#include "dirent.h"
#include "nvs.h"
#include "nvs_flash.h"

#define IIC_HOST I2C_NUM_0

#define EXAMPLE_MAX_CHAR_SIZE 64

static const char* TAG = "SDMMC";

#define MOUNT_POINT "/sd"

#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
const char* names[] = {"CLK", "CMD", "D0", "D1", "D2", "D3"};
const int pins[] = {CONFIG_EXAMPLE_PIN_CLK,
                    CONFIG_EXAMPLE_PIN_CMD,
                    CONFIG_EXAMPLE_PIN_D0
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
                    ,
                    CONFIG_EXAMPLE_PIN_D1,
                    CONFIG_EXAMPLE_PIN_D2,
                    CONFIG_EXAMPLE_PIN_D3
#endif
};

const int pin_count = sizeof(pins) / sizeof(pins[0]);

#if CONFIG_EXAMPLE_ENABLE_ADC_FEATURE
const int adc_channels[] = {CONFIG_EXAMPLE_ADC_PIN_CLK,
                            CONFIG_EXAMPLE_ADC_PIN_CMD,
                            CONFIG_EXAMPLE_ADC_PIN_D0
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
                            ,
                            CONFIG_EXAMPLE_ADC_PIN_D1,
                            CONFIG_EXAMPLE_ADC_PIN_D2,
                            CONFIG_EXAMPLE_ADC_PIN_D3
#endif
};
#endif  // CONFIG_EXAMPLE_ENABLE_ADC_FEATURE

pin_configuration_t config = {
    .names = names,
    .pins = pins,
#if CONFIG_EXAMPLE_ENABLE_ADC_FEATURE
    .adc_channels = adc_channels,
#endif
};
#endif  // CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS

static nvs_handle_t my_handle;

int32_t ParameterGetBootCount() {
  int32_t value = 0;
  nvs_get_i32(my_handle, "bcnt", &value);
  return value;
}

void ParameterManagerInit() {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  // Open a Storage Namespace
  err = nvs_open("mbi", NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    // Handle error
    ESP_LOGE(TAG, "[BACKEND] Failed to open NVS namespace.");
    return;
  }

  {
    int32_t boot_count = ParameterGetBootCount();
    ESP_LOGI(TAG, "[BACKEND] boot count %d", (int)boot_count);
    nvs_set_i32(my_handle, "bcnt", boot_count + 1);
    nvs_commit(my_handle);  // Don't forget to commit!
  }
}

void ParameterSetCurrentTab(int32_t value) {
  if (nvs_set_i32(my_handle, "tab", value) == ESP_OK) {
    nvs_commit(my_handle);  // Don't forget to commit!
  }
}

int32_t ParameterGetCurrentTab() {
  int32_t value = 0;
  nvs_get_i32(my_handle, "tab", &value);
  return value;
}

uint32_t SDCard_Size = 0;
uint32_t SDCard_Free_Size = 0;

static void print_sd_free_space_fatfs() {
  FATFS* fs;
  DWORD free_clusters;
  FRESULT res = f_getfree(MOUNT_POINT, &free_clusters, &fs);
  if (res != FR_OK) {
    ESP_LOGI(TAG, "f_getfree failed: %d", res);
    return;
  }
  SDCard_Free_Size = (uint64_t)free_clusters * fs->csize * 512 / (1024 * 1024);
  SDCard_Size = (uint64_t)(fs->n_fatent - 2) * fs->csize * 512 / (1024 * 1024);
  ESP_LOGI(TAG, "Total: %ld MB, Free: %ld MB", SDCard_Size, SDCard_Free_Size);
}

void LoadAndTestSDMMC(void) {
  esp_err_t ret;

  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
      .format_if_mount_failed = true,
#else
      .format_if_mount_failed = false,
#endif  // EXAMPLE_FORMAT_IF_MOUNT_FAILED
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};
  sdmmc_card_t* card;
  const char mount_point[] = MOUNT_POINT;
  ESP_LOGI(TAG, "Initializing SD card");

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
  // Please check its source code and implement error recovery when developing
  // production applications.

  ESP_LOGI(TAG, "Using SDMMC peripheral");

  // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
  // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
  // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();

  // For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board
  // LDO) power supply. When using specific IO pins (which can be used for ultra high-speed SDMMC)
  // to connect to the SD card and the internal LDO power supply, we need to initialize the power
  // supply first.
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
  sd_pwr_ctrl_ldo_config_t ldo_config = {
      .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
  };
  sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

  ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
    return;
  }
  host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif

  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 1;

  // On chips where the GPIOs used for SD card can be configured, set them in
  // the slot_config structure:
  slot_config.clk = GPIO_NUM_2;
  slot_config.cmd = GPIO_NUM_1;
  slot_config.d0 = GPIO_NUM_3;

  // Enable internal pullups on enabled pins. The internal pullups
  // are insufficient however, please make sure 10k external pullups are
  // connected on the bus. This is for debug / example purpose only.
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  ESP_LOGI(TAG, "Mounting filesystem");
  ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG,
               "Failed to mount filesystem. "
               "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED "
               "menuconfig option.");
    } else {
      ESP_LOGE(TAG,
               "Failed to initialize the card (%s). "
               "Make sure SD card lines have pull-up resistors in place.",
               esp_err_to_name(ret));
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
      check_sd_card_pins(&config, pin_count);
#endif
    }
    return;
  }

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card);
  print_sd_free_space_fatfs();
  ESP_LOGI(TAG, "Filesystem mounted");

  // ListAllFilesInFolder(MOUNT_POINT);
}

void ListAllFilesInFolder(const char* path) {
  DIR* dir = opendir(path);
  if (!dir) {
    ESP_LOGI(TAG, "Failed to open directory: %s", path);
    return;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

    struct stat st;
    if (stat(full_path, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        ESP_LOGI(TAG, "DIR  : %s", full_path);
        // Recursively list subdirectories
        ListAllFilesInFolder(full_path);
      } else {
        ESP_LOGI(TAG, "FILE : %s (%ld bytes)", full_path, st.st_size);
      }
    }
  }
  closedir(dir);
}

void Fall_Asleep(void) {}

void Restart(void) {
  ESP_LOGI("PWR", "[RESTART]");
  esp_restart();
}

void Shutdown(void) {
  // gpio_set_level(PWR_Control_PIN, false);
  // LCD_Backlight = 0;
}


void PowerLoop(void) {
  static int64_t last_time_ = 0;
  static uint16_t Long_Press = 0;

  if (!gpio_get_level(BOOT_KEY_Input_PIN)) {
    int64_t now_ms = esp_timer_get_time() / 1000;
    if (last_time_ == 0) {
      last_time_ = now_ms;
    }
    ESP_LOGI("PWR", "[PRESSED]");

    Long_Press += (now_ms - last_time_);
    last_time_ = now_ms;
    if (Long_Press >= 5) {
      // Restart();
    }
  }
}

void configure_GPIO(int pin, gpio_mode_t Mode) {
  gpio_reset_pin(pin);
  gpio_set_direction(pin, Mode);
}

void InitializePower(void) {
  configure_GPIO(BOOT_KEY_Input_PIN, GPIO_MODE_INPUT);

  // vTaskDelay(100);
  // if (!gpio_get_level(BOOT_KEY_Input_PIN)) {
  //   BAT_State = 1;
  //   gpio_set_level(PWR_Control_PIN, true);
  // }
}
