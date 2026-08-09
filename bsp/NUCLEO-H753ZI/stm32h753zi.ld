/* STM32H753ZI memory used by this image:
   - DTCM: CPU-local data, kernel heap and stacks
   - D2 SRAM3: non-cacheable Ethernet DMA descriptors and frame buffers */

ENTRY(Reset_Handler)

_estack = 0x20020000;    /* ORIGIN(DTCM) + LENGTH(DTCM) */

_Min_Heap_Size  = 0x4000;    /* 16K kernel heap */
_Min_Stack_Size = 0x1000;    /* 4K MSP stack (interrupts + pre-scheduler) */

MEMORY
{
FLASH     (rx)  : ORIGIN = 0x08000000, LENGTH = 2048K
DTCM      (xrw) : ORIGIN = 0x20000000, LENGTH = 128K
SRAM3_ETH (rw)  : ORIGIN = 0x30040000, LENGTH = 32K
}

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
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;

    . = ALIGN(4);
        __shell_cmd_start = .;
        KEEP(*(SORT(.shell_cmd*)))
        __shell_cmd_end = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array     :
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

  /* used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)

    . = ALIGN(4);
    _edata = .;
  } >DTCM AT> FLASH

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
  } >DTCM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    PROVIDE ( __heap_start = .);
    . = . + _Min_Heap_Size;
    PROVIDE (__heap_end = .);
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >DTCM

  /*
   * Objects tagged with __attribute__((section(".eth_dma"))) are placed in
   * D2 SRAM3, which is reachable by the Ethernet DMA.  NOLOAD reserves RAM
   * without storing zero-filled buffers in the firmware image; ETH startup
   * code must initialize descriptor/control memory explicitly.
   */
  .eth_dma (NOLOAD) :
  {
    . = ALIGN(32);
    __eth_dma_start__ = .;

    KEEP(*(.eth_dma))
    KEEP(*(.eth_dma.*))

    . = ALIGN(32);
    __eth_dma_end__ = .;
  } >SRAM3_ETH

  ASSERT((__eth_dma_end__ - __eth_dma_start__) <= LENGTH(SRAM3_ETH),
         "Ethernet DMA objects exceed D2 SRAM3")

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
