# Additional stm32f1xx build rules

# Setup the toolchain
CROSS_PREFIX=arm-none-eabi-

dirs-y += src/stm32f1xx src/generic
dirs-y += lib/libmaple/libmaple/

CFLAGS += -mthumb -mcpu=cortex-m3
CFLAGS += -Ilib/libmaple/libmaple/include -Ilib/libmaple/libmaple/stm32f1/include
CFLAGS += -DMCU_STM32F103C8 -DF_CPU=72000000L -DSERIAL_USB -DGENERIC_BOOTLOADER -DVECT_TAB_ADDR=0x8002000

CFLAGS_klipper.elf += -T lib/libmaple/support/ld/flash.ld -L lib/libmaple/support/ld -L lib/libmaple/support/ld/stm32/mem/sram_20k_flash_128k -L lib/libmaple/support/ld/toolchains/generic -L lib/libmaple/support/ld/stm32/series/stm32f1/performance/
CFLAGS_klipper.elf += -L lib/libmaple/build
CFLAGS_klipper.elf += --specs=nano.specs --specs=nosys.specs
LDFLAGS_klipper.elf += -lmaple

# Add source files
src-y += stm32f1xx/main.c stm32f1xx/timer.c stm32f1xx/gpio.c
src-y += generic/crc16_ccitt.c generic/alloc.c
src-y += generic/armcm_irq.c generic/timer_irq.c
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
