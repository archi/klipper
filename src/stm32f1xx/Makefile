# Additional stm32f1xx build rules

# Setup the toolchain
CROSS_PREFIX=arm-none-eabi-

dirs-y += src/stm32f1xx src/generic
#not used anymore: dirs-y += lib/cmsis-stm32f1xx/source lib/cmsis-sam3x8e/source/gcc

CFLAGS += -mthumb -mcpu=cortex-m3
CFLAGS += -Ilib/stm32/libmaple/include -Ilib/stm32/libmaple
# TODO configure MCU in menuconfig; valid values are in lib/stm32/libmaple/stm32f1/include/series/stm32.h
CFLAGS += -DMCU_STM32F103C8

#not used anymore: CFLAGS_klipper.elf += -Llib/cmsis-sam3x8e/source/gcc
CFLAGS_klipper.elf += -T lib/cmsis-stm32f1xx/LinkerScript.ld
CFLAGS_klipper.elf += --specs=nano.specs --specs=nosys.specs

# Add source files
src-y += stm32f1xx/main.c stm32f1xx/timer.c stm32f1xx/gpio.c
src-y += generic/crc16_ccitt.c generic/alloc.c
src-y += generic/armcm_irq.c generic/timer_irq.c
#src-y += ../lib/cmsis-stm32f1xx/source/system_sam3xa.c
#src-y += ../lib/cmsis-stm32f1xx/source/gcc/startup_sam3xa.c
src-$(CONFIG_SERIAL) += stm32f1xx/serial.c

# Build the additional hex output file
target-y += $(OUT)klipper.bin

$(OUT)klipper.bin: $(OUT)klipper.elf
	@echo "  Creating hex file $@"
	$(Q)$(OBJCOPY) -O binary $< $@

flash: $(OUT)klipper.bin
	@echo "  Flashing $^ to $(FLASH_DEVICE) via bossac"
	$(Q)stty -F "$(FLASH_DEVICE)" 1200
	$(Q)bossac -i -p "$(FLASH_DEVICE:/dev/%=%)" -R -e -w -v -b $(OUT)klipper.bin
