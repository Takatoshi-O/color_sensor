# color_sensor

`color_sensor` は、PCA9548A I2Cマルチプレクサ経由で複数のTCS34725カラーセンサーを読み取り、NVSからキャリブレーション値を読み込み、色判定を行うESP-IDFコンポーネントです。

## 構成

コンポーネントは3層に分かれています。

| 層 | ファイル | 役割 |
|---|---|---|
| ハードウェア | `color_sensor_hw.c/.h` | I2C、PCA9548Aチャンネル選択、TCS34725 RGBC読み取り |
| キャリブレーション | `color_calib.c/.h` | NVSを利用した基準値・黒閾値の管理 |
| 色判定 | `color_classify.c/.h` | RGB正規化と基準値との最近傍判定 |

## ハードウェア想定

Kconfigでは現在、以下を設定できます。

- I2Cポート
- SDA/SCL GPIO
- I2Cクロック周波数
- PCA9548Aアドレス
- 初期化するチャンネル数（1～8、0chから連番）

ハードウェア層では、TCS34725を読む前にPCA9548Aのチャンネルを選択します。

## Kconfig設定

`idf.py menuconfig` の **Component config -> Color Sensor Configuration** から設定します。

| 項目 | デフォルト | 説明 |
|---|---:|---|
| `COLOR_SENSOR_I2C_PORT` | 0 | I2Cポート番号 |
| `COLOR_SENSOR_SDA_GPIO` | 5 | I2C SDA GPIO |
| `COLOR_SENSOR_SCL_GPIO` | 6 | I2C SCL GPIO |
| `COLOR_SENSOR_I2C_FREQ_HZ` | 100000 | I2Cクロック周波数 |
| `COLOR_SENSOR_PCA9548_ADDR` | 0x70 | PCA9548Aアドレス |
| `COLOR_SENSOR_CHANNEL_COUNT` | 2 | 有効にするチャンネル数（0～count-1） |

## 初期化

一般的な初期化順序は次のとおりです。

```c
ESP_ERROR_CHECK(nvs_manager_init());

if (!color_sensor_hw_init()) {
    // ハードウェア初期化失敗
}

ESP_ERROR_CHECK(color_calib_init(CONFIG_COLOR_SENSOR_CHANNEL_COUNT));
```

`color_calib_init()` は、設定されたチャンネルについてキャリブレーション値をRAMキャッシュへ読み込みます。

## 色ID

`color_sensor_color_id_t` では16種類の色IDを定義しています。

`UNKNOWN`, `BLACK`, `WHITE`, `RED`, `GREEN`, `BLUE`, `YELLOW`, `ORANGE`, `PURPLE`, `CYAN`, `MAGENTA`, `BROWN`, `GRAY`, `PINK`, `LIME`, `NAVY`

列挙値はキャリブレーション配列のインデックスとしても使われるため、現在のデータ形式では並び順に意味があります。

## 色判定アルゴリズム

`color_classify_detect()` はまずClear値をチャンネルごとの黒判定閾値と比較します。閾値未満なら `COLOR_SENSOR_COLOR_BLACK` を返します。

黒以外ではRGBを合計1000となるよう正規化し、その値と保存されている基準ベクトルのユークリッド距離を比較します。最も近い基準色が判定結果になります。

組み込みの基準値は初期値なので、実際のセンサーと照明条件に合わせてキャリブレーション値を保存することを推奨します。

## キャリブレーションキャッシュ

`color_calib.c` は、正規化済みRGB基準値とチャンネルごとの黒閾値をRAMに保持します。

- `color_calib_get_refs()` : キャッシュされた基準値配列を取得
- `color_calib_get_black_threshold()` : 現在のClear閾値を取得
- `color_calib_reload()` : 指定チャンネルだけNVSから再読込

現在のソースでは、保存データがない場合の `DEFAULT_BLACK_THRESHOLD` は `30` です。

## NVS依存

永続化されたRGBCキャリブレーションデータには `nvs_manager` を使用します。現在のソースでは `NVS_MANAGER_COLOR_RGBC_COLOR_COUNT` と `COLOR_CALIB_COLOR_COUNT` がともに16であることをビルド時に検証しています。

## 公開API

### ハードウェア

- `color_sensor_hw_init()`
- `color_sensor_hw_read_channel()`

### キャリブレーション

- `color_calib_init()`
- `color_calib_get_refs()`
- `color_calib_get_black_threshold()`
- `color_calib_reload()`

### 色判定

- `color_classify_normalize_rgb()`
- `color_classify_detect()`

## 依存コンポーネント

- `esp_driver_i2c`
- `nvs_flash`
- `nvs_manager`（非公開依存）
