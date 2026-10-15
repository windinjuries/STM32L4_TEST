#
# usb_deep.gdb - inspect what USBD_CMPSIT_AddClass does for the CDC class.
# Driven by Tool/pyocd_debug.py. No dangling `continue` statements: every stop
# is either a `next`/`finish` that cannot hang, or a single `continue` whose
# breakpoint set is known to be reachable.
#

set pagination off
set confirm off
set height 0

echo \n=== break on AddClass + descriptor size ===\n
break USBD_CMPSIT_AddClass
break USBD_CMPSIT_AddToConfDesc
break USBD_CMPSIT_CDCDesc
break Error_Handler

echo \n=== let init run: first AddClass (Audio) ===\n
continue
p class
p pdev->classId
p pdev->NumClasses
p pdev->tclasslist[pdev->classId].Active

echo \n=== step over Audio AddClass ===\n
finish
p (int)CurrFSConfDescSz
p pdev->classId
p pdev->tclasslist[0].Active
p pdev->tclasslist[0].NumIf
p pdev->tclasslist[0].NumEps

echo \n=== next stop should be AddClass for CDC ===\n
continue
p class
p pdev->classId
p pdev->NumClasses
p pdev->tclasslist[pdev->classId].Active
p/x $pc

echo \n=== step over CDC AddClass (this is where the descriptor should grow) ===\n
finish
p (int)CurrFSConfDescSz
p pdev->classId
p pdev->tclasslist[1].Active
p pdev->tclasslist[1].ClassType
p pdev->tclasslist[1].NumIf
p pdev->tclasslist[1].NumEps
p pdev->tclasslist[1].Ifs[0]
p pdev->tclasslist[1].Eps[0].add
p pdev->tclasslist[1].Eps[1].add
p pdev->tclasslist[1].Eps[2].add

echo \n=== did we ever reach CDCDesc? ===\n
info breakpoints

echo \n=== final descriptor dump ===\n
x/200xb USBD_CMPSIT_FSCfgDesc
