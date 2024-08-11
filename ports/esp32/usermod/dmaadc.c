
#include "idf5.2_continuous_read_main.c"

// Include MicroPython API.
#include "py/runtime.h"

// Used to get the time in the Timer class example.
#include "py/mphal.h"


static mp_obj_t dmaadc_init1( 
    mp_uint_t n_args, const mp_obj_t *args
    // 因为参数个数超过3,所以下面这几个不直接使用
    // mp_obj_t read_len_obj, 
    // mp_obj_t buf_size_obj, 
    // mp_obj_t sample_freq_obj
    // mp_obj_t channel_num_obj
)
{
    // (void)n_args; // unused, we know it's 4
    read_len = mp_obj_get_int(args[0]);
    buf_size = mp_obj_get_int(args[1]);
    sample_freq = mp_obj_get_int(args[2]);
    channel_num = mp_obj_get_int(args[3]);
    mp_printf(&mp_plat_print, "read_len=%ld buf_size=%ld sample_freq=%ld channel_num=%ld\n", read_len, buf_size, sample_freq, channel_num);
    return mp_obj_new_int(0);
};
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(dmaadc_init1_obj, 4, 4, dmaadc_init1);
// 用 MP_DEFINE_CONST_FUN_OBJ_X （X=0~3）来设置参数个数，最大3。4或以上要用另一个

static mp_obj_t dmaadc_see_init1()
{
    mp_printf(&mp_plat_print, "read_len=%ld buf_size=%ld sample_freq=%ld channel_num=%ld\n", read_len, buf_size, sample_freq, channel_num);
    return mp_obj_new_int(0);
}
static MP_DEFINE_CONST_FUN_OBJ_0(dmaadc_see_init1_obj, dmaadc_see_init1);



static const mp_rom_map_elem_t dmaadc_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_dmaadc) },
    { MP_ROM_QSTR(MP_QSTR_init1), MP_ROM_PTR(&dmaadc_init1_obj) },
    { MP_ROM_QSTR(MP_QSTR_see_init1), MP_ROM_PTR(&dmaadc_see_init1_obj) },
};
static MP_DEFINE_CONST_DICT(dmaadc_module_globals, dmaadc_module_globals_table);

const mp_obj_module_t dmaadc_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&dmaadc_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_dmaadc, dmaadc_user_cmodule);

