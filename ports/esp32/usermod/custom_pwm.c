//-----------------------------
static mp_obj_t mypm_ledc_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_pin_id,       MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_freq,         MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_duty_percent, MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 2} },
        { MP_QSTR_channel_id,   MP_ARG_REQUIRED | MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1} },
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
    uint32_t channel_id  = args[3].u_int;// ESP32C3:0~5
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
        .channel        = channel_id,
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
static mp_obj_t mypm_ledc_stop(mp_obj_t channel_id_obj, mp_obj_t timer_id_obj) // TODO 速度模式 、停后电压高低
{
    uint32_t channel_id = mp_obj_get_int(channel_id_obj);
    uint32_t timer_id = mp_obj_get_int(timer_id_obj);
    
    esp_err_t ret;
    
    ret = ledc_stop(LEDC_LOW_SPEED_MODE, channel_id, 0);
    if (ret != ESP_OK)
        return mp_const_false;
    
    
    ret = ledc_timer_pause(LEDC_LOW_SPEED_MODE, timer_id);
    if (ret != ESP_OK)
        return mp_const_false;
    
    ledc_timer_config_t ledc_timer = {
        .deconfigure = true, 
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = timer_id,
    };
    ret = ledc_timer_config(&ledc_timer);
    
    return (ret == ESP_OK) ? mp_const_true : mp_const_false;   
}
static MP_DEFINE_CONST_FUN_OBJ_2(mypm_ledc_stop_obj, mypm_ledc_stop); 
