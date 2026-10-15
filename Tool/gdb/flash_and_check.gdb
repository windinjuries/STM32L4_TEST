#
# flash_and_check.gdb - load the image, let it run, and capture the init sequence.
# Driven by Tool/gdb_verify.py.
#
# Sets a breakpoint on USBD_RegisterClassComposite(). If the composite device is
# built correctly it must be hit TWICE (Audio then CDC). Hitting once proves the
# CDC registration line never executes, which is the fact we need.
#

set pagination off
set confirm off
set height 0

echo \n===== LOAD =====\n
load D:/folder/code/STM32L4_TEST/build/Debug/TEST.elf
monitor reset halt

echo \n===== BREAK ON RegisterClassComposite =====\n
break USBD_RegisterClassComposite
info breakpoints

echo \n===== FIRST HIT (expect Audio, classtype should be 6) =====\n
continue
bt 4
p classtype

echo \n===== SECOND HIT (expect CDC, classtype should be 2) =====\n
continue
bt 4
p classtype

echo \n===== AFTER SECOND REGISTRATION =====\n
finish
p hUsbDeviceFS.pClass[0]
p hUsbDeviceFS.pClass[1]
p hUsbDeviceFS.pClass[2]
p hUsbDeviceFS.NumClasses
p (int)CurrFSConfDescSz

echo \n===== END FLASH_AND_CHECK =====\n
