#
# usb_trace2.gdb - find which registration call fails.
# Driven by Tool/pyocd_debug.py (mode "trace").
#

set pagination off
set confirm off
set height 0

echo \n=== breakpoints ===\n
break USBD_RegisterClass
break USBD_RegisterClassComposite
break Error_Handler
info breakpoints

echo \n=== STOP 1 (expect USBD_RegisterClass with USBD_CMPSIT) ===\n
continue
bt 3
p/x $pc
p pclass

echo \n=== STOP 2 ===\n
continue
bt 4
p/x $pc

echo \n=== STOP 3 ===\n
continue
bt 4
p/x $pc

echo \n=== current class-table state ===\n
p hUsbDeviceFS.classId
p hUsbDeviceFS.NumClasses
p hUsbDeviceFS.pClass[0]
p hUsbDeviceFS.pClass[1]
p hUsbDeviceFS.tclasslist[0].Active
p hUsbDeviceFS.tclasslist[0].ClassType
p hUsbDeviceFS.tclasslist[0].NumIf
p hUsbDeviceFS.tclasslist[0].NumEps
p hUsbDeviceFS.tclasslist[1].Active
p hUsbDeviceFS.tclasslist[1].ClassType
p (int)CurrFSConfDescSz

echo \n=== breakpoint hit counts ===\n
info breakpoints
