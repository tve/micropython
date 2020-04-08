MCU_SERIES = l0
CMSIS_MCU = STM32L072xx
MICROPY_FLOAT_IMPL = none
AF_FILE = boards/stm32l072_af.csv
LD_FILES = boards/JNZ5/stm32l072_fm.ld boards/common_basic.ld

# MicroPython settings
MICROPY_VFS_FAT = 0
MICROPY_VFS_LFS2 = 0

# Don't include default frozen modules because MCU is tight on flash space
FROZEN_MANIFEST ?=
