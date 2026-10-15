set pagination off
set confirm off
set height 0
file D:/folder/code/STM32L4_TEST/build/Debug/TEST.elf
target extended-remote localhost:61234
load D:/folder/code/STM32L4_TEST/build/Debug/TEST.elf
monitor reset halt
continue
shell ping -n 6 127.0.0.1 > nul
interrupt
echo \n=== STATE AFTER RUN ===\n
p/x $pc
bt 12
p hUsbDeviceFS.NumClasses
p hUsbDeviceFS.pClass[0]
p hUsbDeviceFS.pClass[1]
p hUsbDeviceFS.pClass[2]
p hUsbDeviceFS.pUserData[0]
p hUsbDeviceFS.pUserData[1]
p hUsbDeviceFS.tclasslist[1].Active
p hUsbDeviceFS.tclasslist[1].ClassType
p hUsbDeviceFS.tclasslist[1].NumIf
p hUsbDeviceFS.tclasslist[1].NumEps
p usbd_static_used
p (int)CurrFSConfDescSz
echo \n=== CONFIG DESC ===\n
x/200xb USBD_CMPSIT_FSCfgDesc
echo \n=== DEVICE DESC ===\n
x/18xb USBD_FS_DeviceDesc
detach
quit
