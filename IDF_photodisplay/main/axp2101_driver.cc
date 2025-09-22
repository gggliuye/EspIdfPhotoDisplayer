
#include "axp2101_driver.h"
#include <stdio.h>
#include <cstring>
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"

#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"
static const char* TAG = "AXP2101";

static XPowersPMU PMU;

extern int pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t* data, uint8_t len);
extern int pmu_register_write_byte(uint8_t devAddr, uint8_t regAddr, uint8_t* data, uint8_t len);

esp_err_t pmu_init() {
  if (PMU.begin(AXP2101_SLAVE_ADDRESS, pmu_register_read, pmu_register_write_byte)) {
    ESP_LOGI(TAG, "Init PMU SUCCESS!");
  } else {
    ESP_LOGE(TAG, "Init PMU FAILED!");
    return ESP_FAIL;
  }

  // Turn off not use power channel
  PMU.disableDC2();
  PMU.disableDC3();
  PMU.disableDC4();
  PMU.disableDC5();

  PMU.disableALDO1();
  PMU.disableALDO2();
  PMU.disableALDO3();
  PMU.disableALDO4();
  PMU.disableBLDO1();
  PMU.disableBLDO2();

  PMU.disableCPUSLDO();
  PMU.disableDLDO1();
  PMU.disableDLDO2();

  // ESP32s3 Core VDD
  PMU.setDC3Voltage(3300);
  PMU.enableDC3();

  // Extern 3.3V VDD
  PMU.setDC1Voltage(3300);
  PMU.enableDC1();

  // CAM DVDD  1500~1800
  PMU.setALDO1Voltage(1800);
  // PMU.setALDO1Voltage(1500);
  PMU.enableALDO1();

  // CAM DVDD 2500~2800
  PMU.setALDO2Voltage(2800);
  PMU.enableALDO2();

  // CAM AVDD 2800~3000
  PMU.setALDO4Voltage(3000);
  PMU.enableALDO4();

  // PIR VDD 3300
  PMU.setALDO3Voltage(3300);
  PMU.enableALDO3();

  // OLED VDD 3300
  PMU.setBLDO1Voltage(3300);
  PMU.enableBLDO1();

  // MIC VDD 33000
  PMU.setBLDO2Voltage(3300);
  PMU.enableBLDO2();

  ESP_LOGI(TAG, "DCDC=======================================================================\n");
  ESP_LOGI(TAG, "DC1  : %s   Voltage:%u mV \n", PMU.isEnableDC1() ? "+" : "-", PMU.getDC1Voltage());
  ESP_LOGI(TAG, "DC2  : %s   Voltage:%u mV \n", PMU.isEnableDC2() ? "+" : "-", PMU.getDC2Voltage());
  ESP_LOGI(TAG, "DC3  : %s   Voltage:%u mV \n", PMU.isEnableDC3() ? "+" : "-", PMU.getDC3Voltage());
  ESP_LOGI(TAG, "DC4  : %s   Voltage:%u mV \n", PMU.isEnableDC4() ? "+" : "-", PMU.getDC4Voltage());
  ESP_LOGI(TAG, "DC5  : %s   Voltage:%u mV \n", PMU.isEnableDC5() ? "+" : "-", PMU.getDC5Voltage());
  ESP_LOGI(TAG, "ALDO=======================================================================\n");
  ESP_LOGI(TAG, "ALDO1: %s   Voltage:%u mV\n", PMU.isEnableALDO1() ? "+" : "-",
           PMU.getALDO1Voltage());
  ESP_LOGI(TAG, "ALDO2: %s   Voltage:%u mV\n", PMU.isEnableALDO2() ? "+" : "-",
           PMU.getALDO2Voltage());
  ESP_LOGI(TAG, "ALDO3: %s   Voltage:%u mV\n", PMU.isEnableALDO3() ? "+" : "-",
           PMU.getALDO3Voltage());
  ESP_LOGI(TAG, "ALDO4: %s   Voltage:%u mV\n", PMU.isEnableALDO4() ? "+" : "-",
           PMU.getALDO4Voltage());
  ESP_LOGI(TAG, "BLDO=======================================================================\n");
  ESP_LOGI(TAG, "BLDO1: %s   Voltage:%u mV\n", PMU.isEnableBLDO1() ? "+" : "-",
           PMU.getBLDO1Voltage());
  ESP_LOGI(TAG, "BLDO2: %s   Voltage:%u mV\n", PMU.isEnableBLDO2() ? "+" : "-",
           PMU.getBLDO2Voltage());
  ESP_LOGI(TAG, "CPUSLDO====================================================================\n");
  ESP_LOGI(TAG, "CPUSLDO: %s Voltage:%u mV\n", PMU.isEnableCPUSLDO() ? "+" : "-",
           PMU.getCPUSLDOVoltage());
  ESP_LOGI(TAG, "DLDO=======================================================================\n");
  ESP_LOGI(TAG, "DLDO1: %s   Voltage:%u mV\n", PMU.isEnableDLDO1() ? "+" : "-",
           PMU.getDLDO1Voltage());
  ESP_LOGI(TAG, "DLDO2: %s   Voltage:%u mV\n", PMU.isEnableDLDO2() ? "+" : "-",
           PMU.getDLDO2Voltage());
  ESP_LOGI(TAG, "===========================================================================\n");

  PMU.clearIrqStatus();

  PMU.enableVbusVoltageMeasure();
  PMU.enableBattVoltageMeasure();
  PMU.enableSystemVoltageMeasure();
  PMU.enableTemperatureMeasure();

  // It is necessary to disable the detection function of the TS pin on the board
  // without the battery temperature detection function, otherwise it will cause abnormal charging
  PMU.disableTSPinMeasure();

  // Disable all interrupts
  PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  // Clear all interrupt flags
  PMU.clearIrqStatus();
  // Enable the required interrupt function
  PMU.enableIRQ(
      XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |     // BATTERY
      XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |   // VBUS
      XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |      // POWER KEY
      XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ  // CHARGE
      // XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ   |   //POWER KEY
  );

  // Set the precharge charging current
  PMU.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_50MA);
  // Set constant current charge current limit
  PMU.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_200MA);
  // Set stop charging termination current
  PMU.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);

  // Set charge cut-off voltage
  PMU.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V1);

  // Read battery percentage
  ESP_LOGI(TAG, "battery percentage:%d %%", PMU.getBatteryPercent());

  // Set the watchdog trigger event type
  // PMU.setWatchdogConfig(XPOWERS_AXP2101_WDT_IRQ_TO_PIN);
  // Set watchdog timeout
  // PMU.setWatchdogTimeout(XPOWERS_AXP2101_WDT_TIMEOUT_4S);
  // Enable watchdog to trigger interrupt event
  // PMU.enableWatchdog();
  return ESP_OK;
}

int32_t GetBatteryPercent() { return PMU.getBatteryPercent(); }

bool IsCharging() { return PMU.isCharging(); }

void pmu_isr_handler() {
  // Get PMU Interrupt Status Register
  PMU.getIrqStatus();

  ESP_LOGI(TAG, "Power Temperature: %.2f°C", PMU.getTemperature());

  ESP_LOGI(TAG, "isCharging: %s", PMU.isCharging() ? "YES" : "NO");

  ESP_LOGI(TAG, "isDischarge: %s", PMU.isDischarge() ? "YES" : "NO");

  ESP_LOGI(TAG, "isStandby: %s", PMU.isStandby() ? "YES" : "NO");

  ESP_LOGI(TAG, "isVbusIn: %s", PMU.isVbusIn() ? "YES" : "NO");

  ESP_LOGI(TAG, "isVbusGood: %s", PMU.isVbusGood() ? "YES" : "NO");

  uint8_t charge_status = PMU.getChargerStatus();
  if (charge_status == XPOWERS_AXP2101_CHG_TRI_STATE) {
    ESP_LOGI(TAG, "Charger Status: tri_charge");
  } else if (charge_status == XPOWERS_AXP2101_CHG_PRE_STATE) {
    ESP_LOGI(TAG, "Charger Status: pre_charge");
  } else if (charge_status == XPOWERS_AXP2101_CHG_CC_STATE) {
    ESP_LOGI(TAG, "Charger Status: constant charge");
  } else if (charge_status == XPOWERS_AXP2101_CHG_CV_STATE) {
    ESP_LOGI(TAG, "Charger Status: constant voltage");
  } else if (charge_status == XPOWERS_AXP2101_CHG_DONE_STATE) {
    ESP_LOGI(TAG, "Charger Status: charge done");
  } else if (charge_status == XPOWERS_AXP2101_CHG_STOP_STATE) {
    ESP_LOGI(TAG, "Charger Status: not charge");
  }

  ESP_LOGI(TAG, "getBattVoltage: %d mV", PMU.getBattVoltage());

  ESP_LOGI(TAG, "getVbusVoltage: %d mV", PMU.getVbusVoltage());

  ESP_LOGI(TAG, "getSystemVoltage: %d mV", PMU.getSystemVoltage());

  if (PMU.isBatteryConnect()) {
    ESP_LOGI(TAG, "getBatteryPercent: %d %%", PMU.getBatteryPercent());
  }
  // Clear PMU Interrupt Status Register
  PMU.clearIrqStatus();
}

extern esp_err_t pmu_init();
extern void pmu_isr_handler();

static void pmu_hander_task(void*);
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR pmu_irq_handler(void* arg) {
  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void irq_init() {
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_NEGEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = PMU_INPUT_PIN_SEL;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);
  gpio_set_intr_type(PMU_INPUT_PIN, GPIO_INTR_NEGEDGE);
  // install gpio isr service
  gpio_install_isr_service(0);
  // hook isr handler for specific gpio pin
  gpio_isr_handler_add(PMU_INPUT_PIN, pmu_irq_handler, (void*)PMU_INPUT_PIN);
}

/**
 * @brief Read a sequence of bytes from a pmu registers
 */
int pmu_register_read(uint8_t devAddr, uint8_t regAddr, uint8_t* data, uint8_t len) {
  if (len == 0) {
    return ESP_OK;
  }
  if (data == NULL) {
    return ESP_FAIL;
  }
  i2c_cmd_handle_t cmd;

  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (devAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdTICKS_TO_MS(1000));
  i2c_cmd_link_delete(cmd);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "PMU i2c_master_cmd_begin FAILED! > ");
    return ESP_FAIL;
  }
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (devAddr << 1) | READ_BIT, ACK_CHECK_EN);
  if (len > 1) {
    i2c_master_read(cmd, data, len - 1, ACK_VAL);
  }
  i2c_master_read_byte(cmd, &data[len - 1], NACK_VAL);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdTICKS_TO_MS(1000));
  i2c_cmd_link_delete(cmd);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "PMU READ FAILED! > ");
  }
  return ret == ESP_OK ? 0 : -1;
}

/**
 * @brief Write a byte to a pmu register
 */
int pmu_register_write_byte(uint8_t devAddr, uint8_t regAddr, uint8_t* data, uint8_t len) {
  if (data == NULL) {
    return ESP_FAIL;
  }
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, (devAddr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
  i2c_master_write(cmd, data, len, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdTICKS_TO_MS(1000));
  i2c_cmd_link_delete(cmd);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "PMU WRITE FAILED! < ");
  }
  return ret == ESP_OK ? 0 : -1;
}

void InitializeAXP2101(void) {
  // create a queue to handle gpio event from isr
  gpio_evt_queue = xQueueCreate(5, sizeof(uint32_t));
  irq_init();
  ESP_ERROR_CHECK(pmu_init());
  // xTaskCreate(pmu_hander_task, "App/pwr", 4 * 1024, NULL, 10, NULL);
}

static void pmu_hander_task(void* args) {
  while (1) {
    // if (PMU.isPekeyShortPressIrq()) {
    //   ESP_LOGI(TAG, "isPekeyShortPressIrq");
    // }
    pmu_isr_handler();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
