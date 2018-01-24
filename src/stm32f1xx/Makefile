# Additional stm32f1xx build rules

# Setup the toolchain
CROSS_PREFIX=arm-none-eabi-

dirs-y += src/stm32f1xx src/generic
dirs-y += lib/stm32/wirish

CFLAGS += -mthumb -mcpu=cortex-m3
CFLAGS += -Ilib/stm32/libmaple/include -Ilib/stm32/libmaple -Ilib/stm32/wirish
CFLAGS += -DMCU_STM32F103C8 -DF_CPU=72000000L -DSERIAL_USB -DGENERIC_BOOTLOADER -DVECT_TAB_ADDR=0x8002000

CFLAGS_klipper.elf += -L lib/stm32/wirish
CFLAGS_klipper.elf += -T lib/stm32/ld/generic_stm32f103c_flash_c8.ld -L lib/stm32/ld/ -lmaple -L lib/stm32
CFLAGS_klipper.elf += --specs=nano.specs --specs=nosys.specs

# Add source files
src-y += stm32f1xx/main.c stm32f1xx/timer.c stm32f1xx/gpio.c
src-y += generic/crc16_ccitt.c generic/alloc.c
src-y += generic/armcm_irq.c generic/timer_irq.c
src-$(CONFIG_SERIAL) += stm32f1xx/serial.c
srcxx-y += ../lib/stm32/wirish/boards.cpp ../lib/stm32/wirish/boards_setup.cpp
srcas-y += ../lib/stm32/wirish/start.S
src-y += ../lib/stm32/wirish/start_c.c
src-y += ../lib/stm32/wirish/syscalls.c

# Build the additional hex output file
target-y += $(OUT)klipper.bin

$(OUT)klipper.bin: $(OUT)klipper.elf
	@echo "  Creating hex file $@"
	$(Q)$(OBJCOPY) -O binary $< $@

flash: $(OUT)klipper.bin
	@echo "  Flashing $^ to $(FLASH_DEVICE) via bossac"
	$(Q)stty -F "$(FLASH_DEVICE)" 1200
	$(Q)bossac -i -p "$(FLASH_DEVICE:/dev/%=%)" -R -e -w -v -b $(OUT)klipper.bin
