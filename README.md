# color_sensor

ESP-IDF用コンポーネント。PCA9548A I2Cマルチプレクサ経由で複数の TCS34725 カラーセンサーを扱い、
NVSベースのキャリブレーションと色判定を行います。

## 構成

3つの層に分割されています。

| ファイル | 責務 |
|---|---|
| `color_sensor_hw.c/h` | I2Cバス・PCA9548A・TCS34725の初期化、RGBC生値の読み取り |
| `color_calib.c/h` | NVS(`nvs_manager`)を使ったキャリブレーション基準値・黒判定閾値の読込/保存/再読込 |
| `color_classify.c/h` | 正規化・ユークリッド距離による色判定、色ID定義(`color_sensor_color_id_t`) |

## 依存コンポーネント

- `driver`（ESP-IDF標準）
- `nvs_flash`（ESP-IDF標準）
- `nvs_manager`（本コンポーネント外で提供される前提。`nvs_manager_read_color_rgbc` / `nvs_manager_write_color_rgbc` 等を使用）

`nvs_manager.h`側で以下のマクロを**16**に設定しておく必要があります。

```c
#define NVS_MANAGER_COLOR_RGBC_COLOR_COUNT 16
```

これは`color_calib.h`の`COLOR_CALIB_COLOR_COUNT`(=16)と`_Static_assert`で一致を検証しています。

## Kconfig設定

`idf.py menuconfig` → `Component config` → `Color Sensor Configuration` で以下を変更できます。

| 設定項目 | デフォルト値 | 説明 |
|---|---|---|
| `COLOR_SENSOR_I2C_PORT` | 0 | I2Cポート番号 |
| `COLOR_SENSOR_SDA_GPIO` | 5 | I2C SDAピン |
| `COLOR_SENSOR_SCL_GPIO` | 6 | I2C SCLピン |
| `COLOR_SENSOR_I2C_FREQ_HZ` | 100000 | I2Cクロック周波数 |
| `COLOR_SENSOR_PCA9548_ADDR` | 0x70 | PCA9548Aのアドレス |
| `COLOR_SENSOR_CHANNEL_COUNT` | 2 | 初期化する使用チャンネル数(0chから連番) |

## 色ID (`color_sensor_color_id_t`)

```c
typedef enum {
    LUMP_COLOR_UNKNOWN = 0,
    LUMP_COLOR_BLACK,
    LUMP_COLOR_WHITE,
    LUMP_COLOR_RED,
    LUMP_COLOR_GREEN,
    LUMP_COLOR_BLUE,
    LUMP_COLOR_YELLOW,
    LUMP_COLOR_ORANGE,
    LUMP_COLOR_PURPLE,
    LUMP_COLOR_CYAN,
    LUMP_COLOR_MAGENTA,
    LUMP_COLOR_BROWN,
    LUMP_COLOR_GRAY,
    LUMP_COLOR_PINK,
    LUMP_COLOR_LIME,
    LUMP_COLOR_NAVY,
} color_sensor_color_id_t;
```

配列インデックスはこのenum値と対応しており、NVS上のRGBCキャリブレーションデータの並び順もこれに合わせます。

## 黒判定の閾値について

以前は`C < 12`という固定値で黒判定していましたが、現在は**NVSに保存されている`LUMP_COLOR_BLACK`エントリの`c`値**をチャンネルごとの黒判定閾値として使用します。

- NVSに該当データが無い場合はデフォルト値`12`が使われます。
- `color_calib_save_raw()`でキャリブレーションデータを保存すると、その中の`LUMP_COLOR_BLACK`インデックスの`c`値が以後の黒閾値として自動的に反映されます。
- 閾値のみを個別に確認したい場合は`color_calib_get_black_threshold(channel)`を呼び出してください。

## 使用例

```c
#include "color_sensor_hw.h"
#include "color_calib.h"
#include "color_classify.h"
#include "nvs_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_manager_init());

    if (!color_sensor_hw_init()) {
        ESP_LOGE("app", "センサー初期化失敗");
        return;
    }
    ESP_ERROR_CHECK(color_calib_init(CONFIG_COLOR_SENSOR_CHANNEL_COUNT));

    while (1) {
        for (int ch = 0; ch < CONFIG_COLOR_SENSOR_CHANNEL_COUNT; ch++) {
            uint16_t r, g, b, c;
            if (color_sensor_hw_read_channel((uint8_t)ch, &r, &g, &b, &c) == ESP_OK) {
                color_sensor_color_id_t result = color_classify_detect((uint8_t)ch, r, g, b, c);
                ESP_LOGI("app", "CH%d: color_id=%d (C=%d)", ch, result, c);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* キャリブレーション用コマンド等でNVSデータを更新した後に呼び出す */
void on_calibration_updated(uint8_t sensor_ch)
{
    color_calib_reload(sensor_ch);
}

/* キャリブレーション値を新規保存する例(16色分のRGBC生値を用意して呼び出す) */
void save_calibration_example(uint8_t sensor_ch,
                               const uint32_t r[COLOR_CALIB_COLOR_COUNT],
                               const uint32_t g[COLOR_CALIB_COLOR_COUNT],
                               const uint32_t b[COLOR_CALIB_COLOR_COUNT],
                               const uint32_t c[COLOR_CALIB_COLOR_COUNT])
{
    /* c[LUMP_COLOR_BLACK] が、以後この sensor_ch の黒判定閾値になる */
    ESP_ERROR_CHECK(color_calib_save_raw(sensor_ch, r, g, b, c));
}
```

## API一覧

### color_sensor_hw

| 関数 | 説明 |
|---|---|
| `color_sensor_hw_init()` | I2Cバス・PCA9548A・TCS34725を初期化 |
| `color_sensor_hw_read_channel()` | 指定チャンネル選択後、RGBC生値を読み取り |

### color_calib

| 関数 | 説明 |
|---|---|
| `color_calib_init()` | 全チャンネル分の基準値・黒閾値をNVSからロード(初期化時に1回呼ぶ) |
| `color_calib_get_refs()` | 指定チャンネルの正規化済み基準値(16色分)を取得 |
| `color_calib_get_black_threshold()` | 指定チャンネルの黒判定閾値(Clear値)を取得 |
| `color_calib_save_raw()` | 生RGBC基準値をNVSへ保存し、キャッシュも更新 |
| `color_calib_reload()` | 指定チャンネルのみNVSから再読込(外部更新の反映用) |

### color_classify

| 関数 | 説明 |
|---|---|
| `color_classify_normalize_rgb()` | R,G,Bを合計1000に正規化 |
| `color_classify_detect()` | 黒閾値判定→正規化→ユークリッド距離判定で色IDを返す |

## 注意事項

- 複数タスクから同時アクセスする場合は、呼び出し側でMutexによる排他制御を行ってください。
- `s_default_refs`（ORANGE/PURPLE/CYAN等）はあくまで仮の目安値です。実機でのキャリブレーション実施を推奨します。
- `color_sensor_hw_init()`はKconfigの`COLOR_SENSOR_CHANNEL_COUNT`で指定した数のチャンネルを初期化しますが、CH0のTCS34725のみでID(0x12)疎通確認を行います。他チャンネルのセンサー未接続を検知したい場合は、別途チャンネルごとのID確認処理を追加してください。
