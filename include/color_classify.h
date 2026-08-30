#pragma once
/**
 * @file color_classify.h
 * @brief カラーセンサーの色ID定義と、正規化済みRGB基準値を用いた色分類APIを定義します。
 */

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
/**
 * @brief カラーセンサーが返す色IDの列挙型です。
 *
 * 値はキャリブレーション配列のインデックスとして利用されます。
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
/**
 * @brief RGB値を合計1000となる比率へ正規化します。
 *
 * @param R 入力R値です。
 * @param G 入力G値です。
 * @param B 入力B値です。
 * @param r 正規化後R値の格納先です。
 * @param g 正規化後G値の格納先です。
 * @param b 正規化後B値の格納先です。
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
/**
 * @brief 指定チャンネルの基準値を使って色を判定します。
 *
 * 黒判定用のClear閾値を先に適用し、それ以外は正規化RGBと基準値の距離から判定します。
 *
 * @param channel センサーを識別するチャンネル番号です。
 * @param R 生R値です。
 * @param G 生G値です。
 * @param B 生B値です。
 * @param C 生Clear値です。
 * @return 判定された色IDです。
 */
color_sensor_color_id_t color_classify_detect(uint8_t channel, int R, int G, int B, int C);

#ifdef __cplusplus
}
#endif
