#
# inspect.gdb - read the USB device state from a running target.
# Driven by Tool/gdb_verify.py. Assumes the core is already halted.
#

set pagination off
set confirm off
set height 0

echo \n===== WHERE IS THE CPU =====\n
p/x $pc
p/x $lr
p/x $sp
p/x $psp
bt 10

echo \n===== FAULT HANDLER GLOBALS =====\n
p/x hf_stacked_pc
p/x hf_bfar
p/x hf_cfsr
p/x hf_exc_return

echo \n===== USB CLASS TABLE =====\n
p hUsbDeviceFS.NumClasses
p hUsbDeviceFS.classId
p hUsbDeviceFS.pClass[0]
p hUsbDeviceFS.pClass[1]
p hUsbDeviceFS.pClass[2]
p hUsbDeviceFS.pUserData[0]
p hUsbDeviceFS.pUserData[1]
p hUsbDeviceFS.tclasslist[0].Active
p hUsbDeviceFS.tclasslist[0].ClassType
p hUsbDeviceFS.tclasslist[0].NumIf
p hUsbDeviceFS.tclasslist[1].Active
p hUsbDeviceFS.tclasslist[1].ClassType
p hUsbDeviceFS.tclasslist[1].NumIf
p hUsbDeviceFS.tclasslist[1].NumEps
p hUsbDeviceFS.tclasslist[1].Ifs[0]
p hUsbDeviceFS.tclasslist[1].Eps[0].add
p hUsbDeviceFS.tclasslist[1].Eps[1].add
p hUsbDeviceFS.tclasslist[1].Eps[2].add

echo \n===== MEMORY POOL / DESCRIPTOR =====\n
p usbd_static_used
p (int)CurrFSConfDescSz

echo \n===== CONFIGURATION DESCRIPTOR BYTES =====\n
x/200xb USBD_CMPSIT_FSCfgDesc

echo \n===== DEVICE DESCRIPTOR =====\n
x/18xb USBD_FS_DeviceDesc

echo \n===== END INSPECT =====\n
