#### Apply patch to ESP8266 SDK

As `iram` is quite small to fit all the codes but linker tries to put it there.
To force JerryScript codes to be placed at the `irom` section,
need to change the order and tell the linker as below;

```
diff --git a/ld/eagle.app.v6.common.ld b/ld/eagle.app.v6.common.ld
index caf8e32..dadaceb 100644
--- a/ld/eagle.app.v6.common.ld
+++ b/ld/eagle.app.v6.common.ld
@@ -151,6 +151,21 @@ SECTIONS
   } >dram0_0_seg :dram0_0_bss_phdr
 /* __stack = 0x3ffc8000; */
 
+  .irom0.text : ALIGN(4)
+  {
+    _irom0_text_start = ABSOLUTE(.);
+    *(.irom0.literal .irom.literal .irom.text.literal .irom0.text .irom.text)
+    *(.literal.* .text.*)
+    _irom0_text_end = ABSOLUTE(.);
+
+    _jerry_text_start = ABSOLUTE(.);
+    *\libjerryentry.a:*(.text*)
+    *\libjerrycore.a:*(.text*)
+    *\libjerrylibm.a:*(.text*)
+    _jerry_text_end = ABSOLUTE(.);
+
+  } >irom0_0_seg :irom0_0_phdr
+
   .text : ALIGN(4)
   {
     _stext = .;
@@ -199,13 +214,6 @@ SECTIONS
     _lit4_end = ABSOLUTE(.);
   } >iram1_0_seg :iram1_0_phdr
 
-  .irom0.text : ALIGN(4)
-  {
-    _irom0_text_start = ABSOLUTE(.);
-    *(.irom0.literal .irom.literal .irom.text.literal .irom0.text .irom.text)
-    *(.literal.* .text.*)
-    _irom0_text_end = ABSOLUTE(.);
-  } >irom0_0_seg :irom0_0_phdr
 }
 
 /* get ROM code address */
diff --git a/ld/eagle.app.v6.ld b/ld/eagle.app.v6.ld
index 3e7ec1b..4a9ab5b 100644
--- a/ld/eagle.app.v6.ld
+++ b/ld/eagle.app.v6.ld
@@ -26,7 +26,7 @@ MEMORY
   dport0_0_seg :                       org = 0x3FF00000, len = 0x10
   dram0_0_seg :                        org = 0x3FFE8000, len = 0x14000
   iram1_0_seg :                        org = 0x40100000, len = 0x8000
-  irom0_0_seg :                        org = 0x40240000, len = 0x3C000
+  irom0_0_seg :                        org = 0x40240000, len = 0xB0000
 }
 
 INCLUDE "../ld/eagle.app.v6.common.ld"
```

Second file is to modify `irom` size so that it can hold all the codes and data.
This can be done by giving another `SPI_SIZE_MAP`.
For now, I'll use this manual modification.


#### Need to get setjmp / longjmp

Extract and copy from the SDK.

```
cd ~/harmony/jerryscript/targets/esp8266/libs
ar -xv $SDK_PATH/lib/libcirom.a lib_a-setjmp.o
```
