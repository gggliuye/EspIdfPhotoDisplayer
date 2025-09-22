
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#define PMU_INPUT_PIN (gpio_num_t) - 1 /*!< axp power chip interrupt Pin*/
#define PMU_INPUT_PIN_SEL (1ULL << PMU_INPUT_PIN)

#define I2C_MASTER_NUM I2C_NUM_0

#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS 1000

#define WRITE_BIT I2C_MASTER_WRITE   /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ     /*!< I2C master read */
#define ACK_CHECK_EN 0x1             /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0            /*!< I2C master will not check ack from slave */
#define ACK_VAL (i2c_ack_type_t)0x0  /*!< I2C ack value */
#define NACK_VAL (i2c_ack_type_t)0x1 /*!< I2C nack value */

/*
! WARN:
Please do not run the example without knowing the external load voltage of the PMU,
it may burn your external load, please check the voltage setting before running the example,
if there is any loss, please bear it by yourself
*/
// #ifndef XPOWERS_NO_ERROR
// #error "Running this example is known to not damage the device! Please go and uncomment this!"
// #endif

#ifdef __cplusplus
extern "C" {
#endif

int32_t GetBatteryPercent();
bool IsCharging();

void InitializeAXP2101(void);

#ifdef __cplusplus
}
#endif
