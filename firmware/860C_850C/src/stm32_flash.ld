/*
*****************************************************************************
**
**  File        : stm32_flash.ld
**
**  Abstract    : Development-only memory layout for the physically identified
**                GD32F303RET6 (512 KiB flash, 64 KiB SRAM).
**
**  Target      : APT 850C GD32F303RET6 platform audit
**
*****************************************************************************
*/

ENTRY(Reset_Handler)

_estack = 0x20010000;
_Min_Heap_Size = 0;
_Min_Stack_Size = 0x100;

MEMORY
{
  FLASH (rx)      : ORIGIN = DEFINED(USE_WITH_BOOTLOADER) ? (0x08000000 + 20K) : 0x08000000,
                    LENGTH = (DEFINED(USE_WITH_BOOTLOADER) ? (512K - 20K) : 512K) -
                             (DEFINED(RESERVE_SETTINGS_PAGES) ? 4K : 0K)
  RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 64K
  MEMORY_B1 (rx)  : ORIGIN = 0x60000000, LENGTH = 0K
}

ASSERT(ORIGIN(RAM) == 0x20000000, "unexpected GD32F303RET6 SRAM origin")
ASSERT(LENGTH(RAM) == 64K, "unexpected GD32F303RET6 SRAM length")
ASSERT(_estack == ORIGIN(RAM) + LENGTH(RAM), "stack top is outside SRAM")
ASSERT((DEFINED(USE_WITH_BOOTLOADER) && ORIGIN(FLASH) == 0x08005000) ||
       (!DEFINED(USE_WITH_BOOTLOADER) && ORIGIN(FLASH) == 0x08000000),
       "unexpected application origin")
_settings_flash_start = 0x0807F000;
_settings_flash_end = 0x08080000;
ASSERT((DEFINED(RESERVE_SETTINGS_PAGES) &&
        ORIGIN(FLASH) + LENGTH(FLASH) == _settings_flash_start) ||
       (!DEFINED(RESERVE_SETTINGS_PAGES) &&
        ORIGIN(FLASH) + LENGTH(FLASH) == _settings_flash_end),
       "unexpected application/settings flash boundary")

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .ARM.attributes : { *(.ARM.attributes) } >FLASH

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  _sidata = .;

  .data : AT ( _sidata )
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM

  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  PROVIDE ( end = _ebss );
  PROVIDE ( _end = _ebss );

  ._user_heap_stack :
  {
    . = ALIGN(4);
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(4);
  } >RAM

  .memory_b1_text :
  {
    *(.mb1text)
    *(.mb1text*)
    *(.mb1rodata)
    *(.mb1rodata*)
  } >MEMORY_B1
}
