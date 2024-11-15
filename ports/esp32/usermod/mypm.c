// Include MicroPython API.
#include "py/runtime.h"

// Used to get the time in the Timer class example.
#include "py/mphal.h"

#include "esp_check.h"
#include "esp_sleep.h"
#include "esp_pm.h"

#include "hal/clk_tree_hal.h"
#include "hal/clk_tree_ll.h"
#include "soc/rtc.h"
#include "hal/assert.h"
#include "hal/log.h"
#include "soc/clk_tree_defs.h"
#include "esp_rom_uart.h"
#include "driver/ledc.h"

#ifdef CONFIG_IDF_TARGET_ESP32
#include "driver/dac_cosine.h"
#endif


static mp_obj_t mypm_sleep_pd_config ( mp_obj_t domain_obj, mp_obj_t option_obj)
{
    int32_t domain = mp_obj_get_int(domain_obj);
    int32_t option = mp_obj_get_int(option_obj);
    
    esp_err_t ret = esp_sleep_pd_config(domain, option);
    
    return (ret == ESP_OK) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mypm_sleep_pd_config_obj, mypm_sleep_pd_config);
//----------------------------------------------
static mp_obj_t mypm_gpio_sleep_sel(mp_obj_t pin_num_obj, mp_obj_t en_obj)
{
    gpio_num_t pin_id = machine_pin_get_id(pin_num_obj);
    bool en = mp_obj_is_true(en_obj);
    
    esp_err_t ret;
    if (en)
        ret = gpio_sleep_sel_en(pin_id);
    else
        ret = gpio_sleep_sel_dis(pin_id);
    
    return (ret == ESP_OK) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_2(mypm_gpio_sleep_sel_obj, mypm_gpio_sleep_sel);
//----------------------------------------------
static mp_obj_t mypm_pm_configure(mp_obj_t max_freq_mhz_obj, mp_obj_t min_freq_mhz_obj, mp_obj_t light_sleep_enable_obj)
{
    int32_t max_freq_mhz = mp_obj_get_int(max_freq_mhz_obj);
    int32_t min_freq_mhz = mp_obj_get_int(min_freq_mhz_obj);
    bool light_sleep_enable = mp_obj_is_true(light_sleep_enable_obj);
    
    esp_pm_config_t pm;
    pm.max_freq_mhz = max_freq_mhz;
    pm.min_freq_mhz = min_freq_mhz;
    pm.light_sleep_enable = light_sleep_enable;
    
    esp_err_t ret = esp_pm_configure(&pm);
    
    return (ret == ESP_OK) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_3(mypm_pm_configure_obj, mypm_pm_configure);
//----------------------------------------------
static mp_obj_t mypm_lightsleep(mp_obj_t ms_obj)
{
    int32_t ms = mp_obj_get_int(ms_obj);
    esp_sleep_enable_timer_wakeup(((uint64_t)ms) * 1000);
    esp_light_sleep_start();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_lightsleep_obj, mypm_lightsleep);
//-------
static mp_obj_t mypm_deepsleep(mp_obj_t ms_obj)
{
    int32_t ms = mp_obj_get_int(ms_obj);
    esp_sleep_enable_timer_wakeup(((uint64_t)ms) * 1000);
    esp_deep_sleep_start();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_deepsleep_obj, mypm_deepsleep);
//--------------------------------------------
#ifdef CONFIG_IDF_TARGET_ESP32C3
static mp_obj_t mypm_sleep_cpu_retention(mp_obj_t en_obj)
{
    bool en = mp_obj_is_true(en_obj);
    
    esp_err_t ret;
    if (en)
        ret = esp_sleep_cpu_retention_init();
    else
        ret = esp_sleep_cpu_retention_deinit();
    
    return (ret == ESP_OK) ? mp_const_true : mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_sleep_cpu_retention_obj, mypm_sleep_cpu_retention);
#endif
//--------------------------------------------
static mp_obj_t mypm_clks_info()
{
    soc_cpu_clk_src_t cpu_source = clk_ll_cpu_get_src();
    char * cpu_source_print;
    switch(cpu_source) {
        case SOC_CPU_CLK_SRC_XTAL:  cpu_source_print = "XTAL" ; break;
        case SOC_CPU_CLK_SRC_PLL:  cpu_source_print = "PLL" ; break;
        case SOC_CPU_CLK_SRC_RC_FAST:  cpu_source_print = "RC_FAST" ; break;
        #ifdef CONFIG_IDF_TARGET_ESP32
        case SOC_CPU_CLK_SRC_APLL: cpu_source_print = "APLL" ; break;
        #endif
        default:                   cpu_source_print = "INVALID" ; break;
    }
    uint32_t cpu_freq = clk_hal_cpu_get_freq_hz();
    uint32_t cpu_source_root_freq = clk_hal_soc_root_get_freq_mhz(cpu_source);
    
    soc_rtc_fast_clk_src_t rtc_fast_source = clk_ll_rtc_fast_get_src();
    char * rtc_fast_source_print;
    switch(rtc_fast_source) {
        #ifdef CONFIG_IDF_TARGET_ESP32
        case SOC_RTC_FAST_CLK_SRC_XTAL_D4: rtc_fast_source_print = "XTAL_D4"; break;
        #endif

#ifdef CONFIG_IDF_TARGET_ESP32C3
        case SOC_RTC_FAST_CLK_SRC_XTAL_D2: rtc_fast_source_print = "XTAL_D2"; break;
#endif
        case SOC_RTC_FAST_CLK_SRC_RC_FAST: rtc_fast_source_print = "RC_FAST"; break;
        default:                           rtc_fast_source_print = "INVALID"; break;
    }
    
    soc_rtc_slow_clk_src_t rtc_slow_source = clk_ll_rtc_slow_get_src();
    char * rtc_slow_source_print;
    switch(rtc_slow_source) {
        case SOC_RTC_SLOW_CLK_SRC_RC_SLOW: rtc_slow_source_print = "RC_SLOW"; break;
        case SOC_RTC_SLOW_CLK_SRC_XTAL32K: rtc_slow_source_print = "XTAL32K"; break;
        case SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256: rtc_slow_source_print = "RC_FAST_D256"; break;
        default:                           rtc_slow_source_print = "INVALID"; break;
    }
    
    uint32_t ahb_freq = clk_hal_ahb_get_freq_hz();
    uint32_t xtal_freq = clk_hal_xtal_get_freq_mhz();
    uint32_t apb_freq = clk_ll_apb_load_freq_hz();
    uint32_t pll_freq = clk_ll_bbpll_get_freq_mhz();
    
    bool rc_fast_en =      clk_ll_rc_fast_is_enabled();
    bool rc_fast_digi_en = clk_ll_rc_fast_digi_is_enabled();
    bool rc_fast_d256_en = clk_ll_rc_fast_d256_is_enabled();
    bool xtal32k_en =      clk_ll_xtal32k_is_enabled();
    bool xtal32k_diti_en = clk_ll_xtal32k_digi_is_enabled();
    
    uint32_t cpu_divider = clk_ll_cpu_get_divider();
    uint32_t rc_fast_divider = clk_ll_rc_fast_get_divider();
    
    mp_printf(&mp_plat_print, "CPU频率：%ld \n CPU时钟源：%s (%ld) \t RTC_FAST时钟源：%s \t RTC_SLOW时钟源：%s \n RC_FAST状态：%d \t RC_FAST_DIGI状态：%d \t RC_FAST_D256状态：%d \t XTAL32K状态：%d \t XTAL32K_DIGI状态：%d \n AHB频率：%ld \t XTAL频率：%ld \t APB频率：%ld \t PLL频率:%ld \n CPU分频数：%ld \t RC_FAST分频数:%ld \n", 
              cpu_freq, 
              
              cpu_source_print, cpu_source_root_freq, 
              rtc_fast_source_print, 
              rtc_slow_source_print, 
              
              (int)rc_fast_en, 
              (int)rc_fast_digi_en, 
              (int)rc_fast_d256_en, 
              (int)xtal32k_en, 
              (int)xtal32k_diti_en, 
              
              ahb_freq, 
              xtal_freq, 
              apb_freq, 
              pll_freq, 
              
              cpu_divider, 
              rc_fast_divider
    );
    // mp_printf(&mp_plat_print, "read_len=%ld buf_size=%ld sample_freq=%ld channel_num=%ld\n", read_len, buf_size, sample_freq, channel_num);
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(mypm_clks_info_obj, mypm_clks_info);
//---------------------------------------------
static mp_obj_t mypm_set_bbpll_enabled(mp_obj_t en_obj)
{
    bool en = mp_obj_is_true(en_obj);
    if (en)
        clk_ll_bbpll_enable();
    else
        clk_ll_bbpll_disable();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_set_bbpll_enabled_obj, mypm_set_bbpll_enabled);
//---------------------------------------------
void rtc_clk_bbpll_configure(rtc_xtal_freq_t xtal_freq, int pll_freq);
// 这是来自 esp-idf/components/esp_hw_support/port/esp32[c3]/rtc_clk.c
// 在IDF 5.2中还需要把上面这个文件的这个函数定义的static去掉

// NOTE ESP32C3 实测改成320后wifi无法用
static mp_obj_t mypm_bbpll_set_freq(mp_obj_t freq_obj)
{
    uint32_t freq = mp_obj_get_int(freq_obj);
    // clk_ll_bbpll_set_freq_mhz(freq);
    rtc_clk_bbpll_configure(clk_hal_xtal_get_freq_mhz() , freq );
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_bbpll_set_freq_obj, mypm_bbpll_set_freq);
//---------------------------------------------
static mp_obj_t mypm_set_rc_fast_enabled(mp_obj_t en_obj)
{
    bool en = mp_obj_is_true(en_obj);
    if (en)
        clk_ll_rc_fast_enable();
    else
        clk_ll_rc_fast_disable();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_set_rc_fast_enabled_obj, mypm_set_rc_fast_enabled);
//--------------------------------
static mp_obj_t mypm_set_rc_fast_d256_enabled(mp_obj_t en_obj)
{
    bool en = mp_obj_is_true(en_obj);
    if (en)
        clk_ll_rc_fast_d256_enable();
    else
        clk_ll_rc_fast_d256_disable();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_set_rc_fast_d256_enabled_obj, mypm_set_rc_fast_d256_enabled);
//--------------------------------
static mp_obj_t mypm_set_rc_fast_digi_enabled(mp_obj_t en_obj)
{
    bool en = mp_obj_is_true(en_obj);
    if (en)
        clk_ll_rc_fast_digi_enable();
    else
        clk_ll_rc_fast_digi_disable();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_set_rc_fast_digi_enabled_obj, mypm_set_rc_fast_digi_enabled);
//--------------------------------
static mp_obj_t mypm_set_rc_fast_d256_digi_enabled(mp_obj_t en_obj)
{
    bool en = mp_obj_is_true(en_obj);
    if (en)
        clk_ll_rc_fast_d256_digi_enable();
    else
        clk_ll_rc_fast_d256_digi_disable();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_set_rc_fast_d256_digi_enabled_obj, mypm_set_rc_fast_d256_digi_enabled);
//---------------------------------------------------------
static mp_obj_t mypm_cpu_set_src ( mp_obj_t clk_src_obj)
{
    int32_t clk_src = mp_obj_get_int(clk_src_obj);
    if (clk_src == SOC_CPU_CLK_SRC_RC_FAST) // 在py那边，要记得改CPU源为RC_FAST之前要打开RC_FAST_DIGI
    {
        // rtc_clk_cpu_freq_to_8m(); // 这个是static不能从外部调用
        rtc_cpu_freq_config_t config = {
            SOC_CPU_CLK_SRC_RC_FAST, 
            0, 0, 0 // 无用，占位
        };
        rtc_clk_cpu_freq_set_config(&config);
    } 
    else // 如果想设成XTAL或PLL，不要用这里了！在py那边用machine.freq()，比直接这里操作底层好
    {
        clk_ll_cpu_set_src(clk_src);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_cpu_set_src_obj, mypm_cpu_set_src);
//---------------------------------------------------------
static mp_obj_t mypm_rtc_slow_set_src ( mp_obj_t clk_src_obj)
{
    int32_t clk_src = mp_obj_get_int(clk_src_obj);
    clk_ll_rtc_slow_set_src(clk_src);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_rtc_slow_set_src_obj, mypm_rtc_slow_set_src);
//---------------------------------------------------------
static mp_obj_t mypm_rtc_fast_set_src ( mp_obj_t clk_src_obj)
{
    int32_t clk_src = mp_obj_get_int(clk_src_obj);
    clk_ll_rtc_fast_set_src(clk_src);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_rtc_fast_set_src_obj, mypm_rtc_fast_set_src);
//--------------------------------------------------------------------
static mp_obj_t mypm_rc_fast_set_divider ( mp_obj_t divider_obj)
{
    uint32_t divider = mp_obj_get_int(divider_obj);
    #ifdef CONFIG_IDF_TARGET_ESP32
    clk_ll_rc_fast_set_divider(divider);
    #endif
    #ifdef CONFIG_IDF_TARGET_ESP32C3
    rtc_clk_8m_divider_set(divider);
    #endif
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_rc_fast_set_divider_obj, mypm_rc_fast_set_divider);
//--------------------------------------------------------------------
#ifdef CONFIG_IDF_TARGET_ESP32C3
static mp_obj_t mypm_rc_slow_set_divider ( mp_obj_t divider_obj)
{
    uint32_t divider = mp_obj_get_int(divider_obj);
    // clk_ll_rc_slow_set_divider(divider);
    rtc_clk_divider_set(divider);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_rc_slow_set_divider_obj, mypm_rc_slow_set_divider);
#endif
//--------------------------------------------------------------------
static mp_obj_t mypm_cpu_set_divider ( mp_obj_t divider_obj)
{
    uint32_t divider = mp_obj_get_int(divider_obj);
    clk_ll_cpu_set_divider(divider);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_cpu_set_divider_obj, mypm_cpu_set_divider);
//-----------------------------
static mp_obj_t mypm_wait_uart_tx(mp_obj_t uart_num_obj)
{
    uint8_t uart_num = mp_obj_get_int(uart_num_obj);
    esp_rom_uart_tx_wait_idle(uart_num);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(mypm_wait_uart_tx_obj, mypm_wait_uart_tx);
//-------------------
#include "custom_pwm.c"
//========================================================

// 所有函数要加进这里面
static const mp_rom_map_elem_t mypm_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_mypm) },
                                         
    // esp_sleep_pd_domain_t （睡眠时可以保持开启的那些域的名称）
    { MP_ROM_QSTR(MP_QSTR_PD_DOMAIN_XTAL), MP_ROM_INT(ESP_PD_DOMAIN_XTAL) },
    { MP_ROM_QSTR(MP_QSTR_PD_DOMAIN_RC_FAST), MP_ROM_INT(ESP_PD_DOMAIN_RC_FAST) },
    { MP_ROM_QSTR(MP_QSTR_PD_DOMAIN_VDDSDIO), MP_ROM_INT(ESP_PD_DOMAIN_VDDSDIO) },
    { MP_ROM_QSTR(MP_QSTR_PD_DOMAIN_MAX), MP_ROM_INT(ESP_PD_DOMAIN_MAX) },
    #ifdef CONFIG_IDF_TARGET_ESP32
    { MP_ROM_QSTR(MP_QSTR_PD_DOMAIN_RTC_PERIPH), MP_ROM_INT(ESP_PD_DOMAIN_RTC_PERIPH) },
    { MP_ROM_QSTR(MP_QSTR_PD_DOMAIN_RTC_SLOW_MEM), MP_ROM_INT(ESP_PD_DOMAIN_RTC_SLOW_MEM) },
    { MP_ROM_QSTR(MP_QSTR_PD_DOMAIN_RTC_FAST_MEM), MP_ROM_INT(ESP_PD_DOMAIN_RTC_FAST_MEM) },
    { MP_ROM_QSTR(MP_QSTR_PD_DOMAIN_MODEM), MP_ROM_INT(ESP_PD_DOMAIN_MODEM) },
    #endif
    #ifdef CONFIG_IDF_TARGET_ESP32C3
    { MP_ROM_QSTR(MP_QSTR_PD_DOMAIN_CPU), MP_ROM_INT(ESP_PD_DOMAIN_CPU) },
    #endif
                                         
    // esp_sleep_pd_option_t （睡眠时设置是否开始的选项名称）
    { MP_ROM_QSTR(MP_QSTR_PD_OPTION_OFF), MP_ROM_INT(ESP_PD_OPTION_OFF) },
    { MP_ROM_QSTR(MP_QSTR_PD_OPTION_ON), MP_ROM_INT(ESP_PD_OPTION_ON) },
    { MP_ROM_QSTR(MP_QSTR_PD_OPTION_AUTO), MP_ROM_INT(ESP_PD_OPTION_AUTO) },
    
    // esp_sleep_pd_config()  (设置睡眠时哪些可以保持开启的函数)
    { MP_ROM_QSTR(MP_QSTR_sleep_pd_config), MP_ROM_PTR(&mypm_sleep_pd_config_obj) },
    
    // gpio_sleep_sel()  (设置某个GPIO在lightsleep时是否不改变状态)
    { MP_ROM_QSTR(MP_QSTR_gpio_sleep_sel), MP_ROM_PTR(&mypm_gpio_sleep_sel_obj) },
                                         
    // esp_pm_configure (设置CPU自动最大、最小频率、是否自动休眠)
    { MP_ROM_QSTR(MP_QSTR_pm_configure), MP_ROM_PTR(&mypm_pm_configure_obj) },
    
    // lightsleep和deepsleep。原MPY已有在machine里，但自己搞个可能不一样。deepsleep后不reset不知能不能用
    { MP_ROM_QSTR(MP_QSTR_lightsleep), MP_ROM_PTR(&mypm_lightsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_deepsleep), MP_ROM_PTR(&mypm_deepsleep_obj) },
    #ifdef CONFIG_IDF_TARGET_ESP32C3
    { MP_ROM_QSTR(MP_QSTR_sleep_cpu_retention), MP_ROM_PTR(&mypm_sleep_cpu_retention_obj) },
    #endif
    
    // 打印时钟和频率相关信息
    { MP_ROM_QSTR(MP_QSTR_clks_info), MP_ROM_PTR(&mypm_clks_info_obj) },
    
    // 启用/禁用 BBPLL
    { MP_ROM_QSTR(MP_QSTR_set_bbpll_enabled), MP_ROM_PTR(&mypm_set_bbpll_enabled_obj) },
    
    // 设置 BBPLL 频率 常数
    { MP_ROM_QSTR(MP_QSTR_PLL_320M_FREQ_MHZ), MP_ROM_INT(CLK_LL_PLL_80M_FREQ_MHZ) },
    { MP_ROM_QSTR(MP_QSTR_PLL_320M_FREQ_MHZ), MP_ROM_INT(CLK_LL_PLL_160M_FREQ_MHZ) },
    #ifdef CONFIG_IDF_TARGET_ESP32
    { MP_ROM_QSTR(MP_QSTR_PLL_320M_FREQ_MHZ), MP_ROM_INT(CLK_LL_PLL_240M_FREQ_MHZ) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_PLL_320M_FREQ_MHZ), MP_ROM_INT(CLK_LL_PLL_320M_FREQ_MHZ) },
    { MP_ROM_QSTR(MP_QSTR_PLL_480M_FREQ_MHZ), MP_ROM_INT(CLK_LL_PLL_480M_FREQ_MHZ) },
    // 设置 BBPLL 频率 函数
    { MP_ROM_QSTR(MP_QSTR_bbpll_set_freq), MP_ROM_PTR(&mypm_bbpll_set_freq_obj) },
    
    // 启用/禁用 RC_FAST
    { MP_ROM_QSTR(MP_QSTR_set_rc_fast_enabled), MP_ROM_PTR(&mypm_set_rc_fast_enabled_obj) },
    
    // 启用/禁用 RC_FAST_DIGI
    { MP_ROM_QSTR(MP_QSTR_set_rc_fast_digi_enabled), MP_ROM_PTR(&mypm_set_rc_fast_digi_enabled_obj) },
    
    // 启用/禁用 RC_FAST_D256
    { MP_ROM_QSTR(MP_QSTR_set_rc_fast_d256_enabled), MP_ROM_PTR(&mypm_set_rc_fast_d256_enabled_obj) },

    // 启用/禁用 RC_FAST_D256_DIGI
    { MP_ROM_QSTR(MP_QSTR_set_rc_fast_d256_digi_enabled), MP_ROM_PTR(&mypm_set_rc_fast_d256_digi_enabled_obj) },
    
    // 设置CPU时钟源的常数
    { MP_ROM_QSTR(MP_QSTR_SOC_CPU_CLK_SRC_XTAL), MP_ROM_INT(SOC_CPU_CLK_SRC_XTAL) },
    { MP_ROM_QSTR(MP_QSTR_SOC_CPU_CLK_SRC_PLL), MP_ROM_INT(SOC_CPU_CLK_SRC_PLL) },
    { MP_ROM_QSTR(MP_QSTR_SOC_CPU_CLK_SRC_RC_FAST), MP_ROM_INT(SOC_CPU_CLK_SRC_RC_FAST) },
    #ifdef CONFIG_IDF_TARGET_ESP32
    { MP_ROM_QSTR(MP_QSTR_SOC_CPU_CLK_SRC_APLL), MP_ROM_INT(SOC_CPU_CLK_SRC_APLL) },
    #endif
    // 设置CPU时钟源的函数
    { MP_ROM_QSTR(MP_QSTR_cpu_set_src), MP_ROM_PTR(&mypm_cpu_set_src_obj) },
    
    // 设置 RTC SLOW 时钟源的常数
    { MP_ROM_QSTR(MP_QSTR_SOC_RTC_SLOW_CLK_SRC_RC_SLOW), MP_ROM_INT(SOC_RTC_SLOW_CLK_SRC_RC_SLOW) },
    { MP_ROM_QSTR(MP_QSTR_SOC_RTC_SLOW_CLK_SRC_XTAL32K), MP_ROM_INT(SOC_RTC_SLOW_CLK_SRC_XTAL32K) },
    { MP_ROM_QSTR(MP_QSTR_SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256), MP_ROM_INT(SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256) },
    // 设置 RTC SLOW 时钟源的函数
    { MP_ROM_QSTR(MP_QSTR_rtc_slow_set_src), MP_ROM_PTR(&mypm_rtc_slow_set_src_obj) },
    
    // 设置 RTC FAST 时钟源的常数
    { MP_ROM_QSTR(MP_QSTR_SOC_RTC_FAST_CLK_SRC_RC_FAST), MP_ROM_INT(SOC_RTC_FAST_CLK_SRC_RC_FAST) },
    #ifdef CONFIG_IDF_TARGET_ESP32
    { MP_ROM_QSTR(MP_QSTR_SOC_RTC_FAST_CLK_SRC_XTAL_D4), MP_ROM_INT(SOC_RTC_FAST_CLK_SRC_XTAL_D4) },
    #endif
    #ifdef CONFIG_IDF_TARGET_ESP32C3
    { MP_ROM_QSTR(MP_QSTR_SOC_RTC_FAST_CLK_SRC_XTAL_D2), MP_ROM_INT(SOC_RTC_FAST_CLK_SRC_XTAL_D2) },
    #endif
    // 设置 RTC FAST 时钟源的函数
    { MP_ROM_QSTR(MP_QSTR_rtc_fast_set_src), MP_ROM_PTR(&mypm_rtc_fast_set_src_obj) },
    
    // 设置 RC FAST 分频数 
    { MP_ROM_QSTR(MP_QSTR_rc_fast_set_divider), MP_ROM_PTR(&mypm_rc_fast_set_divider_obj) },
    
    // 设置 RC SLOW 分频数 
    #ifdef CONFIG_IDF_TARGET_ESP32C3
    { MP_ROM_QSTR(MP_QSTR_rc_slow_set_divider), MP_ROM_PTR(&mypm_rc_slow_set_divider_obj) },
    #endif
    // 设置 CPU 分频数
    { MP_ROM_QSTR(MP_QSTR_cpu_set_divider), MP_ROM_PTR(&mypm_cpu_set_divider_obj) },
    
    // 等待UART发完
    { MP_ROM_QSTR(MP_QSTR_wait_uart_tx), MP_ROM_PTR(&mypm_wait_uart_tx_obj) },
    //=========================
    // PWM
    // 时钟源选择
    { MP_ROM_QSTR(MP_QSTR_LEDC_AUTO_CLK), MP_ROM_INT(LEDC_AUTO_CLK) },
    { MP_ROM_QSTR(MP_QSTR_LEDC_USE_APB_CLK), MP_ROM_INT(LEDC_USE_APB_CLK) },
    { MP_ROM_QSTR(MP_QSTR_LEDC_USE_RC_FAST_CLK), MP_ROM_INT(LEDC_USE_RC_FAST_CLK) },
    #ifdef CONFIG_IDF_TARGET_ESP32
    { MP_ROM_QSTR(MP_QSTR_LEDC_USE_REF_TICK), MP_ROM_INT(LEDC_USE_REF_TICK) },
    #endif
    #ifdef CONFIG_IDF_TARGET_ESP32C3
    { MP_ROM_QSTR(MP_QSTR_LEDC_USE_XTAL_CLK), MP_ROM_INT(LEDC_USE_XTAL_CLK) },
    #endif
    
    // 注意与上面的有USE的不同。这些是用在ledc_timer_set()函数，而不是更常用的ledc_timer_config()函数
    #ifdef CONFIG_IDF_TARGET_ESP32
    { MP_ROM_QSTR(MP_QSTR_LEDC_REF_TICK), MP_ROM_INT(LEDC_REF_TICK) },
    #endif 
    { MP_ROM_QSTR(MP_QSTR_LEDC_APB_CLK), MP_ROM_INT(LEDC_APB_CLK) },
    { MP_ROM_QSTR(MP_QSTR_LEDC_SCLK), MP_ROM_INT(LEDC_SCLK) }, 
    
    // 速度模式 
    #ifdef CONFIG_IDF_TARGET_ESP32
    { MP_ROM_QSTR(MP_QSTR_LEDC_HIGH_SPEED_MODE), MP_ROM_INT(LEDC_HIGH_SPEED_MODE) },
    #endif
    { MP_ROM_QSTR(MP_QSTR_LEDC_LOW_SPEED_MODE), MP_ROM_INT(LEDC_LOW_SPEED_MODE) },
    { MP_ROM_QSTR(MP_QSTR_LEDC_SPEED_MODE_MAX), MP_ROM_INT(LEDC_SPEED_MODE_MAX) },
    
    // 方向
    { MP_ROM_QSTR(MP_QSTR_LEDC_DUTY_DIR_DECREASE), MP_ROM_INT(LEDC_DUTY_DIR_DECREASE) },
    { MP_ROM_QSTR(MP_QSTR_LEDC_DUTY_DIR_INCREASE), MP_ROM_INT(LEDC_DUTY_DIR_INCREASE) },
    { MP_ROM_QSTR(MP_QSTR_LEDC_DUTY_DIR_MAX), MP_ROM_INT(LEDC_DUTY_DIR_MAX) },
    
    //中断
    { MP_ROM_QSTR(MP_QSTR_LEDC_INTR_DISABLE), MP_ROM_INT(LEDC_INTR_DISABLE) },
    { MP_ROM_QSTR(MP_QSTR_LEDC_INTR_FADE_END), MP_ROM_INT(LEDC_INTR_FADE_END) },
    { MP_ROM_QSTR(MP_QSTR_LEDC_INTR_MAX), MP_ROM_INT(LEDC_INTR_MAX) },
    
    // LEDC 函数
    { MP_ROM_QSTR(MP_QSTR_ledc_init), MP_ROM_PTR(&mypm_ledc_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_ledc_stop), MP_ROM_PTR(&mypm_ledc_stop_obj) },
    
    //=================
    // DAC 
    #ifdef CONFIG_IDF_TARGET_ESP32
    // DAC Cosine 时钟源
    { MP_ROM_QSTR(MP_QSTR_DAC_COSINE_CLK_SRC_RTC_FAST), MP_ROM_INT(DAC_COSINE_CLK_SRC_RTC_FAST) },
    { MP_ROM_QSTR(MP_QSTR_DAC_COSINE_CLK_SRC_DEFAULT), MP_ROM_INT(DAC_COSINE_CLK_SRC_DEFAULT) }, // 这个其实就是RTC_FAST
    // DAC Cosine 相位
    { MP_ROM_QSTR(MP_QSTR_DAC_COSINE_PHASE_0), MP_ROM_INT(DAC_COSINE_PHASE_0) },
    { MP_ROM_QSTR(MP_QSTR_DAC_COSINE_PHASE_180), MP_ROM_INT(DAC_COSINE_PHASE_180) },
    // DAC 衰减
    { MP_ROM_QSTR(MP_QSTR_DAC_COSINE_ATTEN_DEFAULT), MP_ROM_INT(DAC_COSINE_ATTEN_DEFAULT) }, // 与0同
    { MP_ROM_QSTR(MP_QSTR_DAC_COSINE_ATTEN_DB_0), MP_ROM_INT(DAC_COSINE_ATTEN_DB_0) },
    { MP_ROM_QSTR(MP_QSTR_DAC_COSINE_ATTEN_DB_6), MP_ROM_INT(DAC_COSINE_ATTEN_DB_6) },
    { MP_ROM_QSTR(MP_QSTR_DAC_COSINE_ATTEN_DB_12), MP_ROM_INT(DAC_COSINE_ATTEN_DB_12) },
    { MP_ROM_QSTR(MP_QSTR_DAC_COSINE_ATTEN_DB_18), MP_ROM_INT(DAC_COSINE_ATTEN_DB_18) },
    // DAC函数
    { MP_ROM_QSTR(MP_QSTR_dac_cosine_start), MP_ROM_PTR(&mypm_dac_cosine_start_obj) },
    #endif
    // { MP_ROM_QSTR(MP_QSTR_), MP_ROM_INT() },
};
static MP_DEFINE_CONST_DICT(mypm_module_globals, mypm_module_globals_table);

const mp_obj_module_t mypm_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mypm_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_mypm, mypm_user_cmodule); 
