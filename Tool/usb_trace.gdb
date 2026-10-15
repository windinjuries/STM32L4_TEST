#
# usb_trace.gdb - capture the USB init sequence and the resulting device state
#
# WHY: the device reports only the Audio function (CurrFSConfDescSz = 109,
# bNumInterfaces = 2) although usb_device.c registers Audio *and* CDC. Static
# reading of the sources says CDC must be added, so the missing piece is a
# runtime fact: how many times USBD_RegisterClassComposite() is actually
# reached, and whether the default task survives MX_USB_DEVICE_Init().
#
# The default task stack is only 128 BYTES (main.c: osThreadStaticDef(...,128,...)),
# so a stack overflow during USB init would corrupt aUsbDeviceFS and produce
# exactly the half-initialised state observed. The a5 pattern check at the end
# tests that hypothesis directly.
#
# USAGE - in your existing IDE debug session, from the GDB console:
#     source D:/folder/code/STM32L4_TEST/Tool/usb_trace.gdb
#
# Paste the complete output back. Do NOT abort when the program hits a
# breakpoint: use `continue` (this is scripted) and let it run to the end.
#

set pagination off
set confirm off
set print pretty off

echo \n########## 0. load the exact ELF under test ##########\n
load D:/folder/code/STM32L4_TEST/build/Debug/TEST.elf
monitor reset halt

echo \n########## 1. breakpoints on the init sequence ##########\n
delete breakpoints
break MX_USB_DEVICE_Init
break USBD_RegisterClassComposite
break USBD_CMPSIT_AddClass
break Error_Handler
info breakpoints

echo \n########## 2. run to MX_USB_DEVICE_Init ##########\n
continue
echo --- hit #1 (expect MX_USB_DEVICE_Init) ---\n
bt 4

echo \n########## 3. step through registrations ##########\n
continue
echo --- hit #2: first RegisterClassComposite (expect Audio) ---\n
p classtype
bt 3
finish
echo after audio registration:\n
p hUsbDeviceFS.pClass[0]
p hUsbDeviceFS.pClass[1]
p hUsbDeviceFS.pClass[2]
p hUsbDeviceFS.NumClasses
p hUsbDeviceFS.classId

continue
echo --- hit #3: second RegisterClassComposite (expect CDC) ---\n
p classtype
bt 3

echo \n########## 4. if we get here, CDC registration was reached ##########\n
finish
p hUsbDeviceFS.pClass[0]
p hUsbDeviceFS.pClass[1]
p hUsbDeviceFS.pClass[2]
p hUsbDeviceFS.NumClasses
p hUsbDeviceFS.classId
p (int)CurrFSConfDescSz

echo \n########## 5. run to the end of configuration ##########\n
continue
echo --- where did we stop? ---\n
p/x $pc
bt 8

echo \n########## 6. final device state ##########\n
p hUsbDeviceFS.NumClasses
p hUsbDeviceFS.classId
p hUsbDeviceFS.pClass[0]
p hUsbDeviceFS.pClass[1]
p hUsbDeviceFS.pClass[2]
p hUsbDeviceFS.pUserData[0]
p hUsbDeviceFS.pUserData[1]
p hUsbDeviceFS.tclasslist[0].Active
p hUsbDeviceFS.tclasslist[1].Active
p hUsbDeviceFS.tclasslist[1].ClassType
p hUsbDeviceFS.tclasslist[1].NumIf
p hUsbDeviceFS.tclasslist[1].NumEps
p usbd_static_used
p (int)CurrFSConfDescSz

echo \n########## 7. default task stack integrity ##########\n
echo defaultTaskControlBlock.address = stack base; size 128 bytes = 32 words\n
p defaultTaskControlBlock
echo first 16 words of defaultTaskBuffer (fresh stack is filled with 0xa5a5a5a5):\n
x/16xw defaultTaskBuffer

echo \n########## 8. USB interrupt / fault state ##########\n
p/x $pc
p/x $lr
p/x $sp
p/x $msp
p/x $psp
p hf_stacked_pc
p hf_bfar
p hf_cfsr

echo \n########## END ##########\n
