FLASH_FREQ = 80m

SDKCONFIG += boards/sdkconfig.base
SDKCONFIG += boards/sdkconfig.240mhz
SDKCONFIG += boards/sdkconfig.spiram
SDKCONFIG += boards/TOOTHLESS_SPIRAM_OTA_8MB/sdkconfig.board

PART_SRC = boards/GENERIC_OTA_8MB/partitions-ota-8mb.csv

MICROPY_PY_BLUETOOTH = 0
