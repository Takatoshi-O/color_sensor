#include "color_sensor_hw.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "color_sensor_hw";

#define TCS34725_ADDR   0x29
#define CMD_BIT         0x80

#define REG_ENABLE      0x00
#define REG_ATIME       0x01
#define REG_CONTROL     0x0F
#define REG_ID          0x12
#define REG_CDATAL      0x14

#define ENABLE_PON      0x01
#define ENABLE_AEN      0x02

static i2c_master_bus_handle_t s_bus = NULL;
static i2c_master_dev_handle_t s_mux = NULL;   /* PCA9548A */
static i2c_master_dev_handle_t s_dev = NULL;   /* TCS34725 */
static uint8_t s_current_channel = 0xFF;

static esp_err_t write_reg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = { (uint8_t)(CMD_BIT | reg), value };
    return i2c_master_transmit(s_dev, buf, sizeof(buf), -1);
}

static esp_err_t tca9548_select(uint8_t ch)
{
    if (ch > 7) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_current_channel == ch) {
        return ESP_OK;
    }

    uint8_t data = (uint8_t)(1u << ch);
    esp_err_t err = i2c_master_transmit(s_mux, &data, 1, -1);
    if (err == ESP_OK) {
        s_current_channel = ch;
    }
    return err;
}

bool color_sensor_hw_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = CONFIG_COLOR_SENSOR_I2C_PORT,
        .sda_io_num = CONFIG_COLOR_SENSOR_SDA_GPIO,
        .scl_io_num = CONFIG_COLOR_SENSOR_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    if (i2c_new_master_bus(&bus_cfg, &s_bus) != ESP_OK) {
        ESP_LOGE(TAG, "I2Cバス生成失敗");
        return false;
    }

    i2c_device_config_t mux_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CONFIG_COLOR_SENSOR_PCA9548_ADDR,
        .scl_speed_hz = CONFIG_COLOR_SENSOR_I2C_FREQ_HZ,
    };
    if (i2c_master_bus_add_device(s_bus, &mux_cfg, &s_mux) != ESP_OK) {
        ESP_LOGE(TAG, "PCA9548Aデバイス登録失敗");
        return false;
    }

    if (tca9548_select(0) != ESP_OK) {
        ESP_LOGE(TAG, "初期チャンネル選択失敗");
        return false;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = TCS34725_ADDR,
        .scl_speed_hz = CONFIG_COLOR_SENSOR_I2C_FREQ_HZ,
    };
    if (i2c_master_bus_add_device(s_bus, &dev_cfg, &s_dev) != ESP_OK) {
        ESP_LOGE(TAG, "TCS34725デバイス登録失敗");
        return false;
    }

    uint8_t reg = (uint8_t)(CMD_BIT | REG_ID);
    uint8_t id = 0;
    if (i2c_master_transmit_receive(s_dev, &reg, 1, &id, 1, -1) != ESP_OK) {
        ESP_LOGE(TAG, "TCS34725 ID読み取り失敗");
        return false;
    }
    if (id != 0x44) {
        ESP_LOGE(TAG, "TCS34725 ID不一致: 0x%02X", id);
        return false;
    }

    for (int ch = 0; ch < CONFIG_COLOR_SENSOR_CHANNEL_COUNT; ch++) {
        if (tca9548_select((uint8_t)ch) != ESP_OK) {
            ESP_LOGE(TAG, "CH%d: チャンネル選択失敗、このチャンネルをスキップ", ch);
            continue;
        }

        esp_err_t e1 = write_reg(REG_ATIME, 0xFF);
        esp_err_t e2 = write_reg(REG_CONTROL, 0x02);
        esp_err_t e3 = write_reg(REG_ENABLE, ENABLE_PON);
        vTaskDelay(pdMS_TO_TICKS(3));
        esp_err_t e4 = write_reg(REG_ENABLE, ENABLE_PON | ENABLE_AEN);
        vTaskDelay(pdMS_TO_TICKS(3));

        if (e1 != ESP_OK || e2 != ESP_OK || e3 != ESP_OK || e4 != ESP_OK) {
            ESP_LOGE(TAG, "CH%d: 初期化中にI2Cエラーが発生しました", ch);
        } else {
            ESP_LOGI(TAG, "CH%d: TCS34725初期化完了", ch);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
    return true;
}

esp_err_t color_sensor_hw_read_channel(uint8_t ch, uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c)
{
    if (r == NULL || g == NULL || b == NULL || c == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = tca9548_select(ch);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t addr = (uint8_t)(CMD_BIT | REG_CDATAL);
    uint8_t data[8];
    err = i2c_master_transmit_receive(s_dev, &addr, 1, data, sizeof(data), -1);
    if (err != ESP_OK) {
        return err;
    }

    *c = (uint16_t)(data[0] | (data[1] << 8));
    *r = (uint16_t)(data[2] | (data[3] << 8));
    *g = (uint16_t)(data[4] | (data[5] << 8));
    *b = (uint16_t)(data[6] | (data[7] << 8));

    return ESP_OK;
}
