# Board specific build rules

LIB_DIR      = lib/stm32f1xx
HAL_DIR      = $(LIB_DIR)/Drivers/STM32F1xx_HAL_Driver

LINKER_FILE  = $(LIB_DIR)/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/linker/$(patsubst "%",%,$(CONFIG_STM32F1XX_LINKER_FILE))_FLASH.ld
STARTUP_FILE = $(LIB_DIR)/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_$(patsubst "%",%,$(CONFIG_STM32F1XX_STARTUP_FILE)).s
SYSTEM_FILE  = $(LIB_DIR)/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c

USBLIBDIR    = $(LIB_DIR)/Middlewares/ST/STM32_USB_Device_Library

# Setup the toolchain
CROSS_PREFIX = arm-none-eabi-

OPTIM_LEV := -O2 #-g3
PROC_SPEC := -mcpu=cortex-m3 -mthumb -mlittle-endian # -mthumb-interwork

CFLAGS += $(PROC_SPEC) $(OPTIM_LEV)
CFLAGS += -ffunction-sections -fdata-sections  -fno-exceptions -fno-delete-null-pointer-checks
CFLAGS += -D__STM32F1XX__ -D$(patsubst "%",%,$(CONFIG_STM32F1XX_TYPE_DEFINE)) -DMCU_STM32F103C8
ifeq ($(CONFIG_STM32F1XX_SPI_CH),"SPI1")
  CFLAGS += -DSPI=1 -DSPI_REMAP=0
endif
ifeq ($(CONFIG_STM32F1XX_SPI_CH),"SPI1_REMAP")
  CFLAGS += -DSPI=1 -DSPI_REMAP=1
endif
ifeq ($(CONFIG_STM32F1XX_SPI_CH),"SPI2")
  CFLAGS += -DSPI=2 -DSPI_REMAP=0
endif

# include dirs
CFLAGS += -Isrc/stm32f1xx
CFLAGS += -I$(LIB_DIR)/Drivers/CMSIS/Device/ST/STM32F1xx/Include/
CFLAGS += -I$(LIB_DIR)/Drivers/CMSIS/Include/
CFLAGS += -I$(LIB_DIR)/Drivers/STM32F1xx_HAL_Driver/Inc/
CFLAGS += -I$(USBLIBDIR)/Core/Inc/
CFLAGS += -I$(USBLIBDIR)/Class/CDC/Inc/

ASFLAGS += $(PROC_SPEC) $(OPTIM_LEV)

CFLAGS_klipper.elf += $(PROC_SPEC)
CFLAGS_klipper.elf += -Wl,-Map=$(OUT)klipper.map
CFLAGS_klipper.elf += -Wl,--gc-sections
CFLAGS_klipper.elf += -T$(LINKER_FILE)
#CFLAGS_END_klipper.elf += --specs=nano.specs
#CFLAGS_END_klipper.elf += --specs=nosys.specs
#CFLAGS_END_klipper.elf += -lstdc++ -lsupc++ -lm -lgcc -lc -lnosys
#CFLAGS_END_klipper.elf += -lm -lgcc -lc -lnosys

# usb serial:
src-$(CONFIG_SERIAL) += ../$(USBLIBDIR)/Core/Src/usbd_core.c
src-$(CONFIG_SERIAL) += ../$(USBLIBDIR)/Core/Src/usbd_ctlreq.c
src-$(CONFIG_SERIAL) += ../$(USBLIBDIR)/Core/Src/usbd_ioreq.c
src-$(CONFIG_SERIAL) += ../$(USBLIBDIR)/Class/CDC/Src/usbd_cdc.c

# Add all processor specific source files
src-y += $(patsubst src/%,%,$(wildcard src/stm32f1xx/*.c))

# Add generic sources
src-y += generic/crc16_ccitt.c generic/alloc.c generic/timer_irq.c generic/armcm_irq.c

# Add driver sources
src-y += ../$(HAL_DIR)/Src/stm32f1xx_hal_rcc.c
src-y += ../$(HAL_DIR)/Src/stm32f1xx_hal_wwdg.c
src-y += ../$(SYSTEM_FILE)
src-y += ../$(STARTUP_FILE)

# Build the additional hex output file
target-y += $(OUT)klipper.bin
target-y += $(OUT)klipper.disasm

$(OUT)klipper.bin: $(OUT)klipper.elf
	@echo "  Creating hex file $@"
	$(Q)$(OBJCOPY) -O binary $< $@

$(OUT)klipper.disasm: $(OUT)klipper.elf
	@echo "  Creating disasm file $@"
	$(Q)$(OBJDUMP)  -d -f -M reg-names-std --demangle $< > $@

flash: $(OUT)klipper.bin
	@echo " [STM32F1XX] Sorry! Flashing is not supported yet"
