idf_component_register(
    SRCS 
        "bt_mgr.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        bt              # ESP-IDF Bluetooth stack
        nvs_flash       # Required for NVS storing BT pairing state
)