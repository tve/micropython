SDKCONFIG += boards/sdkconfig.base
SDKCONFIG += boards/sdkconfig.240mhz
SDKCONFIG += boards/TOOTHLESS_OTA_8MB/sdkconfig.board

PART_SRC = boards/GENERIC_OTA_8MB/partitions-ota-8mb.csv

MICROPY_PY_BLUETOOTH = 0
