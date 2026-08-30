#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2Cバス・PCA9548A・TCS34725を初期化する。
 *
 * SDA/SCL/I2Cポート/PCA9548Aアドレス/使用チャンネル数はKconfig
 * (CONFIG_COLOR_SENSOR_*)から取得される。
 *
 * @return true 初期化成功
 * @return false 初期化失敗(I2Cエラー、またはTCS34725のID不一致)
 */
bool color_sensor_hw_init(void);

/**
 * @brief 指定チャンネルを選択したのち、TCS34725からRGBC生値を読み取る。
 *
 * @param ch チャンネル番号(0-7)
 * @param r,g,b,c 読み取り結果の格納先(生値、スケーリングなし)
 * @return esp_err_t
 */
esp_err_t color_sensor_hw_read_channel(uint8_t ch, uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c);

#ifdef __cplusplus
}
#endif
