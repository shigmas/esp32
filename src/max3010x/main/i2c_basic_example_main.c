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
#include "esp_timer.h" // for esp_timer_get_time(), the replacement for millis()
#include "driver/i2c_master.h"

static const char *TAG = "max3010x";

// Status Registers
#define MAX30105_INTSTAT1 		0x00
#define MAX30105_INTSTAT2 		0x01
#define MAX30105_INTENABLE1 		0x02
#define MAX30105_INTENABLE2 		0x03

// FIFO Registers
#define MAX30105_FIFOWRITEPTR   	0x04
#define MAX30105_FIFOOVERFLOW   	0x05
#define MAX30105_FIFOREADPTR      	0x06
#define MAX30105_FIFODATA 		0x07

// Configuration Registers
#define MAX30105_FIFOCONFIG  		0x08
#define MAX30105_MODECONFIG  		0x09
#define MAX30105_PARTICLECONFIG  	0x0A    // Note, sometimes listed as "SPO2" config in datasheet (pg. 11)
#define MAX30105_LED1_PULSEAMP  	0x0C
#define MAX30105_LED2_PULSEAMP  	0x0D
#define MAX30105_LED3_PULSEAMP  	0x0E
#define MAX30105_LED_PROX_AMP   	0x10
#define MAX30105_MULTILEDCONFIG1        0x11
#define MAX30105_MULTILEDCONFIG2        0x12

// Die Temperature Registers
#define MAX30105_DIETEMPINT  		0x1F
#define MAX30105_DIETEMPFRAC  	        0x20
#define MAX30105_DIETEMPCONFIG  	0x21

// Proximity Function Registers
#define MAX30105_PROXINTTHRESH  	0x30

#define MAX30105_INT_A_FULL_MASK 	(uint8_t)~0b10000000
#define MAX30105_INT_A_FULL_ENABLE  	0x80
#define MAX30105_INT_A_FULL_DISABLE  	0x00

#define MAX30105_INT_DATA_RDY_MASK      (uint8_t)~0b01000000
#define MAX30105_INT_DATA_RDY_ENABLE 	0x40
#define MAX30105_INT_DATA_RDY_DISABLE   0x00

#define MAX30105_INT_ALC_OVF_MASK       (uint8_t)~0b00100000
#define MAX30105_INT_ALC_OVF_ENABLE  	0x20
#define MAX30105_INT_ALC_OVF_DISABLE    0x00

#define MAX30105_INT_PROX_INT_MASK      (uint8_t)~0b00010000
#define MAX30105_INT_PROX_INT_ENABLE    0x10
#define MAX30105_INT_PROX_INT_DISABLE   0x00

#define MAX30105_INT_DIE_TEMP_RDY_MASK  (uint8_t)~0b00000010
#define MAX30105_INT_DIE_TEMP_RDY_ENABLE 0x02
#define MAX30105_INT_DIE_TEMP_RDY_DISABLE 0x00

#define MAX30105_SAMPLEAVG_MASK 	(uint8_t)~0b11100000
#define MAX30105_SAMPLEAVG_1    	0x00
#define MAX30105_SAMPLEAVG_2    	0x20
#define MAX30105_SAMPLEAVG_4    	0x40
#define MAX30105_SAMPLEAVG_8    	0x60
#define MAX30105_SAMPLEAVG_16   	0x80
#define MAX30105_SAMPLEAVG_32   	0xA0

#define MAX30105_ROLLOVER_MASK  	0xEF
#define MAX30105_ROLLOVER_ENABLE        0x10
#define MAX30105_ROLLOVER_DISABLE       0x00

#define MAX30105_A_FULL_MASK    	0xF0

// Mode configuration commands (page 19)
#define MAX30105_SHUTDOWN_MASK  	0x7F
#define MAX30105_SHUTDOWN       	0x80
#define MAX30105_WAKEUP  		0x00

#define MAX30105_RESET_MASK       	0xBF
#define MAX30105_RESET  		0x40

#define MAX30105_MODE_MASK      	0xF8
#define MAX30105_MODE_REDONLY   	0x02
#define MAX30105_MODE_REDIRONLY  	0x03
#define MAX30105_MODE_MULTILED  	0x07

// Particle sensing configuration commands (pgs 19-20)
#define MAX30105_ADCRANGE_MASK  	0x9F
#define MAX30105_ADCRANGE_2048  	0x00
#define MAX30105_ADCRANGE_4096  	0x20
#define MAX30105_ADCRANGE_8192  	0x40
#define MAX30105_ADCRANGE_16384  	0x60

#define MAX30105_SAMPLERATE_MASK        0xE3
#define MAX30105_SAMPLERATE_50  	0x00
#define MAX30105_SAMPLERATE_100  	0x04
#define MAX30105_SAMPLERATE_200  	0x08
#define MAX30105_SAMPLERATE_400  	0x0C
#define MAX30105_SAMPLERATE_800  	0x10
#define MAX30105_SAMPLERATE_1000        0x14
#define MAX30105_SAMPLERATE_1600        0x18
#define MAX30105_SAMPLERATE_3200        0x1C

#define MAX30105_PULSEWIDTH_MASK        0xFC
#define MAX30105_PULSEWIDTH_69  	0x00
#define MAX30105_PULSEWIDTH_118  	0x01
#define MAX30105_PULSEWIDTH_215  	0x02
#define MAX30105_PULSEWIDTH_411  	0x03

//Multi-LED Mode configuration (pg 22)
#define MAX30105_SLOT1_MASK  		0xF8
#define MAX30105_SLOT2_MASK  		0x8F
#define MAX30105_SLOT3_MASK  		0xF8
#define MAX30105_SLOT4_MASK  		0x8F

#define SLOT_NONE 			0x00
#define SLOT_RED_LED  			0x01
#define SLOT_IR_LED 			0x02
#define SLOT_GREEN_LED 			0x03
#define SLOT_NONE_PILOT 		0x04
#define SLOT_RED_PILOT 			0x05
#define SLOT_IR_PILOT  			0x06
#define SLOT_GREEN_PILOT  		0x07


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

// global variables (module level)
uint8_t  activeLEDs;

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
  if (partID != MAX_30105_EXPECTEDPARTID)
  {
      ESP_LOGI(TAG, "I2C MAX30105 did not match expected part ID");
  }

  uint8_t revID;
  max3010x_register_read(dev_handle, MAX30105_REVISIONID, &revID, 1);
  ESP_LOGI(TAG, "I2C MAX30105 rev ID: ", revID);
}

static void max_bitmask(i2c_master_dev_handle_t dev_handle, uint8_t reg, uint8_t mask, uint8_t bit) {
  uint8_t origBit;
  
  max3010x_register_read(dev_handle, MAX30105_PARTID, &origBit, 1);
  origBit = origBit & mask;

  max3010x_register_write_byte(dev_handle, reg, origBit | bit);
}

static void soft_reset(i2c_master_dev_handle_t dev_handle) {
 max_bitmask(dev_handle, MAX30105_MODECONFIG, MAX30105_RESET_MASK,
              MAX30105_RESET);
}

static void setFIFOAverage(i2c_master_dev_handle_t dev_handle, uint8_t numberOfSamples) {
  max_bitmask(dev_handle, MAX30105_FIFOCONFIG, MAX30105_SAMPLEAVG_MASK, numberOfSamples);
}

static void enableFIFORollover(i2c_master_dev_handle_t dev_handle) {
  max_bitmask(dev_handle, MAX30105_FIFOCONFIG, MAX30105_ROLLOVER_MASK,
          MAX30105_ROLLOVER_ENABLE);
}

static void setLEDMode(i2c_master_dev_handle_t dev_handle, uint8_t mode) {
  // Set which LEDs are used for sampling -- Red only, RED+IR only, or custom.
  // See datasheet, page 19
  max_bitmask(dev_handle, MAX30105_MODECONFIG, MAX30105_MODE_MASK, mode);
}

static void setADCRange(i2c_master_dev_handle_t dev_handle, uint8_t adcRange) {
  // adcRange: one of MAX30105_ADCRANGE_2048, _4096, _8192, _16384
  max_bitmask(dev_handle, MAX30105_PARTICLECONFIG, MAX30105_ADCRANGE_MASK, adcRange);
}

static void setSampleRate(i2c_master_dev_handle_t dev_handle, uint8_t sampleRate) {
  // sampleRate: one of MAX30105_SAMPLERATE_50, _100, _200, _400, _800, _1000, _1600, _3200
  max_bitmask(dev_handle, MAX30105_PARTICLECONFIG, MAX30105_SAMPLERATE_MASK, sampleRate);
}

static void setPulseWidth(i2c_master_dev_handle_t dev_handle, uint8_t pulseWidth) {
  // pulseWidth: one of MAX30105_PULSEWIDTH_69, _188, _215, _411
  max_bitmask(dev_handle, MAX30105_PARTICLECONFIG, MAX30105_PULSEWIDTH_MASK, pulseWidth);
}

// NOTE: Amplitude values: 0x00 = 0mA, 0x7F = 25.4mA, 0xFF = 50mA (typical)
// See datasheet, page 21
static void setPulseAmplitudeRed(i2c_master_dev_handle_t dev_handle, uint8_t amplitude) {
  max3010x_register_write_byte(dev_handle, MAX30105_LED1_PULSEAMP, amplitude);
}

static void setPulseAmplitudeIR(i2c_master_dev_handle_t dev_handle, uint8_t amplitude) {
  max3010x_register_write_byte(dev_handle, MAX30105_LED2_PULSEAMP, amplitude);
}

static void setPulseAmplitudeGreen(i2c_master_dev_handle_t dev_handle, uint8_t amplitude) {
  max3010x_register_write_byte(dev_handle, MAX30105_LED3_PULSEAMP, amplitude);
}

static void setPulseAmplitudeProximity(i2c_master_dev_handle_t dev_handle, uint8_t amplitude) {
  max3010x_register_write_byte(dev_handle, MAX30105_LED_PROX_AMP, amplitude);
}

//Given a slot number assign a thing to it
//Devices are SLOT_RED_LED or SLOT_RED_PILOT (proximity)
//Assigning a SLOT_RED_LED will pulse LED
//Assigning a SLOT_RED_PILOT will ??
static void enableSlot(i2c_master_dev_handle_t dev_handle, uint8_t slotNumber, uint8_t device) {

  switch (slotNumber) {
    case (1):
      max_bitmask(dev_handle, MAX30105_MULTILEDCONFIG1, MAX30105_SLOT1_MASK, device);
      break;
    case (2):
      max_bitmask(dev_handle, MAX30105_MULTILEDCONFIG1, MAX30105_SLOT2_MASK, device << 4);
      break;
    case (3):
      max_bitmask(dev_handle, MAX30105_MULTILEDCONFIG2, MAX30105_SLOT3_MASK, device);
      break;
    case (4):
      max_bitmask(dev_handle, MAX30105_MULTILEDCONFIG2, MAX30105_SLOT4_MASK, device << 4);
      break;
    default:
      //Shouldn't be here!
      break;
  }
}

//Resets all points to start in a known state
//Page 15 recommends clearing FIFO before beginning a read
static void clearFIFO(i2c_master_dev_handle_t dev_handle) {
  max3010x_register_write_byte(dev_handle, MAX30105_FIFOWRITEPTR, 0);
  max3010x_register_write_byte(dev_handle, MAX30105_FIFOOVERFLOW, 0);
  max3010x_register_write_byte(dev_handle, MAX30105_FIFOREADPTR, 0);
}

// particleSensor.setup(0); //Configure sensor. Turn off LEDs (1 is fine, just a
// little warmer)
static void max_set_up_device(i2c_master_dev_handle_t dev_handle,
                              uint8_t powerLevel, uint8_t sampleAverage, uint8_t ledMode,
                              int sampleRate, int pulseWidth, int adcRange)
{
  soft_reset(dev_handle);

  //FIFO Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  //The chip will average multiple samples of same type together if you wish
  if (sampleAverage == 1) setFIFOAverage(dev_handle, MAX30105_SAMPLEAVG_1); //No averaging per FIFO record
  else if (sampleAverage == 2) setFIFOAverage(dev_handle, MAX30105_SAMPLEAVG_2);
  else if (sampleAverage == 4) setFIFOAverage(dev_handle, MAX30105_SAMPLEAVG_4);
  else if (sampleAverage == 8) setFIFOAverage(dev_handle, MAX30105_SAMPLEAVG_8);
  else if (sampleAverage == 16) setFIFOAverage(dev_handle, MAX30105_SAMPLEAVG_16);
  else if (sampleAverage == 32) setFIFOAverage(dev_handle, MAX30105_SAMPLEAVG_32);
  else setFIFOAverage(dev_handle, MAX30105_SAMPLEAVG_4);

  //setFIFOAlmostFull(2); //Set to 30 samples to trigger an 'Almost Full' interrupt
  enableFIFORollover(dev_handle); //Allow FIFO to wrap/roll over
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  //Mode Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  if (ledMode == 3) setLEDMode(dev_handle, MAX30105_MODE_MULTILED); //Watch all three LED channels
  else if (ledMode == 2) setLEDMode(dev_handle, MAX30105_MODE_REDIRONLY); //Red and IR
  else setLEDMode(dev_handle, MAX30105_MODE_REDONLY); //Red only
  activeLEDs = ledMode; //Used to control how many bytes to read from FIFO buffer
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  //Particle Sensing Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  if(adcRange < 4096) setADCRange(dev_handle, MAX30105_ADCRANGE_2048); //7.81pA per LSB
  else if(adcRange < 8192) setADCRange(dev_handle, MAX30105_ADCRANGE_4096); //15.63pA per LSB
  else if(adcRange < 16384) setADCRange(dev_handle, MAX30105_ADCRANGE_8192); //31.25pA per LSB
  else if(adcRange == 16384) setADCRange(dev_handle, MAX30105_ADCRANGE_16384); //62.5pA per LSB
  else setADCRange(dev_handle, MAX30105_ADCRANGE_2048);

  if (sampleRate < 100) setSampleRate(dev_handle, MAX30105_SAMPLERATE_50); //Take 50 samples per second
  else if (sampleRate < 200) setSampleRate(dev_handle, MAX30105_SAMPLERATE_100);
  else if (sampleRate < 400) setSampleRate(dev_handle, MAX30105_SAMPLERATE_200);
  else if (sampleRate < 800) setSampleRate(dev_handle, MAX30105_SAMPLERATE_400);
  else if (sampleRate < 1000) setSampleRate(dev_handle, MAX30105_SAMPLERATE_800);
  else if (sampleRate < 1600) setSampleRate(dev_handle, MAX30105_SAMPLERATE_1000);
  else if (sampleRate < 3200) setSampleRate(dev_handle, MAX30105_SAMPLERATE_1600);
  else if (sampleRate == 3200) setSampleRate(dev_handle, MAX30105_SAMPLERATE_3200);
  else setSampleRate(dev_handle, MAX30105_SAMPLERATE_50);

  //The longer the pulse width the longer range of detection you'll have
  //At 69us and 0.4mA it's about 2 inches
  //At 411us and 0.4mA it's about 6 inches
  if (pulseWidth < 118) setPulseWidth(dev_handle, MAX30105_PULSEWIDTH_69); //Page 26, Gets us 15 bit resolution
  else if (pulseWidth < 215) setPulseWidth(dev_handle, MAX30105_PULSEWIDTH_118); //16 bit resolution
  else if (pulseWidth < 411) setPulseWidth(dev_handle, MAX30105_PULSEWIDTH_215); //17 bit resolution
  else if (pulseWidth == 411) setPulseWidth(dev_handle, MAX30105_PULSEWIDTH_411); //18 bit resolution
  else setPulseWidth(dev_handle, MAX30105_PULSEWIDTH_69);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  //LED Pulse Amplitude Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  //Default is 0x1F which gets us 6.4mA
  //powerLevel = 0x02, 0.4mA - Presence detection of ~4 inch
  //powerLevel = 0x1F, 6.4mA - Presence detection of ~8 inch
  //powerLevel = 0x7F, 25.4mA - Presence detection of ~8 inch
  //powerLevel = 0xFF, 50.0mA - Presence detection of ~12 inch

  setPulseAmplitudeRed(dev_handle, powerLevel);
  setPulseAmplitudeIR(dev_handle, powerLevel);
  setPulseAmplitudeGreen(dev_handle, powerLevel);
  setPulseAmplitudeProximity(dev_handle, powerLevel);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-


  //Multi-LED Mode Configuration, Enable the reading of the three LEDs
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  enableSlot(dev_handle, 1, SLOT_RED_LED);
  if (ledMode > 1) enableSlot(dev_handle, 2, SLOT_IR_LED);
  if (ledMode > 2) enableSlot(dev_handle, 3, SLOT_GREEN_LED);
  //enableSlot(1, SLOT_RED_PILOT);
  //enableSlot(2, SLOT_IR_PILOT);
  //enableSlot(3, SLOT_GREEN_PILOT);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  clearFIFO(dev_handle); //Reset the FIFO before we begin checking the sensor
}

//particleSensor.enableDIETEMPRDY(); //Enable the temp ready interrupt. This is required.
static void max_enable_device_temp(i2c_master_dev_handle_t dev_handle) {}


//float temperature = particleSensor.readTemperature();
//float temperatureF = particleSensor.readTemperatureF(); //Because I am a bad global citizen
static float max_read_temp(i2c_master_dev_handle_t dev_handle) {
  //DIE_TEMP_RDY interrupt must be enabled
  //See issue 19: https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/issues/19
  
  // Step 1: Config die temperature register to take 1 temperature sample
  max3010x_register_write_byte(dev_handle, MAX30105_DIETEMPCONFIG, 0x01);

  // Poll for bit to clear, reading is then complete
  // Timeout after 100ms
  unsigned long startTime = esp_timer_get_time();
  while (esp_timer_get_time() - startTime < 100)
  {
    //uint8_t response = readRegister8(_i2caddr, MAX30105_DIETEMPCONFIG); //Original way
    //if ((response & 0x01) == 0) break; //We're done!
    
    //Check to see if DIE_TEMP_RDY interrupt is set
    uint8_t response;
    max3010x_register_read(dev_handle, MAX30105_INTSTAT2, &response, 1);
    if ((response & MAX30105_INT_DIE_TEMP_RDY_ENABLE) > 0) break; //We're done!
    vTaskDelay(1000); //Let's not over burden the I2C bus
  }
  //TODO How do we want to fail? With what type of error?
  //? if(millis() - startTime >= 100) return(-999.0);

  // Step 2: Read die temperature register (integer)
  uint8_t tempInt;
  max3010x_register_read(dev_handle, MAX30105_DIETEMPINT, &tempInt, 1);
  uint8_t tempFrac;
  max3010x_register_read(dev_handle, MAX30105_DIETEMPFRAC, &tempFrac, 1); //Causes the clearing of the DIE_TEMP_RDY interrupt

  // Step 3: Calculate temperature (datasheet pg. 23)
  return (float)tempInt + ((float)tempFrac * 0.0625);
}

void app_main(void)
{
    uint8_t data[2];
    i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;

    uint8_t powerLevel = 0x1F;
    uint8_t sampleAverage = 4;
    uint8_t ledMode = 3;
    int sampleRate = 400;
    int pulseWidth = 411;
    int adcRange = 4096;


    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG, "I2C initialized successfully");

    i2c_verify_device(dev_handle);
    ESP_LOGI(TAG, "I2C device created successfully");

    max_set_up_device(dev_handle, powerLevel, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
    ESP_LOGI(TAG, "I2C device set up");

    float temp = max_read_temp(dev_handle);
    ESP_LOGI(TAG, "I2C read temp: %f", temp);
    
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(dev_handle));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C de-initialized successfully");
}
