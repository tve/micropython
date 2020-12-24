freeze("$(PORT_DIR)/modules")
freeze("$(MPY_DIR)/../mqboard/mqtt_async", "mqtt_async.py")
freeze("$(MPY_DIR)/../mqboard/mqrepl", ["mqrepl.py", "watchdog.py"])
freeze("$(MPY_DIR)/../mqboard/board", ["mqtt.py", "logging.py"])
freeze("$(MPY_DIR)/../mpy-lib/sntp", "sntp.py")
freeze(
    "$(MPY_DIR)/../mpy-lib/u8g2",
    [
        "u8g2_font.py",
        "luRS12_te.py",
        "luRS18_te.py",
        "luRS24_te.py",
        "logisoso46_tn.py",
        "logisoso50_tn.py",
    ],
)
include("$(MPY_DIR)/extmod/uasyncio/manifest.py")
include("$(MPY_DIR)/extmod/webrepl/manifest.py")
