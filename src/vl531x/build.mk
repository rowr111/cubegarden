LIBVL53L1SRC += \
                 $(LIBVL53L1)/core/vl53l1_api.c \
                 $(LIBVL53L1)/core/vl53l1_api_calibration.c \
                 $(LIBVL53L1)/core/vl53l1_api_core.c \
                 $(LIBVL53L1)/core/vl53l1_api_debug.c \
                 $(LIBVL53L1)/core/vl53l1_api_preset_modes.c \
                 $(LIBVL53L1)/core/vl53l1_api_strings.c \
                 $(LIBVL53L1)/core/vl53l1_core.c \
                 $(LIBVL53L1)/core/vl53l1_core_support.c \
                 $(LIBVL53L1)/core/vl53l1_silicon_core.c \
                 $(LIBVL53L1)/core/vl53l1_error_strings.c \
                 $(LIBVL53L1)/core/vl53l1_register_funcs.c \
                 $(LIBVL53L1)/core/vl53l1_wait.c \
                 $(LIBVL53L1)/platform/vl53l1_platform.c \

LIBVL53L1INC += $(LIBVL53L1)/core \
                $(LIBVL53L1)/platform \

# change this to add configurational #defines to environment
LIBVL53L1DEFS += -DCPU_MK22FN128VLH10
USE_COPT += $(LIBVL53L1DEFS)
