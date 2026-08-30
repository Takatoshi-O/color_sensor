#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief カラーセンサーが判定する色の種別です。
 *
 * 配列インデックスとしてそのまま使われるため、値の並び順は
 * NVS上のRGBCキャリブレーションデータの並び順と対応しています。
 */
typedef enum {
    COLOR_SENSOR_COLOR_UNKNOWN = 0,
    COLOR_SENSOR_COLOR_BLACK,
    COLOR_SENSOR_COLOR_WHITE,
    COLOR_SENSOR_COLOR_RED,
    COLOR_SENSOR_COLOR_GREEN,
    COLOR_SENSOR_COLOR_BLUE,
    COLOR_SENSOR_COLOR_YELLOW,
    COLOR_SENSOR_COLOR_ORANGE,
    COLOR_SENSOR_COLOR_PURPLE,
    COLOR_SENSOR_COLOR_CYAN,
    COLOR_SENSOR_COLOR_MAGENTA,
    COLOR_SENSOR_COLOR_BROWN,
    COLOR_SENSOR_COLOR_GRAY,
    COLOR_SENSOR_COLOR_PINK,
    COLOR_SENSOR_COLOR_LIME,
    COLOR_SENSOR_COLOR_NAVY,
} color_sensor_color_id_t;

/**
 * @brief R,G,Bを合計1000になるよう比率正規化する。
 *
 * @param R,G,B 生値(またはホワイトバランス補正後の値)
 * @param r,g,b 正規化後の出力先(合計1000)
 */
void color_classify_normalize_rgb(int R, int G, int B, int *r, int *g, int *b);

/**
 * @brief 指定チャンネル(センサー)のキャリブレーション基準値を使って色判定する。
 *
 * Clear値が該当チャンネルの黒閾値(NVSのCOLOR_SENSOR_COLOR_BLACKエントリのc値)未満の場合は
 * 無条件でCOLOR_SENSOR_COLOR_BLACKを返す。それ以外はR,G,Bを正規化したのち、
 * COLOR_SENSOR_COLOR_WHITE以降の基準値とのユークリッド距離が最小の色を返す。
 *
 * @param channel 対象センサーのチャンネル番号(color_calibのsensor_idと対応)
 * @param R,G,B,C センサー生値
 * @return color_sensor_color_id_t 判定結果
 */
color_sensor_color_id_t color_classify_detect(uint8_t channel, int R, int G, int B, int C);

#ifdef __cplusplus
}
#endif
