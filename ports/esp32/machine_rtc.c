/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 "Eric Poulsen" <eric@zyxod.com>
 * Copyright (c) 2017 "Tom Manning" <tom@manningetal.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>
#include "driver/gpio.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "lib/timeutils/timeutils.h"
#include "modmachine.h"
#include "machine_rtc.h"

typedef struct _machine_rtc_obj_t {
    mp_obj_base_t base;
} machine_rtc_obj_t;

/* There is 8K of rtc_slow_memory, but some is used by the system software
    If the MICROPY_HW_RTC_USER_MEM_MAX is set too high, the following compile error will happen:
        region `rtc_slow_seg' overflowed by N bytes
    The current system software allows almost 4096 to be used.
    To avoid running into issues if the system software uses more, 2048 was picked as a max length

    You can also change this max length at compile time by defining MICROPY_HW_RTC_USER_MEM_MAX
    either on your make line, or in your board config.

    If MICROPY_HW_RTC_USER_MEM_MAX is set to 0, the RTC.memory() functionality will be not
    be compiled which frees some extra flash and RTC memory.
*/
#ifndef MICROPY_HW_RTC_USER_MEM_MAX
#define MICROPY_HW_RTC_USER_MEM_MAX     2048
#endif

#if MICROPY_HW_RTC_USER_MEM_MAX > 0 && MICROPY_PY_UZLIB > 0
#include "extmod/uzlib/tinf.h"
#endif

// Optionally compile user memory functionality if the size of memory is greater than 0
#if MICROPY_HW_RTC_USER_MEM_MAX > 0
#define MEM_MAGIC           0x75507921
RTC_NOINIT_ATTR uint32_t rtc_user_mem_magic;
RTC_NOINIT_ATTR uint16_t rtc_user_mem_len;
#if MICROPY_PY_UZLIB
RTC_NOINIT_ATTR uint16_t rtc_user_mem_crc; // 32-bit CRC folded in half
#endif
RTC_NOINIT_ATTR uint8_t rtc_user_mem_data[MICROPY_HW_RTC_USER_MEM_MAX];
#endif

// singleton RTC object
STATIC const machine_rtc_obj_t machine_rtc_obj = {{&machine_rtc_type}};

machine_rtc_config_t machine_rtc_config = {
    .ext1_pins = 0,
    .ext0_pin = -1
};

// forward decl
STATIC mp_obj_t machine_rtc_init_helper(mp_obj_t self, mp_uint_t n_args, const mp_obj_t *args);

STATIC mp_obj_t machine_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    // get constant object
    mp_obj_t self = (mp_obj_t)&machine_rtc_obj;

    // call init
    if (n_args == 0) {
        machine_rtc_init_helper(self, 0, NULL);
    } else {
        machine_rtc_init_helper(self, n_args-1, args+1);
    }

    return self;
}
STATIC mp_obj_t machine_rtc_init(mp_uint_t n_args, const mp_obj_t *args) {
    return machine_rtc_init_helper(args[0], n_args - 1, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_rtc_init_obj, 1, 2, machine_rtc_init);

STATIC mp_obj_t machine_rtc_datetime_helper(mp_obj_t self, mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args == 1) {
        // Get time

        struct timeval tv;

        gettimeofday(&tv, NULL);
        timeutils_struct_time_t tm;

        timeutils_seconds_since_epoch_to_struct_time(tv.tv_sec, &tm);

        mp_obj_t tuple[8] = {
            mp_obj_new_int(tm.tm_year),
            mp_obj_new_int(tm.tm_mon),
            mp_obj_new_int(tm.tm_mday),
            mp_obj_new_int(tm.tm_wday),
            mp_obj_new_int(tm.tm_hour),
            mp_obj_new_int(tm.tm_min),
            mp_obj_new_int(tm.tm_sec),
            mp_obj_new_int(tv.tv_usec)
        };

        return mp_obj_new_tuple(8, tuple);
    } else {
        // Set time

        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[1], 8, &items);

        struct timeval tv = {0};
        tv.tv_sec = timeutils_seconds_since_epoch(mp_obj_get_int(items[0]), mp_obj_get_int(items[1]), mp_obj_get_int(items[2]), mp_obj_get_int(items[4]), mp_obj_get_int(items[5]), mp_obj_get_int(items[6]));
        tv.tv_usec = mp_obj_get_int(items[7]);
        settimeofday(&tv, NULL);

        return mp_const_none;
    }
}
STATIC mp_obj_t machine_rtc_datetime(mp_uint_t n_args, const mp_obj_t *args) {
    return machine_rtc_datetime_helper(args[0], n_args-1, args+1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_rtc_datetime_obj, 1, 2, machine_rtc_datetime);

#if MICROPY_PY_UZLIB
STATIC uint16_t machine_rtc_memcrc(const uint8_t *data) {
    if (data == NULL) data = rtc_user_mem_data;
    uint32_t calc_crc = uzlib_crc32(&rtc_user_mem_magic, sizeof(rtc_user_mem_magic), 0xffffffff);
    calc_crc = uzlib_crc32(&rtc_user_mem_len, sizeof(rtc_user_mem_len), calc_crc);
    calc_crc = uzlib_crc32(data, rtc_user_mem_len, calc_crc);
    return (calc_crc & 0xffff) ^ (calc_crc >> 16);
}
#endif

STATIC mp_obj_t machine_rtc_init_helper(mp_obj_t self, mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args > 0) {
        machine_rtc_datetime_helper(self, n_args, args);
    }

    #if MICROPY_HW_RTC_USER_MEM_MAX > 0
    uint8_t ok = rtc_user_mem_magic == MEM_MAGIC && rtc_user_mem_len <= MICROPY_HW_RTC_USER_MEM_MAX;
    #if MICROPY_PY_UZLIB
    ok = ok && (machine_rtc_memcrc(NULL) == rtc_user_mem_crc);
    #endif
    if (!ok) {
        rtc_user_mem_magic = 0;
        rtc_user_mem_len = 0;
    }
    #endif

    return mp_const_none;
}

#if MICROPY_HW_RTC_USER_MEM_MAX > 0
STATIC mp_obj_t machine_rtc_memory(mp_uint_t n_args, const mp_obj_t *args) {
    if (n_args == 1) {
        ulTaskNotifyTake(pdFALSE, 10);
        if (rtc_user_mem_len == 0) return mp_const_empty_bytes;
        // Read RTC memory -- There's a small chance that magic is valid, len is random and
        // CRC will fail. In that case we might allocate up to MICROPY_HW_RTC_USER_MEM_MAX
        // in vain, but doing the CRC first is very slow 'cause RTC RAM is slow.
        mp_obj_t buf = mp_obj_new_bytes(rtc_user_mem_data, rtc_user_mem_len);
        #if MICROPY_PY_UZLIB
        const byte *data = (const byte *)mp_obj_str_get_str(buf);
        if (machine_rtc_memcrc(data) != rtc_user_mem_crc) {
            rtc_user_mem_len = 0; // invalidate
            return mp_const_empty_bytes;
        }
        #endif
        return buf;
    } else {
        // write RTC memory
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_READ);

        if (bufinfo.len > MICROPY_HW_RTC_USER_MEM_MAX) {
            mp_raise_ValueError(MP_ERROR_TEXT("buffer too long"));
        }
        rtc_user_mem_magic = MEM_MAGIC;
        memcpy((char *)rtc_user_mem_data, (char *)bufinfo.buf, bufinfo.len);
        rtc_user_mem_len = bufinfo.len;
        #if MICROPY_PY_UZLIB
        rtc_user_mem_crc = machine_rtc_memcrc(bufinfo.buf);
        #endif
        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_rtc_memory_obj, 1, 2, machine_rtc_memory);
#endif

STATIC const mp_rom_map_elem_t machine_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_rtc_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&machine_rtc_datetime_obj) },
    #if MICROPY_HW_RTC_USER_MEM_MAX > 0
    { MP_ROM_QSTR(MP_QSTR_memory), MP_ROM_PTR(&machine_rtc_memory_obj) },
    #endif
};
STATIC MP_DEFINE_CONST_DICT(machine_rtc_locals_dict, machine_rtc_locals_dict_table);

const mp_obj_type_t machine_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .make_new = machine_rtc_make_new,
    .locals_dict = (mp_obj_t)&machine_rtc_locals_dict,
};
