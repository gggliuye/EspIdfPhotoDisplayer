#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include "driver/i2c.h"
#include "driver/sdmmc_host.h"
#include "esp_io_expander_tca9554.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "sdmmc_cmd.h"

#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

// 1. BOOT: Press and then power on, and the development board enters download mode (commonly used
// when the program freezes or the USB GPIO is occupied); under normal working conditions, the GPIO0
// can detect the high and low level of the button to determine the action, and the low level is
// pressed, which can recognize click, double-click, multi-click, and long-press actions.

// 2. PWR: In the power-on state, press and hold for 6s to power off, in the power-off state (power
// off to charge the battery), click to power on; Under normal working conditions, the action can be
// judged by the high and low levels of the EXIO4 detection buttons of the extended IO, and the high
// level is pressed, which can identify the actions of single click, double click, multiple click
// and long press (long press should not exceed 6s, otherwise the power will be turned off).

#define BOOT_KEY_Input_PIN 0

extern uint32_t SDCard_Size;
extern uint32_t SDCard_Free_Size;

void LoadAndTestSDMMC(void);
void ListAllFilesInFolder(const char* path);

void InitializePower(void);
void PowerLoop(void);

#ifdef __cplusplus
extern "C" {
#endif

void ParameterManagerInit();
int32_t ParameterGetBootCount();
void ParameterSetCurrentTab(int32_t value);
int32_t ParameterGetCurrentTab();

#ifdef __cplusplus
}
#endif
