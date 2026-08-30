# color_sensor

`color_sensor` is an ESP-IDF component for reading multiple TCS34725 color sensors through a PCA9548A I2C multiplexer, loading calibration data from NVS, and classifying measured colors.

## Architecture

The component is divided into three layers.

| Layer | Files | Responsibility |
|---|---|---|
| Hardware | `color_sensor_hw.c/.h` | I2C, PCA9548A channel selection, and TCS34725 RGBC reads |
| Calibration | `color_calib.c/.h` | NVS-backed reference and black-threshold management |
| Classification | `color_classify.c/.h` | RGB normalization and nearest-reference color classification |

## Hardware assumptions

The Kconfig currently provides settings for:

- I2C port
- SDA and SCL GPIOs
- I2C clock frequency
- PCA9548A address
- Number of channels to initialize (1-8, starting at channel 0)

The hardware layer selects a PCA9548A channel before reading a TCS34725 device.

## Kconfig

Open `idf.py menuconfig` and select **Component config -> Color Sensor Configuration**.

| Option | Default | Description |
|---|---:|---|
| `COLOR_SENSOR_I2C_PORT` | 0 | I2C port number |
| `COLOR_SENSOR_SDA_GPIO` | 5 | I2C SDA GPIO |
| `COLOR_SENSOR_SCL_GPIO` | 6 | I2C SCL GPIO |
| `COLOR_SENSOR_I2C_FREQ_HZ` | 100000 | I2C clock frequency |
| `COLOR_SENSOR_PCA9548_ADDR` | 0x70 | PCA9548A address |
| `COLOR_SENSOR_CHANNEL_COUNT` | 2 | Number of enabled channels (0..count-1) |

## Initialization

The normal startup order is:

```c
ESP_ERROR_CHECK(nvs_manager_init());

if (!color_sensor_hw_init()) {
    // Hardware initialization failed.
}

ESP_ERROR_CHECK(color_calib_init(CONFIG_COLOR_SENSOR_CHANNEL_COUNT));
```

`color_calib_init()` loads calibration data into an internal RAM cache for the configured channels.

## Color IDs

`color_sensor_color_id_t` defines 16 IDs:

`UNKNOWN`, `BLACK`, `WHITE`, `RED`, `GREEN`, `BLUE`, `YELLOW`, `ORANGE`, `PURPLE`, `CYAN`, `MAGENTA`, `BROWN`, `GRAY`, `PINK`, `LIME`, `NAVY`.

The enum values are also used as indices into the calibration arrays, so their order is part of the current data format.

## Classification algorithm

`color_classify_detect()` first checks the Clear value against the per-channel black threshold. Values below that threshold are classified as `COLOR_SENSOR_COLOR_BLACK`.

For non-black samples, RGB is normalized so that the three channels sum to 1000, then the normalized sample is compared with the stored reference vectors using Euclidean distance. The nearest reference determines the result.

The built-in reference values are starting points; real calibration data should be stored for the actual sensors and lighting conditions.

## Calibration cache

`color_calib.c` maintains a RAM cache containing normalized RGB references and one black threshold per channel.

- `color_calib_get_refs()` returns the cached reference array.
- `color_calib_get_black_threshold()` returns the current Clear threshold.
- `color_calib_reload()` reloads one channel from NVS and refreshes the cache.

The current source uses `DEFAULT_BLACK_THRESHOLD` = `30` when no saved calibration data is available.

## NVS dependency

The component uses `nvs_manager` for persistent RGBC calibration data. The project currently requires `NVS_MANAGER_COLOR_RGBC_COLOR_COUNT` to match `COLOR_CALIB_COLOR_COUNT`, which is 16.

## Public API

### Hardware

- `color_sensor_hw_init()`
- `color_sensor_hw_read_channel()`

### Calibration

- `color_calib_init()`
- `color_calib_get_refs()`
- `color_calib_get_black_threshold()`
- `color_calib_reload()`

### Classification

- `color_classify_normalize_rgb()`
- `color_classify_detect()`

## Dependencies

- `esp_driver_i2c`
- `nvs_flash`
- `nvs_manager` (private dependency)
