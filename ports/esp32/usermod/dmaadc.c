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
void s3(void); //  call_adc_digi_controller_configure
void s4(void);
// void s5(void);

STATIC mp_obj_t dmaadc_s1(/*mp_obj_t a_obj, mp_obj_t b_obj*/) {
    // int a = mp_obj_get_int(a_obj);
    // int b = mp_obj_get_int(b_obj);
    s1(); // memset()
    // return mp_obj_new_int(a + b);
    return mp_obj_new_int(0);
};


// call_adc_digi_initialize
// buffer大小（不知道是哪里）   每次产生中断前的转换次数（重要）
STATIC mp_obj_t dmaadc_s2() {
    s2();
    return mp_obj_new_int(0);
};

// call_adc_digi_controller_configure
// 频率   转换次数限制及其使能   样式表
STATIC mp_obj_t dmaadc_s3() {
    s3();   
    return mp_obj_new_int(0);
};
STATIC mp_obj_t dmaadc_s4() {
    s4(); // adc_digi_start(void) 
    return mp_obj_new_int(0);
};
// STATIC mp_obj_t dmaadc_s5() {
//     s5();
//     return mp_obj_new_int(0);
// };



STATIC MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_s1_obj, dmaadc_s1);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_s2_obj, dmaadc_s2);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_s3_obj, dmaadc_s3);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_s4_obj, dmaadc_s4);
// STATIC MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_s5_obj, dmaadc_s5);

STATIC const mp_rom_map_elem_t dmaadc_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_dmaadc) },
    { MP_ROM_QSTR(MP_QSTR_s1), MP_ROM_PTR(&dmaadc_s1_obj) },
    { MP_ROM_QSTR(MP_QSTR_s2), MP_ROM_PTR(&dmaadc_s2_obj) },
    { MP_ROM_QSTR(MP_QSTR_s3), MP_ROM_PTR(&dmaadc_s3_obj) },
    { MP_ROM_QSTR(MP_QSTR_s4), MP_ROM_PTR(&dmaadc_s4_obj) },
    // { MP_ROM_QSTR(MP_QSTR_s5), MP_ROM_PTR(&dmaadc_s5_obj) },
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
static uint16_t adc1_chan_mask = BIT(0)| BIT(1) | BIT(2) | BIT(3) | BIT(4) ;
static uint16_t adc2_chan_mask = 0; // 用了可能导致wifi冲突。可以后面在wifi启动之后再用它
static adc_channel_t        channel[5] = {ADC1_CHANNEL_0, ADC1_CHANNEL_1, ADC1_CHANNEL_2, ADC1_CHANNEL_3 , ADC1_CHANNEL_4};



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

static void call_adc_digi_controller_configure(    // s3()
    bool  conv_limit_en, 
    uint32_t  conv_limit_num, // 例默认250，未使能
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
