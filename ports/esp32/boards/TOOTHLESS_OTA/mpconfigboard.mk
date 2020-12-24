SDKCONFIG += boards/sdkconfig.base
SDKCONFIG += boards/sdkconfig.240mhz
SDKCONFIG += boards/TOOTHLESS_OTA/sdkconfig.board

PART_SRC = partitions-ota.csv

MICROPY_PY_BLUETOOTH = 0
