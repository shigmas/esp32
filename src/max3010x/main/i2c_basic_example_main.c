/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* i2c - Simple Example

   Simple I2C example that shows how to initialize I2C
   as well as reading and writing from and to registers for a sensor connected over I2C.

   The sensor used in this example is a MAX3010X inertial measurement unit.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "max3010x";

#define I2C_MASTER_SCL_IO           CONFIG_I2C_MASTER_SCL       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           CONFIG_I2C_MASTER_SDA       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
// 100000 from the arduino library
#define I2C_MASTER_FREQ_HZ          CONFIG_I2C_MASTER_FREQUENCY /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000

#define MAX3010X_SENSOR_ADDR         0x57        /*!< Address of the MAX3010X sensor */
#define MAX3010X_WHO_AM_I_REG_ADDR   0x75        /*!< Register addresses of the "who am I" register */
#define MAX3010X_PWR_MGMT_1_REG_ADDR 0x6B        /*!< Register addresses of the power management register */
#define MAX3010X_RESET_BIT           7

// Part ID Registers
#define MAX30105_REVISIONID 	0xFE
#define MAX30105_PARTID		0xFF    // Should always be 0x15. Identical to MAX30102.

#define MAX_30105_EXPECTEDPARTID 0x15

/**
 * @brief Read a sequence of bytes from a MAX3010X sensor registers
 */
static esp_err_t max3010x_register_read(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief Write a byte to a MAX3010X sensor register
 */
static esp_err_t max3010x_register_write_byte(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/**
 * @brief i2c master initialization
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MAX3010X_SENSOR_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
}

static void i2c_verify_device(i2c_master_dev_handle_t dev_handle) {
  uint8_t partID;
  max3010x_register_read(dev_handle, MAX30105_PARTID, &partID, 1);
  if (partID != MAX_30105_EXPECTEDPARTID) {
    ESP_LOGI(TAG, "I2C MAX30105 did not match expected part ID");
  }

  uint8_t revID;
  max3010x_register_read(dev_handle, MAX30105_REVISIONID, &revID, 1);
  ESP_LOGI(TAG, "I2C MAX30105 rev ID: ", revID);
}

void app_main(void)
{
    uint8_t data[2];
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG, "I2C initialized successfully");

    i2c_verify_device(dev_handle);

    /* /\* Read the MAX3010X WHO_AM_I register, on power up the register should have the value 0x71 *\/ */
    /* ESP_ERROR_CHECK(max3010x_register_read(dev_handle, MAX3010X_WHO_AM_I_REG_ADDR, data, 1)); */
    /* ESP_LOGI(TAG, "WHO_AM_I = %X", data[0]); */

    /* /\* Demonstrate writing by resetting the MAX3010X *\/ */
    /* ESP_ERROR_CHECK(max3010x_register_write_byte(dev_handle, MAX3010X_PWR_MGMT_1_REG_ADDR, 1 << MAX3010X_RESET_BIT)); */

    ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C de-initialized successfully");
}
