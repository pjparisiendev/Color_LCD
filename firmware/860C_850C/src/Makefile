#  Copyright (C) 2015 Joerg Hoener
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.

# we expect DISPLAY_VERSION to be passed as argument to Make, like: make DISPLAY_VERSION="860C_BOOTLOADER"
# check to see if version is defined, if not, assume default
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

# Confirmed Green Pedel E2.3 hardware. This target intentionally retains the
# APT 16 KiB application offset proven by an official-tool virtual capture,
# while the historical F103 compatibility layer
# is audited against the official GD32F303 CMSIS/peripheral library.
ifeq ($(DISPLAY_VERSION), APT_850C_GD32F303RET6)
  CFLAGS += -DDISPLAY_850C -DTARGET_APT_850C_GD32F303RET6 -DGD32F30X_HD -DCONTROLLER_TX_LOCKED
  BOOTLOADER = yes
  TARGET_LFLAGS = -Xlinker --defsym=RESERVE_SETTINGS_PAGES=1
  TARGET_CPU = cortex-m4
  TARGET_CMSIS_INCLUDES = -I./gd32f30x_cmsis/Include -I./gd32f30x_cmsis/Core
  TARGET_SYSTEM_SOURCE = gd32f30x_cmsis/system_gd32f30x.c gd32f30x_cmsis/gd32f30x_adc.c gd32f30x_cmsis/gd32f30x_dma.c gd32f30x_cmsis/gd32f30x_gpio.c gd32f30x_cmsis/gd32f30x_misc.c gd32f30x_cmsis/gd32f30x_rcu.c gd32f30x_cmsis/gd32f30x_timer.c gd32f30x_cmsis/gd32f30x_usart.c
  TARGET_STARTUP_OBJECT = startup_gd32f30x_hd.o
  DISPLAY_VERSION_DEFINED = yes
  $(warning DEVELOPMENT ONLY: GD32F303 peripheral/pin audit incomplete; NOT SAFE TO FLASH)
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
  $(info WARNING | target: APT_850C_GD32F303RET6 at validated APT offset 0x4000)
  endif
endif

ifdef BOOTLOADER
CFLAGS += -DUSE_WITH_BOOTLOADER
LFLAGS = -Xlinker --defsym=USE_WITH_BOOTLOADER=1
endif

TCPREFIX  = arm-none-eabi-
CC      = $(TCPREFIX)gcc
AS      = $(TCPREFIX)as 
LD      = $(TCPREFIX)gcc -v # use GCC and not LD so the math functions work like atan2()
CP      = $(TCPREFIX)objcopy
OD      = $(TCPREFIX)objdump
GDB     = $(TCPREFIX)gdb
SIZE     = $(TCPREFIX)size

# Optimization level, can be [0, 1, 2, 3, s]. 
# 0 = Turn off optimization. Reduce compilation time and make debugging
#     produce the expected results.
# 1 = The compiler tries to reduce code size and execution time, without
#     performing any optimizations that take a great deal of compilation time.
# 2 = GCC performs nearly all supported optimizations that do not involve a 
#     space-speed tradeoff. As compared to -O1, this option increases
#     both compilation time and the performance of the generated code.
# 3 = Optimize yet more. Turns on -finline-functions and more.
# s = -Os enables all -O2 optimizations that do not typically increase code
#     size.
# (See gcc manual for further information)
OPT = 0

# -mfix-cortex-m3-ldrd should be enabled by default for Cortex M3.
# CFLAGS -H show header files

AFLAGS  = -I./GD32F10x_standard_peripheral/Include -Ispl/CMSIS -Ispl/inc -c -g -mcpu=$(TARGET_CPU) -mthumb
CFLAGS  += $(TARGET_CMSIS_INCLUDES) -I./GD32F10x_standard_peripheral/Include -I. -I./spl/CMSIS -I./spl/CMSIS/inc -I./spl/inc -DUSE_FULL_ASSERT -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER -c -fno-common -O$(OPT) -g -mcpu=$(TARGET_CPU) -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections
# Need following option for LTO as LTO will treat retarget functions as
# unused without following option
CFLAGS += -fno-builtin
CFLAGS += -std=c99 --specs=nano.specs --specs=nosys.specs -Wall  -I../../common/include -I../../common/protocol

LFLAGS  += -Tstm32_flash.ld -lgcc -lm -nostartfiles -lnosys -mcpu=$(TARGET_CPU) -mthumb -mfloat-abi=soft -Wl,--gc-sections
LFLAGS  += $(TARGET_LFLAGS)

LFLAGS += --specs=nano.specs --specs=nosys.specs # to use newlib nano
LFLAGS += -Xlinker -Map=main.map
#LFLAGS += -u _printf_float # newlib nano printf use floats
#LFLAGS += -u _scanf_float # newlib nano scanf use floats
CPFLAGS = -Obinary 
ODFLAGS = -S

include ../../common/Makefile.common

COMMONDIR = ../../common/src
PROTOCOLDIR = ../../common/protocol
OBJDIR = _build
CUSTOM_SOURCES=$(shell find spl ugui_driver *.c -type f -iname '*.c') 
ifneq ($(TARGET_SYSTEM_SOURCE),)
# A GD32F303 target must never silently link an STM32F10x peripheral or CMSIS
# implementation. Native GD32 sources above provide every required facility.
CUSTOM_SOURCES := $(filter-out spl/CMSIS/%.c spl/src/%.c,$(CUSTOM_SOURCES)) $(TARGET_SYSTEM_SOURCE)
endif
COMMON_SOURCES = fault.c buttons.c utils.c ugui.c fonts.c state.c screen.c mainscreen.c configscreen.c eeprom.c
PROTOCOL_SOURCES = protocol.c tsdz2_protocol.c bafang_protocol.c range_estimator.c instrumentation.c
SOURCES = $(CUSTOM_SOURCES) $(foreach x, $(COMMON_SOURCES), $(COMMONDIR)/$(x)) $(foreach x, $(PROTOCOL_SOURCES), $(PROTOCOLDIR)/$(x))
OBJECTS = $(foreach x, $(basename $(SOURCES)), $(OBJDIR)/$(x).o)

# dev platform specific.
# OPENOCD_PATH := E:/nrf5/Toolchain/OpenOCD/0.10.0-12-20190422-2015/bin
# OPENOCD_BIN := openocd.exe
# NRFUTIL := $(SDK_ROOT)/external_tools/Windows/nrfutil.exe

OPENOCD_PATH := /usr/local/share/openocd/bin
OPENOCD_BIN := openocd
NRFUTIL := nrfutil

OPENOCD := '$(OPENOCD_PATH)/$(OPENOCD_BIN)' -f $(OPENOCD_PATH)/../scripts/interface/stlink.cfg -f $(OPENOCD_PATH)/../scripts/target/stm32f1x.cfg

all: main.bin size

# Ignore - useful for debugging makefiles
test:
	echo objs $(OBJECTS)

clean: 
	rm -f main.lst main.elf main.bin
	rm -rf $(OBJDIR) .deps
	rm -f ../common/src/*.o ../common/protocol/*.o

flash_serial flash_jtag run_jtag:
	$(error DISABLED: physical programming is prohibited until recovery and the E2.3 platform are verified)

# Just start an openocd session
openocd:
	$(OPENOCD) 

size: main.elf
	@echo "Size:"
	$(SIZE) main.elf

main.bin: main.elf
	@echo "...copying"
	$(CP) $(CPFLAGS) main.elf main.bin
	$(OD) $(ODFLAGS) main.elf > main.lst

main.elf: $(OBJECTS) $(TARGET_STARTUP_OBJECT)
	@echo "..linking"
	$(LD)  $^ $(LFLAGS) -o $@

startup_stm32f10x_md.o:
	$(AS) $(ASFLAGS) startup_stm32f10x_md.s -o startup_stm32f10x_md.o

startup_gd32f30x_hd.o: gd32f30x_cmsis/startup_gd32f30x_hd.S
	$(CC) $(CFLAGS) -x assembler-with-cpp $< -o $@

#
# The following makefile magic generates proper dependencies for headerfiles, so that if you change a headerfile
# the correct C files that use it will be recompiled 
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
