/* eagle.flash.bin     @ 0x00000 */
/* eagle.irom0text.bin @ 0x20000 */

/* Flash Map, support 512KB/1MB/2MB/4MB SPI Flash */
/* |......|..............................|..........................|.....|....|                       */
/* ^      ^                              ^                          ^     ^                            */
/* |_flash.bin start(0x00000)             |_irom0text.bin start(0x20000)   |                            */
/*        |_flash.bin end                                           |_irom0text.bin end                */
/*                                                                        |_system param area(0x7b000) */

/* NOTICE: */
/* 1. You can change irom0 org, but MUST make sure irom0text.bin start not overlap flash.bin end.   */
/* 2. You can change irom0 len, but MUST make sure irom0text.bin end not overlap system param area. */
/* 3. Space between flash.bin end and irom0text.bin start can be used as user param area.           */
/* 4. Space between irom0text.bin end and system param area can be used as user param area.         */
/* 5. Make sure irom0text.bin end < 0x100000                                                        */
/* 6. system param area:                                                                            */
/*    1>. 512KB--->0x07b000                                                                         */
/*    2>. 1MB----->0x0fb000                                                                         */
/*    3>. 2MB----->0x1fb000                                                                         */
/*    4>. 4MB----->0x3fb000                                                                         */
/* 7. Don't change any other seg.                                                                   */

MEMORY
{
  dport0_0_seg :                      	org = 0x3FF00000, len = 0x10
  dram0_0_seg :                       	org = 0x3FFE8000, len = 0x18000
  iram1_0_seg :                       	org = 0x40100000, len = 0x8000
  irom0_0_seg :                       	org = 0x40220000, len = 0x7C000
}

INCLUDE ../ld/eagle.app.v6.common.ld
