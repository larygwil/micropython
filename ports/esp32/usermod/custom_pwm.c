//-----------------------------
static mp_obj_t mypm_ledc_init(mp_uint_t n_args, const mp_obj_t *args)
{// TODO 改为用有名传py参
    uint32_t pin_id = mp_obj_get_int(args[0]);
    uint32_t freq = mp_obj_get_int(args[1]);
    uint32_t duty_percent  = mp_obj_get_int(args[2]);//0~100
    uint32_t channel_id  = mp_obj_get_int(args[3]);// 0~5
    uint32_t timer_id = mp_obj_get_int(args[4]); // 0~3
    uint32_t reso = mp_obj_get_int(args[5]);   // 1~14
    uint32_t clk_src = mp_obj_get_int(args[6]); // enum
    
    uint32_t duty = (int) ((float)duty_percent/100*(1<<reso) );
    // 在 ESP32-C3 上，当通道绑定的定时器配置了其最大 PWM 占空比分辨率（ MAX_DUTY_RES ），通道的占空比不能被设置到 (2 ** MAX_DUTY_RES) 。否则，硬件内部占空比计数器会溢出，并导致占空比计算错误。
    
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
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
        .speed_mode     = LEDC_LOW_SPEED_MODE, // TODO
        .channel        = channel_id,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = timer_id,
        .duty           = duty, 
        .hpoint         = 0
        // TODO
        // .output_invert
    };
    ret = ledc_channel_config(&ledc_channel);
    return (ret == ESP_OK) ? mp_const_true : mp_const_false;
    
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mypm_ledc_init_obj, 7, 7, mypm_ledc_init);
//----------------------------
// ledc_timer_rst() ??
static mp_obj_t mypm_ledc_stop(mp_obj_t channel_id_obj, mp_obj_t timer_id_obj)
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
