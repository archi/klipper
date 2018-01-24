# Additional stm32f1xx build rules

# Setup the toolchain
CROSS_PREFIX=arm-none-eabi-

dirs-y += src/stm32f1xx src/generic
dirs-y += lib/stm32duino/wirish
dirs-y += ../lib/stm32duino/wirish

CFLAGS += -mthumb -mcpu=cortex-m3
CFLAGS += -Ilib/stm32duino/libmaple/include -Ilib/stm32duino/libmaple/stm32f1/include -Ilib/stm32duino/libmaple -Ilib/stm32duino/wirish -I lib/stm32duino/ld
CFLAGS += -DMCU_STM32F103C8 -DF_CPU=72000000L -DSERIAL_USB -DGENERIC_BOOTLOADER -DVECT_TAB_ADDR=0x8002000 -DBOARD_HAVE_SERIALUSB -D__STM32F1__

CFLAGS_klipper.elf += -T lib/stm32duino/ld/bootloader_20.ld -L lib/stm32duino/support/ld
CFLAGS_klipper.elf += -L lib/stm32duino/support/ld/toolchains/generic -L lib/stm32duino/ld
CFLAGS_klipper.elf += -L lib/stm32duino/build
CFLAGS_klipper.elf += --specs=nano.specs --specs=nosys.specs
LDFLAGS_klipper.elf += -lmaple

# Add source files
src-y += stm32f1xx/main.c stm32f1xx/timer.c stm32f1xx/gpio.c
src-y += generic/crc16_ccitt.c generic/alloc.c
src-y += generic/armcm_irq.c generic/timer_irq.c
src-$(CONFIG_SERIAL) += stm32f1xx/serial.c
sry-cxx-y += ../lib/stm32duino/wirish/boards.cpp
src-cxx-y += ../lib/stm32duino/wirish/boards_setup.cpp
src-as-y += ../lib/stm32duino/wirish/start.S
src-y += ../lib/stm32duino/wirish/start_c.c
src-y += ../lib/stm32duino/wirish/syscalls.c

# Build the additional hex output file
target-y += $(OUT)klipper.bin

$(OUT)klipper.bin: $(OUT)klipper.elf
	@echo "  Creating hex file $@"
	$(Q)$(OBJCOPY) -O binary $< $@

flash: $(OUT)klipper.bin
	@echo "  Flashing $^ to $(FLASH_DEVICE) via bossac"
	$(Q)stty -F "$(FLASH_DEVICE)" 1200
	$(Q)bossac -i -p "$(FLASH_DEVICE:/dev/%=%)" -R -e -w -v -b $(OUT)klipper.bin
