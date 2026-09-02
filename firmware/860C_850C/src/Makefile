#  Copyright (C) 2015 Joerg Hoener
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.

ifndef DISPLAY_VERSION
  CFLAGS += -DDISPLAY_850C
  DISPLAY_VERSION_DEFINED = yes
  $(info WARNING | using default display version: DISPLAY_850C, no bootloader)
endif

ifeq ($(DISPLAY_VERSION), 850C_BOOTLOADER)
  CFLAGS += -DDISPLAY_850C
  BOOTLOADER = yes
  DISPLAY_VERSION_DEFINED = yes
endif

# Confirmed Green Pedel E2.3 hardware. Reuse the newer LF60 panel path and
# 104 MHz clock behavior from the BIKEL firmware that works on this HMI.
# The custom APT flasher writes byte zero of a raw image to 0x08004000, so
# this target must be linked to the same 16 KiB application origin.
ifeq ($(DISPLAY_VERSION), APT_850C_GD32F303RET6)
  CFLAGS += -DDISPLAY_850C -DDISPLAY_850C_LF60 -DTARGET_APT_850C_GD32F303RET6 -DGD32F30X_HD -DCONTROLLER_TX_LOCKED
  BOOTLOADER = yes
  TARGET_LFLAGS = -Xlinker --defsym=RESERVE_SETTINGS_PAGES=1
  TARGET_CPU = cortex-m4
  TARGET_CMSIS_INCLUDES = -I./gd32f30x_cmsis/Include -I./gd32f30x_cmsis/Core
  TARGET_SYSTEM_SOURCE = gd32f30x_cmsis/system_gd32f30x.c gd32f30x_cmsis/gd32f30x_adc.c gd32f30x_cmsis/gd32f30x_dma.c gd32f30x_cmsis/gd32f30x_gpio.c gd32f30x_cmsis/gd32f30x_misc.c gd32f30x_cmsis/gd32f30x_rcu.c gd32f30x_cmsis/gd32f30x_timer.c gd32f30x_cmsis/gd32f30x_usart.c
  TARGET_STARTUP_OBJECT = startup_gd32f30x_hd.o
  DISPLAY_VERSION_DEFINED = yes
  $(warning DEVELOPMENT ONLY: GD32F303 target uses working BIKEL LF60/clock assumptions with E2.3 0x08004000 origin)
endif

TARGET_CPU ?= cortex-m3
TARGET_STARTUP_OBJECT ?= startup_stm32f10x_md.o

ifeq ($(LCD_DIAGNOSTIC), yes)
  CFLAGS += -DLCD_BRINGUP_DIAGNOSTIC
endif

ifeq ($(DISPLAY_VERSION), 860C_BOOTLOADER)
  CFLAGS += -DDISPLAY_860C
  BOOTLOADER = yes
  DISPLAY_VERSION_DEFINED = yes
endif

ifneq ($(DISPLAY_VERSION_DEFINED), yes)
  $(error wrong DISPLAY_VERSION or not set)
else
  ifeq ($(DISPLAY_VERSION), 850C_BOOTLOADER)
  $(info WARNING | using display version: 850C_BOOTLOADER)
  endif
  ifeq ($(DISPLAY_VERSION), 860C_BOOTLOADER)
  $(info WARNING | using display version: 860C_BOOTLOADER)
  endif
  ifeq ($(DISPLAY_VERSION), APT_850C_GD32F303RET6)
  $(info WARNING | target: APT_850C_GD32F303RET6 at E2.3 offset 0x4000, LF60 LCD)
  endif
endif

ifdef BOOTLOADER
CFLAGS += -DUSE_WITH_BOOTLOADER
LFLAGS = -Xlinker --defsym=USE_WITH_BOOTLOADER=1
endif

TCPREFIX  = $(TCPREFIX)gcc
AS      = $(TCPREFIX)as
LD      = $(TCPREFIX)gcc -v
CP      = $(TCPREFIX)objcopy
OD      = $(TCPREFIX)objdump
GDB     = $(TCPREFIX)gdb
SIZE    = $(TCPREFIX)size

OPT = 0

AFLAGS  = -I./GD32F10x_standard_peripheral/Include -Ispl/CMSIS -Ispl/inc -c -g -mcpu=$(TARGET_CPU) -mthumb
CFLAGS  += $(TARGET_CMSIS_INCLUDES) -I./GD32F10x_standard_peripheral/Include -I. -I./spl/CMSIS -I./spl/CMSIS/inc -I./spl/inc -DUSE_FULL_ASSERT -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -c -fno-common -O$(OPT) -g -mcpu=$(TARGET_CPU) -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections
CFLAGS += -fno-builtin
CFLAGS += -std=c99 --specs=nano.specs --specs=nosys.specs -Wall -I../../common/include -I../../common/protocol

LFLAGS  += -Tstm32_flash.ld -lgcc -lm -nostartfiles -lnosys -mcpu=$(TARGET_CPU) -mthumb -mfloat-abi=soft -Wl,--gc-sections
LFLAGS  += $(TARGET_LFLAGS)
LFLAGS += --specs=nano.specs --specs=nosys.specs
LFLAGS += -Xlinker -Map=main.map
CPFLAGS = -Obinary
ODFLAGS = -S

include ../../common/Makefile.common

COMMONDIR = ../../common/src
PROTOCOLDIR = ../../common/protocol
OBJDIR = _build
CUSTOM_SOURCES=$(shell find spl ugui_driver *.c -type f -iname '*.c')
ifneq ($(TARGET_SYSTEM_SOURCE),)
CUSTOM_SOURCES := $(filter-out spl/CMSIS/%.c spl/src/%.c,$(CUSTOM_SOURCES)) $(TARGET_SYSTEM_SOURCE)
endif
COMMON_SOURCES = fault.c buttons.c utils.c ugui.c fonts.c state.c screen.c mainscreen.c configscreen.c eeprom.c
PROTOCOL_SOURCES = protocol.c tsdz2_protocol.c bafang_protocol.c range_estimator.c instrumentation.c
SOURCES = $(CUSTOM_SOURCES) $(foreach x, $(COMMON_SOURCES), $(COMMONDIR)/$(x)) $(foreach x, $(PROTOCOL_SOURCES), $(PROTOCOLDIR)/$(x))
OBJECTS = $(foreach x, $(basename $(SOURCES)), $(OBJDIR)/$(x).o)

all: main.bin size

test:
	echo objs $(OBJECTS)

clean:
	rm -f main.lst main.elf main.bin
	rm -rf $(OBJDIR) .deps

size:
	@echo "Size:"
	$(SIZE) main.elf

main.bin: main.elf
	@echo "...copying"
	$(CP) $(CPFLAGS) main.elf main.bin
	$(OD) $(ODFLAGS) main.elf > main.lst

main.elf: $(OBJECTS) $(TARGET_STARTUP_OBJECT)
	@echo "..linking"
	$(LD) $^ $(LFLAGS) -o $@

startup_stm32f10x_md.o:
	$(AS) $(ASFLAGS) startup_stm32f10x_md.s -o startup_stm32f10x_md.o

startup_gd32f30x_hd.o:
	$(CC) $(CFLAGS) -x assembler-with-cpp -c gd32f30x_cmsis/startup_gd32f30x_hd.S -o startup_gd32f30x_hd.o

DEPDIR := .deps
DEPFLAGS = -MT $@ -MD -MP -MF $(DEPDIR)/$*.d
COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

$(OBJDIR)/%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	@mkdir -p $(dir $@) $(DEPDIR)/$(dir $<)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(DEPDIR): ; @mkdir -p $@
DEPFILES := $(SOURCES:%.c=$(DEPDIR)/%.d)
$(DEPFILES):
include $(wildcard $(DEPFILES))