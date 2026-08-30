#include "color_classify.h"
#include "color_calib.h"
#include "esp_err.h"
#include <limits.h>
#include <stddef.h>

void color_classify_normalize_rgb(int R, int G, int B, int *r, int *g, int *b)
{
    if (r == NULL || g == NULL || b == NULL) {
        return;
    }

    uint32_t sum = (uint32_t)(R + G + B);
    if (sum == 0) {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    *r = (R * 1000) / (int)sum;
    *g = (G * 1000) / (int)sum;
    *b = (B * 1000) / (int)sum;
}

static int color_distance(int r1, int g1, int b1, int r2, int g2, int b2)
{
    int dr = r1 - r2;
    int dg = g1 - g2;
    int db = b1 - b2;
    return dr * dr + dg * dg + db * db;
}

color_sensor_color_id_t color_classify_detect(uint8_t channel, int R, int G, int B, int C)
{
    /* 黒判定閾値はNVS(COLOR_SENSOR_COLOR_BLACKのc値)から取得する。
     * NVS未設定の場合はcolor_calib側のデフォルト値(30)が返る。
     */
    uint32_t black_threshold = color_calib_get_black_threshold(channel);
    if ((uint32_t)C < black_threshold) 
        return COLOR_SENSOR_COLOR_BLACK;

    int r, g, b;
    color_classify_normalize_rgb(R, G, B, &r, &g, &b);

    color_calib_ref_t refs[COLOR_CALIB_COLOR_COUNT];
    if (color_calib_get_refs(channel, refs) != ESP_OK) 
        return COLOR_SENSOR_COLOR_UNKNOWN;

    int best_dist = INT32_MAX;
    color_sensor_color_id_t best_color = COLOR_SENSOR_COLOR_UNKNOWN;

    /* index 0(UNKNOWN), 1(BLACK)は距離判定の対象から除外し、WHITE以降のみ比較する */
    for (int i = COLOR_SENSOR_COLOR_WHITE; i < COLOR_CALIB_COLOR_COUNT; i++) {
        int dist = color_distance(r, g, b, (int)refs[i].r, (int)refs[i].g, (int)refs[i].b);
        if (dist < best_dist) 
        {
            best_dist = dist;
            best_color = (color_sensor_color_id_t)i;
        }
    }

    return best_color;
}
