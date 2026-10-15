#
# usb_state.gdb - dump the composite device state to explain a missing class
#
# Usage from the GDB console (device already enumerated / halted anywhere):
#     source D:/folder/code/STM32L4_TEST/Tool/usb_state.gdb
#
# Everything is printed with plain commands; no nested quotes for the IDE to
# mangle. Paste the whole output back.
#

echo \n========== USB HANDLE ==========\n
p hUsbDeviceFS.NumClasses
p hUsbDeviceFS.classId
p usbd_static_used

echo \n========== CLASS SLOT 0 (expected: Audio) ==========\n
p hUsbDeviceFS.tclasslist[0].ClassType
p hUsbDeviceFS.tclasslist[0].Active
p hUsbDeviceFS.tclasslist[0].NumIf
p hUsbDeviceFS.tclasslist[0].NumEps
p hUsbDeviceFS.tclasslist[0].Ifs[0]
p hUsbDeviceFS.tclasslist[0].Ifs[1]
p hUsbDeviceFS.tclasslist[0].Eps[0].add
p hUsbDeviceFS.pClass[0]
p hUsbDeviceFS.pClassDataCmsit[0]

echo \n========== CLASS SLOT 1 (expected: CDC) ==========\n
p hUsbDeviceFS.tclasslist[1].ClassType
p hUsbDeviceFS.tclasslist[1].Active
p hUsbDeviceFS.tclasslist[1].NumIf
p hUsbDeviceFS.tclasslist[1].NumEps
p hUsbDeviceFS.tclasslist[1].Ifs[0]
p hUsbDeviceFS.tclasslist[1].Ifs[1]
p hUsbDeviceFS.tclasslist[1].Eps[0].add
p hUsbDeviceFS.tclasslist[1].Eps[1].add
p hUsbDeviceFS.tclasslist[1].Eps[2].add
p hUsbDeviceFS.pClass[1]
p hUsbDeviceFS.pClassDataCmsit[1]

echo \n========== CLASS SLOT 2 (expected: inactive) ==========\n
p hUsbDeviceFS.tclasslist[2].ClassType
p hUsbDeviceFS.tclasslist[2].Active
p hUsbDeviceFS.pClass[2]

echo \n========== USER CALLBACK TABLES ==========\n
p hUsbDeviceFS.pUserData[0]
p hUsbDeviceFS.pUserData[1]
p hUsbDeviceFS.pUserData[2]

echo \n========== DESCRIPTOR SIZE ==========\n
p (int)CurrFSConfDescSz

echo \n========== DEVICE DESCRIPTOR (18 bytes) ==========\n
x/18xb USBD_FS_DeviceDesc

echo \n========== CLASS TYPE ENUM REFERENCE ==========\n
echo NONE=0 HID=1 CDC=2 MSC=3 DFU=4 CHID=5 AUDIO=6 ECM=7 RNDIS=8 MTP=9 VIDEO=10 PRINTER=11 CCID=12\n

echo \n========== END ==========\n
