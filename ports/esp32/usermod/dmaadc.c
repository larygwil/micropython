/*
 * This file is part of the micropython-usermod project, 
 *
 * https://github.com/v923z/micropython-usermod
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Zoltán Vörös
*/
    
#include "py/obj.h"
#include "py/runtime.h"

void s1(void);
void s2(void);
void s3(void);
void s4(void);
void s5(void);

STATIC mp_obj_t dmaadc_s1(/*mp_obj_t a_obj, mp_obj_t b_obj*/) {
    // int a = mp_obj_get_int(a_obj);
    // int b = mp_obj_get_int(b_obj);
    s1();
    // return mp_obj_new_int(a + b);
    return mp_obj_new_int(0);
};

STATIC mp_obj_t dmaadc_s2() {
    s2();
    return mp_obj_new_int(0);
};
STATIC mp_obj_t dmaadc_s3() {
    s3();
    return mp_obj_new_int(0);
};
STATIC mp_obj_t dmaadc_s4() {
    s4();
    return mp_obj_new_int(0);
};
STATIC mp_obj_t dmaadc_s5() {
    s5();
    return mp_obj_new_int(0);
};



STATIC MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_s1_obj, dmaadc_s1);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_s2_obj, dmaadc_s2);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_s3_obj, dmaadc_s3);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_s4_obj, dmaadc_s4);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_s5_obj, dmaadc_s5);

STATIC const mp_rom_map_elem_t dmaadc_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_dmaadc) },
    { MP_ROM_QSTR(MP_QSTR_s1), MP_ROM_PTR(&dmaadc_s1_obj) },
    { MP_ROM_QSTR(MP_QSTR_s2), MP_ROM_PTR(&dmaadc_s2_obj) },
    { MP_ROM_QSTR(MP_QSTR_s3), MP_ROM_PTR(&dmaadc_s3_obj) },
    { MP_ROM_QSTR(MP_QSTR_s4), MP_ROM_PTR(&dmaadc_s4_obj) },
    { MP_ROM_QSTR(MP_QSTR_s5), MP_ROM_PTR(&dmaadc_s5_obj) },
};
STATIC MP_DEFINE_CONST_DICT(dmaadc_module_globals, dmaadc_module_globals_table);

const mp_obj_module_t dmaadc_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&dmaadc_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_dmaadc, dmaadc_user_cmodule);


/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/adc.h"

#define TIMES              256
#define GET_UNIT(x)        ((x>>3) & 0x1)



#define ADC_RESULT_BYTE     4
#define ADC_CONV_LIMIT_EN   0
#define ADC_CONV_MODE       ADC_CONV_ALTER_UNIT     //ESP32C3 only supports alter mode
#define ADC_OUTPUT_TYPE     ADC_DIGI_OUTPUT_FORMAT_TYPE2



; ;
static uint16_t adc1_chan_mask = BIT(2) | BIT(3);
static uint16_t adc2_chan_mask = 0;
static adc_channel_t        channel[2] = {ADC1_CHANNEL_2, ADC1_CHANNEL_3};



static void call_adc_digi_initialize(
    uint32_t  max_store_buf_size,   // 例中默认1024
    uint32_t  conv_num_each_intr,     // 例中默认TIMES (=256)
    uint16_t adc1_chan_mask, 
    uint16_t adc2_chan_mask
)
{
    adc_digi_init_config_t   adc_dma_config = {
        .max_store_buf_size = max_store_buf_size,
        .conv_num_each_intr = conv_num_each_intr,
        .adc1_chan_mask = adc1_chan_mask,
        .adc2_chan_mask = adc2_chan_mask,
    };
    ESP_ERROR_CHECK(  adc_digi_initialize(&adc_dma_config));
}

// static void call_adc_digi_controller_configure( bool  conv_limit_en, uint32_t  conv_limit_num, adc_channel_t *  channel, uint8_t channel_num );

static void call_adc_digi_controller_configure(
    bool  conv_limit_en, 
    uint32_t  conv_limit_num, // 例默认250
    adc_channel_t *  channel, 
    uint8_t channel_num
)
{
    adc_digi_configuration_t   dig_cfg = {
        .conv_limit_en = conv_limit_en,
        .conv_limit_num = conv_limit_num,
        .sample_freq_hz = 10 * 1000,
        .conv_mode = ADC_CONV_MODE,
        .format = ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t   adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    
    // 编样式表 
    for (int i = 0; i < channel_num; i++) {
        uint8_t unit = GET_UNIT(channel[i]);
        uint8_t ch = channel[i] & 0x7;
        adc_pattern[i].atten = ADC_ATTEN_DB_11;
        adc_pattern[i].channel = ch;
        adc_pattern[i].unit = unit;
        adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

        printf( "adc_pattern[%d].atten is :%x \n", i, adc_pattern[i].atten);
        printf( "adc_pattern[%d].channel is :%x \n", i, adc_pattern[i].channel);
        printf( "adc_pattern[%d].unit is :%x \n", i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(   adc_digi_controller_configure(&dig_cfg));
}

static bool check_valid_data(const adc_digi_output_data_t *data)
{
    const unsigned int unit = data->type2.unit;
    if (unit > 2) return false;
    if (data->type2.channel >= SOC_ADC_CHANNEL_NUM(unit)) return false;

    return true;
}

static esp_err_t     ret;
static uint32_t     ret_num = 0;
static uint8_t     result[TIMES] = {0};

void s1(void)
{
    printf("memset() \n");
    memset(result, 0xcc, TIMES);
}

void s2(void)
{
    printf("call_adc_digi_initialize() \n");
    call_adc_digi_initialize(1024, TIMES, adc1_chan_mask, adc2_chan_mask );
}

void s3(void)
{
    printf("call_adc_digi_controller_configure \n");
    call_adc_digi_controller_configure (
        ADC_CONV_LIMIT_EN, 250, channel,   sizeof(channel) / sizeof(adc_channel_t) 
    );
}

void s4(void)
{
    printf("calling adc_digi_start() \n");
    adc_digi_start();
}

void s5(void)
{

    for (int j=0; j<2; j++) 
    {
        printf("calling adc_digi_read_bytes() \n");
        ret = adc_digi_read_bytes(result, TIMES, &ret_num, ADC_MAX_DELAY);
        if (ret == ESP_OK || ret == ESP_ERR_INVALID_STATE) {
            if (ret == ESP_ERR_INVALID_STATE) {
                printf("ret = ESP_ERR_INVALID_STATE \n");
                /**
                 * @note 1
                 * Issue:
                 * As an example, we simply print the result out, which is super slow. Therefore the conversion is too
                 * fast for the task to handle. In this condition, some conversion results lost.
                 *
                 * Reason:
                 * When this error occurs, you will usually see the task watchdog timeout issue also.
                 * Because the conversion is too fast, whereas the task calling `adc_digi_read_bytes` is slow.
                 * So `adc_digi_read_bytes` will hardly block. Therefore Idle Task hardly has chance to run. In this
                 * example, we add a `vTaskDelay(1)` below, to prevent the task watchdog timeout.
                 *
                 * Solution:
                 * Either decrease the conversion speed, or increase the frequency you call `adc_digi_read_bytes`
                 */
            }

            ESP_LOGI("TASK:", "ret is %x, ret_num is %d", ret, ret_num);
            for (int i = 0; i < ret_num; i += ADC_RESULT_BYTE) {
                adc_digi_output_data_t *p = (void*)&result[i];
                if (ADC_CONV_MODE == ADC_CONV_BOTH_UNIT || ADC_CONV_MODE == ADC_CONV_ALTER_UNIT) {
                    if (check_valid_data(p)) {
                        printf("ad%dch%d: %5u\t", p->type2.unit+1, p->type2.channel, p->type2.data);
                    } else {
                        // abort();
                        printf( "Invalid data [%d_%d_%x]\t", p->type2.unit+1, p->type2.channel, p->type2.data);
                    }
                }

            }
            printf("\n");
            //See `note 1`
            vTaskDelay(40);
        } else if (ret == ESP_ERR_TIMEOUT) {
            /**
             * ``ESP_ERR_TIMEOUT``: If ADC conversion is not finished until Timeout, you'll get this return error.
             * Here we set Timeout ``portMAX_DELAY``, so you'll never reach this branch.
             */
            printf( "No data, increase timeout or reduce conv_num_each_intr \n");
            vTaskDelay(1000);
        }

    }

    // adc_digi_stop();
    // ret = adc_digi_deinitialize();
    assert(ret == ESP_OK);
}