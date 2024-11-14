/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */


/*
  针对ESP32C3修改
  例子默认原为IO2 和 IO3 作 ADC 共2通道数据 要改
  衰减需要改成11才能达到2.5V
  ADC1 共有5个通道 (ADC_UNIT_1 下的 ADC_CHANNEL_0 ~ ADC_CHANNEL_4)
  ADC2 共有1个通道。用WIFI时可能无法用。最近版IDF默认已禁用，想用要特别开启
*/

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_adc/adc_continuous.h"

// ADC_UNIT_1 下有 ADC_CHANNEL_0 ~ ADC_CHANNEL_4 共5个通道
#define EXAMPLE_ADC_UNIT                    ADC_UNIT_1
#define _EXAMPLE_ADC_UNIT_STR(unit)         #unit
#define EXAMPLE_ADC_UNIT_STR(unit)          _EXAMPLE_ADC_UNIT_STR(unit)
#define EXAMPLE_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1 // 只使用ADC1（5通道那个）一个而不用2
// 衰减（ATTEN）从0改成12可测2.5V （12就是11）
#define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_12
#define EXAMPLE_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH

// type1不是esp32c3型号的，c3只有type2
#define EXAMPLE_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type2.channel)
#define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type2.data)

// 原本为宏 EXAMPLE_READ_LEN (=256) 改为变量
static uint32_t read_len = 256;

// 原本为 adc_config 里硬编码的数 （ continuous_adc_init() 里用） 改为变量
static uint32_t buf_size = 1024;

// 原本为 dig_cfg 里硬编码的数 （ continuous_adc_init() 里用） 改为变量
static uint32_t sample_freq = 20*1000;

// 自加。原本是 continuous_adc_init() 的一个参数
static uint32_t channel_num = 5; 

// 原来的 channel 数组改名为 arr_channels
static adc_channel_t arr_channels[SOC_ADC_PATT_LEN_MAX] = {ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2, ADC_CHANNEL_3, ADC_CHANNEL_4};


// 原本是 main() 中的，改为全局。它会在 continuous_adc_init() 里被赋值。不知道在stop和deinit后会不会被NULL回来，或许之后应该手动NULL
static adc_continuous_handle_t handle = NULL;

static TaskHandle_t s_task_handle;
static const char *TAG = "EXAMPLE";

static uint32_t total_sample_cnt = 0;
static uint32_t last_datasize = 0;
static uint32_t last_dataaddr = 0;
// 转换完成回调函数 
static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    // 自己添加
    // 说明： main() 里是好像是先搬运一遍数据才解析，这里先尝试不搬运直接在原位置解析
    uint32_t datasize = edata->size;
    total_sample_cnt += datasize;
    last_datasize = datasize;
    // mp_printf(&mp_plat_print, "datasize: %ld\n", datasize);
    
    adc_digi_output_data_t *p = (adc_digi_output_data_t*) (edata->conv_frame_buffer) ;
    last_dataaddr = (uint32_t) p ;
    // for (int i = 0; i<3; i++) 
    // {
    //     uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
    //     uint32_t data = EXAMPLE_ADC_GET_DATA(p);
    //     mp_printf(&mp_plat_print, "    channel %ld : %ld\n", chan_num, data);
    //     p++;
    // } 
    
    // 结束自己添加
    
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(s_task_handle, &mustYield);
    return (mustYield == pdTRUE);
}

// 原本定义在 main() 里，改为全局
static adc_continuous_evt_cbs_t cbs = {
    .on_conv_done = s_conv_done_cb,
};


// 初始化连续采样
static void continuous_adc_init(adc_channel_t *arr_channels, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = buf_size,
        
        // This should be in multiples of SOC_ADC_DIGI_DATA_BYTES_PER_CONV (=4). 
        .conv_frame_size = read_len, 
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = sample_freq,
        .conv_mode = EXAMPLE_ADC_CONV_MODE,
        .format = EXAMPLE_ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = EXAMPLE_ADC_ATTEN;
        adc_pattern[i].channel = arr_channels[i] & 0x7;
        adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
        adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;

        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}

// 自己写的函数，用于看一些宏和数值到底是多少
static void seeinfo()
{
    ESP_LOGI("seeinfo()", "SOC_ADC_CHANNEL_NUM(EXAMPLE_ADC_UNIT)=%d", SOC_ADC_CHANNEL_NUM(EXAMPLE_ADC_UNIT)); // 应该输出5
    ESP_LOGI("seeinfo()", "SOC_ADC_DIGI_MAX_BITWIDTH=%d", SOC_ADC_DIGI_MAX_BITWIDTH); // 应该输出12
    ESP_LOGI("seeinfo()", "SOC_ADC_DIGI_RESULT_BYTES=%d", SOC_ADC_DIGI_RESULT_BYTES); // 应该输出4
    ESP_LOGI("seeinfo()", "SOC_ADC_PATT_LEN_MAX=%d", SOC_ADC_PATT_LEN_MAX); // 输出8
}

#ifdef NNNNNNNNNNNNNNNNNNNNNNNN
void app_main(void)
{
    seeinfo();
    
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[read_len];
    memset(result, 0xcc, read_len);

    continuous_adc_init(arr_channels, channel_num, &handle);

    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
    ESP_ERROR_CHECK(adc_continuous_start(handle));

    while (1) {

        /**
         * This is to show you the way to use the ADC continuous mode driver event callback.
         * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
         * However in this example, the data processing (print) is slow, so you barely block here.
         *
         * Without using this event callback (to notify this task), you can still just call
         * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
         */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        char unit[] = EXAMPLE_ADC_UNIT_STR(EXAMPLE_ADC_UNIT);

        while (1) {
            ret = adc_continuous_read(handle, result, read_len, &ret_num, 0);
            if (ret == ESP_OK) {
                ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32" bytes", ret, ret_num);
                for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
                    adc_digi_output_data_t *p = (adc_digi_output_data_t*)&result[i];
                    uint32_t chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
                    uint32_t data = EXAMPLE_ADC_GET_DATA(p);
                    /* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
                    if (chan_num < SOC_ADC_CHANNEL_NUM(EXAMPLE_ADC_UNIT)) {
                        ESP_LOGI(TAG, "Unit: %s, Channel: %"PRIu32", Value: %"PRIx32, unit, chan_num, data);
                    } else {
                        ESP_LOGW(TAG, "Invalid data [%s_%"PRIu32"_%"PRIx32"]", unit, chan_num, data);
                    }
                }
                /**
                 * Because printing is slow, so every time you call `ulTaskNotifyTake`, it will immediately return.
                 * To avoid a task watchdog timeout, add a delay here. When you replace the way you process the data,
                 * usually you don't need this delay (as this task will block for a while).
                 */
                vTaskDelay(1);
            } else if (ret == ESP_ERR_TIMEOUT) {
                //We try to read `read_len` until API returns timeout, which means there's no available data
                break;
            }
        }
    }

    ESP_ERROR_CHECK(adc_continuous_stop(handle));
    ESP_ERROR_CHECK(adc_continuous_deinit(handle));
}
#endif