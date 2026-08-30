#pragma once
/**
 * @file color_calib.h
 * @brief カラーセンサーの色基準値と黒判定閾値をNVSから読み出して管理するキャリブレーションAPIを定義します。
 */

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "color_classify.h"  /* color_sensor_color_id_t を使用 */

#ifdef __cplusplus
extern "C" {
#endif

/* NVS_MANAGER_COLOR_RGBC_COLOR_COUNT(=16)と一致させること。
 * 配列インデックスはcolor_sensor_color_id_tの値と対応する。
 */
/** @brief キャリブレーションで管理する色数です。color_sensor_color_id_tの列挙値と対応します。 */
#define COLOR_CALIB_COLOR_COUNT 16

/**
 * @brief 1色分の正規化済みRGB基準値です。
 *
 * 通常はR、G、Bの比率を表します。
 */
typedef struct {
    float r;
    float g;
    float b;
} color_calib_ref_t;

/**
 * @brief キャリブレーション層を初期化し、全チャンネル分の基準値をNVSからロードする。
 *        NVSに未保存のチャンネルはデフォルト値を使用する。
 *
 * @param channel_count 管理するチャンネル数(PCA9548Aの使用ch数)
 * @return esp_err_t
 */
esp_err_t color_calib_init(uint8_t channel_count);

/**
 * @brief 指定チャンネル(センサー)の基準値配列(正規化済みR,G,B)を取得する。
 *
 * @param channel チャンネル番号 = sensor_idとして扱う
 * @param out COLOR_CALIB_COLOR_COUNT個の格納先
 * @return esp_err_t
 */
esp_err_t color_calib_get_refs(uint8_t channel, color_calib_ref_t out[COLOR_CALIB_COLOR_COUNT]);

/**
 * @brief 指定チャンネルの黒判定用Clear閾値を取得する。
 *
 * NVSのCOLOR_SENSOR_COLOR_BLACKエントリのc値(生値)を返す。
 * NVSに未保存の場合はデフォルト閾値(30)を返す。
 *
 * @param channel チャンネル番号 = sensor_id
 * @return uint32_t 黒判定の閾値(この値未満のClearをCOLOR_SENSOR_COLOR_BLACKとみなす)
 */
uint32_t color_calib_get_black_threshold(uint8_t channel);

/**
 * @brief 指定チャンネル(センサー)の基準値・黒閾値をNVSから再読み込みし、
 *        内部キャッシュを更新する。
 *
 * 外部でNVSデータを更新した後、該当センサーだけに反映させたい場合に呼び出す。
 *
 * @param channel チャンネル番号 = sensor_id
 * @return esp_err_t ESP_OK: 成功 / ESP_ERR_NVS_NOT_FOUND: データなし(デフォルト維持)
 */
esp_err_t color_calib_reload(uint8_t channel);

#ifdef __cplusplus
}
#endif
