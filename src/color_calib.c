#include "color_calib.h"
#include "nvs_manager.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>
#include <inttypes.h>

static const char *TAG = "color_calib";

#define MAX_CHANNELS 8

/* 黒閾値のデフォルト値 */
#define DEFAULT_BLACK_THRESHOLD 30

_Static_assert(NVS_MANAGER_COLOR_RGBC_COLOR_COUNT == COLOR_CALIB_COLOR_COUNT,
               "NVS_MANAGER_COLOR_RGBC_COLOR_COUNTとCOLOR_CALIB_COLOR_COUNTは一致させてください");

/* デフォルト基準値。index = color_sensor_color_id_t の値。
 * UNKNOWN(0), BLACK(1) は距離判定で使わないためゼロ埋め。
 * WHITE以降は実測ベースの目安値。実機のキャリブレーション値に置き換えて使用すること。
 */
static const color_calib_ref_t s_default_refs[COLOR_CALIB_COLOR_COUNT] = {
    [COLOR_SENSOR_COLOR_UNKNOWN] = {0, 0, 0},
    [COLOR_SENSOR_COLOR_BLACK]   = {0, 0, 0},
    [COLOR_SENSOR_COLOR_WHITE]   = {260, 400, 340},
    [COLOR_SENSOR_COLOR_RED]     = {670, 170, 170},
    [COLOR_SENSOR_COLOR_GREEN]   = {230, 460, 300},
    [COLOR_SENSOR_COLOR_BLUE]    = {120, 320, 570},
    [COLOR_SENSOR_COLOR_YELLOW]  = {470, 360, 160},
    [COLOR_SENSOR_COLOR_ORANGE]  = {560, 280, 160},
    [COLOR_SENSOR_COLOR_PURPLE]  = {320, 220, 460},
    [COLOR_SENSOR_COLOR_CYAN]    = {170, 420, 410},
    [COLOR_SENSOR_COLOR_MAGENTA] = {450, 200, 350},
    [COLOR_SENSOR_COLOR_BROWN]   = {430, 300, 270},
    [COLOR_SENSOR_COLOR_GRAY]    = {330, 350, 320},
    [COLOR_SENSOR_COLOR_PINK]    = {480, 300, 220},
    [COLOR_SENSOR_COLOR_LIME]    = {330, 480, 190},
    [COLOR_SENSOR_COLOR_NAVY]    = {180, 260, 560},
};

static color_calib_ref_t s_refs[MAX_CHANNELS][COLOR_CALIB_COLOR_COUNT];
static uint32_t s_black_threshold[MAX_CHANNELS];
static uint8_t s_channel_count = 0;

/* 生RGBCを合計1000の比率に正規化する */
static void normalize(uint32_t r, uint32_t g, uint32_t b, color_calib_ref_t *out)
{
    uint32_t sum = r + g + b;
    if (sum == 0) {
        out->r = 0;
        out->g = 0;
        out->b = 0;
        return;
    }
    out->r = (r * 1000.0f) / sum;
    out->g = (g * 1000.0f) / sum;
    out->b = (b * 1000.0f) / sum;
}

static void load_from_nvs_or_default(uint8_t channel)
{
    nvs_manager_color_rgbc_t data;
    esp_err_t err = nvs_manager_read_color_rgbc(channel, &data);

    if (err == ESP_OK) 
    {
        for (int i = 0; i < COLOR_CALIB_COLOR_COUNT; i++) 
        {
            normalize(data.color[i].r, data.color[i].g, data.color[i].b, &s_refs[channel][i]);
        }
        /* COLOR_SENSOR_COLOR_BLACKのcの値を黒判定閾値として採用する */
        s_black_threshold[channel] = data.color[COLOR_SENSOR_COLOR_BLACK].c;
        ESP_LOGI(TAG, "CH%d: NVSからキャリブレーション値を読込(黒閾値=%" PRIu32 ")",
                 channel, s_black_threshold[channel]);
    } 
    else 
    {
        memcpy(s_refs[channel], s_default_refs, sizeof(s_default_refs));
        s_black_threshold[channel] = DEFAULT_BLACK_THRESHOLD;

        if (err == ESP_ERR_NVS_NOT_FOUND) 
        {
            ESP_LOGW(TAG, "CH%d: NVSにデータなし、デフォルト値を使用(黒閾値=%d)",
                     channel, DEFAULT_BLACK_THRESHOLD);
        } 
        else 
        {
            ESP_LOGE(TAG, "CH%d: NVS読込エラー(%s)、デフォルト値を使用",
                     channel, esp_err_to_name(err));
        }
    }
}

esp_err_t color_calib_init(uint8_t channel_count)
{
    if (channel_count == 0 || channel_count > MAX_CHANNELS) {
        return ESP_ERR_INVALID_ARG;
    }
    s_channel_count = channel_count;

    for (uint8_t ch = 0; ch < channel_count; ch++) {
        load_from_nvs_or_default(ch);
    }
    return ESP_OK;
}

esp_err_t color_calib_get_refs(uint8_t channel, color_calib_ref_t out[COLOR_CALIB_COLOR_COUNT])
{
    if (channel >= s_channel_count || out == NULL) 
        return ESP_ERR_INVALID_ARG;

    memcpy(out, s_refs[channel], sizeof(s_refs[channel]));
    return ESP_OK;
}

uint32_t color_calib_get_black_threshold(uint8_t channel)
{
    if (channel >= s_channel_count) 
        return DEFAULT_BLACK_THRESHOLD;
    
    return s_black_threshold[channel];
}


esp_err_t color_calib_reload(uint8_t channel)
{
    if (channel >= s_channel_count) 
        return ESP_ERR_INVALID_ARG;

    load_from_nvs_or_default(channel);
    return ESP_OK;
}
