#
# usb_verify.gdb - flash and verify the USB composite device, unattended.
# Driven by Tool/gdb_verify.py, which starts/stops the ST-LINK GDB server.
#
# Ordinal counter technique: every phase re-arms the same three breakpoints and
# uses `ignore` to skip already-seen hits. This keeps the output small and shows
# exactly how many times USBD_RegisterClassComposite() is reached -- the key
# fact, since 1 hit means the CDC registration line never runs while 2 hits mean
# it ran and must have failed internally.
#

set pagination off
set confirm off
set height 0
set width 0

echo \n@@@@@ LOAD IMAGE @@@@@\n
load D:/folder/code/STM32L4_TEST/build/Debug/TEST.elf
monitor reset halt

echo \n@@@@@ SET BREAKPOINTS @@@@@\n
delete breakpoints
break USBD_RegisterClassComposite
break USBD_CMPSIT_AddClass
break Error_Handler
info breakpoints

# ---------------------------------------------------------------------------
echo \n@@@@@ HIT 1: expect first USBD_RegisterClassComposite (Audio) @@@@@\n
ignore 1 0
ignore 2 0
ignore 3 0
continue
bt 4
p classtype

echo \n@@@@@ HIT 2: expect USBD_CMPSIT_AddClass @@@@@\n
ignore 1 1
ignore 2 0
ignore 3 0
continue
bt 4
p class

echo \n@@@@@ HIT 3: expect second USBD_RegisterClassComposite (CDC) @@@@@\n
ignore 1 1
ignore 2 1
ignore 3 0
continue
bt 4
p classtype

echo \n@@@@@ HIT 4: expect USBD_CMPSIT_AddClass for CDC @@@@@\n
ignore 1 2
ignore 2 1
ignore 3 0
continue
bt 4
p class

echo \n@@@@@ HIT 5: next event (Error_Handler would prove a registration failure) @@@@@\n
ignore 1 2
ignore 2 2
ignore 3 0
continue
bt 6
p/x $pc

# ---------------------------------------------------------------------------
echo \n@@@@@ LET THE DEVICE RUN AND ENUMERATE @@@@@\n
delete breakpoints
continue
