//-----------------------------
static mp_obj_t mypm_ledc_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_pin_id,       MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_freq,         MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_duty_percent, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 2} },
        { MP_QSTR_chan_id,      MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_timer_id  ,   MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        
        { MP_QSTR_speed_mode    , MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = LEDC_LOW_SPEED_MODE} },
        { MP_QSTR_reso          , MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8} }, // TODO 默认计算，非固定。最好不要用芯片允许的最大值
        { MP_QSTR_clk_src       , MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = LEDC_AUTO_CLK} },
        { MP_QSTR_output_invert , MP_ARG_KW_ONLY | MP_ARG_BOOL, {.u_bool = false} }, 
        { MP_QSTR_intr_type     , MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = LEDC_INTR_DISABLE} }, 
        { MP_QSTR_hpoint        , MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, 
    };
    
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    uint32_t pin_id = args[0].u_int;
    uint32_t freq = args[1].u_int;
    uint32_t duty_percent  = args[2].u_int;//0~100
    uint32_t chan_id  = args[3].u_int;// ESP32C3:0~5
    uint32_t timer_id = args[4].u_int; // ESP32C3:0~3
    uint32_t speed_mode = args[5].u_int; // enum
    uint32_t reso = args[6].u_int;   // ESP32C3:1~14
    uint32_t clk_src = args[7].u_int; // enum
    bool output_invert = args[8].u_bool; 
    uint32_t intr_type = args[9].u_int; // enum
    uint32_t hpoint = args[10].u_int; //  range is [0, (2**duty_resolution)-1]
    
    uint32_t duty = (int) ((float)duty_percent/100*(1<<reso) );
    // 在 ESP32-C3 上，当通道绑定的定时器配置了其最大 PWM 占空比分辨率（ MAX_DUTY_RES ），通道的占空比不能被设置到 (2 ** MAX_DUTY_RES) 。否则，硬件内部占空比计数器会溢出，并导致占空比计算错误。
    
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = speed_mode, 
        .duty_resolution  = reso,
        .timer_num        = timer_id,
        .freq_hz          = freq,  
        .clk_cfg          = clk_src // 这里使用的是有USE的
    };
    esp_err_t ret;
    
    ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK)
        return mp_const_false;
    
    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = pin_id,
        .speed_mode     = speed_mode, 
        .channel        = chan_id,
        .intr_type      = intr_type, 
        .timer_sel      = timer_id,
        .duty           = duty, 
        .hpoint         = hpoint, 
        .output_invert  = output_invert, 
    };
    ret = ledc_channel_config(&ledc_channel);
    return (ret == ESP_OK) ? mp_const_true : mp_const_false;
    
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mypm_ledc_init_obj, 5, mypm_ledc_init);
//----------------------------
// ledc_timer_rst() ??
static mp_obj_t mypm_ledc_stop(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_chan_id,      MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_timer_id  ,   MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_speed_mode,   MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_idle_level,  MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };
    
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    uint32_t chan_id = args[0].u_int;
    uint32_t timer_id = args[1].u_int;
    uint32_t speed_mode = args[2].u_int;
    uint32_t idle_level = args[3].u_int;
    
    esp_err_t ret;
    
    ret = ledc_stop(speed_mode, chan_id, idle_level);
    if (ret != ESP_OK)
        return mp_const_false;
    
    
    ret = ledc_timer_pause(speed_mode, timer_id);
    if (ret != ESP_OK)
        return mp_const_false;
    
    ledc_timer_config_t ledc_timer = {
        .deconfigure = true, 
        .speed_mode       = speed_mode,
        .timer_num        = timer_id,
    };
    ret = ledc_timer_config(&ledc_timer);
    
    return (ret == ESP_OK) ? mp_const_true : mp_const_false;   
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mypm_ledc_stop_obj, 3, mypm_ledc_stop);
//=========================
#ifdef CONFIG_IDF_TARGET_ESP32
static mp_obj_t mypm_dac_cosine_start(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_chan_id,   MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} }, // 0=gpio25 1=gpio26
        { MP_QSTR_freq,         MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        
        { MP_QSTR_atten       , MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DAC_COSINE_ATTEN_DEFAULT} },  // enum
        { MP_QSTR_offset       , MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} }, 
        { MP_QSTR_phase       , MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DAC_COSINE_PHASE_0} }, // enum
        { MP_QSTR_clk_src       , MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = DAC_COSINE_CLK_SRC_DEFAULT} }, // enum
        { MP_QSTR_force_freq ,  , MP_ARG_KW_ONLY | MP_ARG_BOOL {.u_bool= true} }, 
    };
    
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    uint32_t chan_id = args[0].u_int; // 0~1 0=gpio25 1=gpio26
    uint32_t freq = args[1].u_int;
    uint32_t atten = args[2].u_int; //enum
    uint32_t offset = args[3].u_int;
    uint32_t phase = args[4].u_int; //enum
    uint32_t clk_src = args[5].u_int; //enum
    uint32_t force_freq = args[6].u_bool; // 强制让另一通道频率也改变
    
    
    dac_cosine_handle_t chan_handle;
    dac_cosine_config_t cos_cfg = {
        .chan_id = chan_id, // int 0~1
        .freq_hz = freq, // It will be covered by 8000 in the latter configuration
        .clk_src = DAC_COSINE_CLK_SRC_DEFAULT,
        .offset = offset,
        .phase = phase,
        .atten = atten,
        .flags.force_set_freq = force_freq, // true的话也会强行改变另一通道的频率
    };
    
    esp_err_t ret;
    ret = dac_cosine_new_channel(&cos_cfg, &chan_handle);
    if (ret != ESP_OK)
        return mp_const_false;

    ret = dac_cosine_start(chan_handle);
    return (ret == ESP_OK) ? mp_const_true : mp_const_false;   
}
static MP_DEFINE_CONST_FUN_OBJ_KW(mypm_dac_cosine_start_obj, 2, mypm_dac_cosine_start);
#endif