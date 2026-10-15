set pagination off
set confirm off
set height 0
file D:/folder/code/STM32L4_TEST/build/Debug/TEST.elf
target extended-remote localhost:50000
monitor reset halt
continue
shell ping -n 6 127.0.0.1 > nul
interrupt
#
# usb_final.gdb - let the firmware run freely, then inspect where it ended up.
# No breakpoints, so nothing can hang.
#

set pagination off
set confirm off
set height 0

echo \n=== let it run ===\n
continue
echo \n(back after the host-side sleep)\n

echo \n=== WHERE IS THE CPU ===\n
p/x $pc
bt 12

echo \n=== CLASS TABLE ===\n
p hUsbDeviceFS.classId
p hUsbDeviceFS.NumClasses
p hUsbDeviceFS.pClass[0]
p hUsbDeviceFS.pClass[1]
p hUsbDeviceFS.pClass[2]
p hUsbDeviceFS.pUserData[0]
p hUsbDeviceFS.pUserData[1]
p hUsbDeviceFS.tclasslist[0].Active
p hUsbDeviceFS.tclasslist[0].ClassType
p hUsbDeviceFS.tclasslist[0].NumIf
p hUsbDeviceFS.tclasslist[0].NumEps
p hUsbDeviceFS.tclasslist[1].Active
p hUsbDeviceFS.tclasslist[1].ClassType
p hUsbDeviceFS.tclasslist[1].NumIf
p hUsbDeviceFS.tclasslist[1].NumEps
p hUsbDeviceFS.tclasslist[1].Ifs[0]
p hUsbDeviceFS.tclasslist[1].Eps[0].add
p hUsbDeviceFS.tclasslist[1].Eps[1].add
p hUsbDeviceFS.tclasslist[1].Eps[2].add
p usbd_static_used
p (int)CurrFSConfDescSz

echo \n=== FAULT GLOBALS ===\n
p/x hf_stacked_pc
p/x hf_bfar
p/x hf_cfsr

echo \n=== CONFIG DESCRIPTOR ===\n
x/200xb USBD_CMPSIT_FSCfgDesc

echo \n=== DEVICE DESCRIPTOR ===\n
x/18xb USBD_FS_DeviceDesc

detach
quit
