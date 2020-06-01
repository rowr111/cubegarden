LIBVL53L1SRC += \
                 $(LIBVL53L1)/core/VL53L1X_api.c \
                 $(LIBVL53L1)/core/VL53L1X_calibration.c \
                 $(LIBVL53L1)/platform/vl53l1_platform.c \

LIBVL53L1INC += $(LIBVL53L1)/core \
                $(LIBVL53L1)/platform \

# change this to add configurational #defines to environment
LIBVL53L1DEFS += -DCPU_MK22FN128VLH10
USE_COPT += $(LIBVL53L1DEFS)
