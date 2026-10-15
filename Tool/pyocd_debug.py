#!/usr/bin/env python3
"""Debug the USB composite device through pyOCD + arm-none-eabi-gdb.

pyOCD is used because ST-LINK_gdbserver could not attach in this environment
("Target no device found") while pyOCD and STM32_Programmer_CLI both can.

Stage 1 (init):  break on USBD_RegisterClassComposite, USBD_CMPSIT_AddClass and
                 Error_Handler and report how many times each is reached. The
                 key fact is whether RegisterClassComposite is reached twice
                 (Audio then CDC) or only once.
Stage 2 (state): run to the scheduler and dump the class table plus the
                 configuration descriptor the host receives.

Usage: python Tool/pyocd_debug.py [init|state]
"""
import os
import subprocess
import sys
import time

PROJECT = r"D:\folder\code\STM32L4_TEST"
PYOCD = r"C:\Users\Administrator\.pyenv\pyenv-win\versions\3.10.11\Scripts\pyocd.exe"
GDB = r"C:\Users\Administrator\AppData\Local\stm32cube\bundles\gnu-tools-for-stm32\14.3.1+st.2\bin\arm-none-eabi-gdb.exe"
ELF = os.path.join(PROJECT, r"build\Debug\TEST.elf")
OUT = os.path.join(PROJECT, "Tool", "gdb")
PORT = 50000


def start_pyocd():
    log = open(os.path.join(OUT, "pyocd.log"), "wb")
    proc = subprocess.Popen(
        [PYOCD, "gdbserver", "--target", "stm32l475xg",
         "--frequency", "1000000", "--port", str(PORT), "-O", "reset_type=hw"],
        stdout=log, stderr=subprocess.STDOUT)
    time.sleep(9)
    return proc


def run_gdb(body, name):
    cmdfile = os.path.join(OUT, "_pyocd_%s.gdb" % name)
    logfile = os.path.join(OUT, "pyocd_%s.log" % name)
    with open(cmdfile, "w") as fh:
        fh.write("set pagination off\nset confirm off\nset height 0\n")
        fh.write("file %s\n" % ELF.replace("\\", "/"))
        fh.write("target extended-remote localhost:%d\n" % PORT)
        fh.write("monitor reset halt\n")
        fh.write(body)
        fh.write("\ndetach\nquit\n")
    with open(logfile, "wb") as out:
        rc = subprocess.run([GDB, "--batch", "-x", cmdfile],
                            stdout=out, stderr=subprocess.STDOUT,
                            timeout=240).returncode
    print("gdb rc=%s log=%s" % (rc, logfile))
    with open(logfile, "r", errors="replace") as fh:
        print(fh.read())


INIT = """
break USBD_RegisterClassComposite
break USBD_CMPSIT_AddClass
break Error_Handler
echo \\n=== RUN ===\\n
continue
echo \\n=== HIT A ===\\n
bt 3
p classtype
continue
echo \\n=== HIT B ===\\n
bt 3
p class
continue
echo \\n=== HIT C ===\\n
bt 3
continue
echo \\n=== HIT D ===\\n
bt 3
continue
echo \\n=== HIT E ===\\n
bt 6
p/x $pc
continue
echo \\n=== HIT F ===\\n
bt 6
p/x $pc
"""

STATE = """
break StartDefaultTask
continue
delete breakpoints
continue
echo \\n=== after a short run ===\\n
shell ping -n 5 127.0.0.1 > nul
interrupt
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
echo \\n=== CONFIG DESC ===\\n
x/200xb USBD_CMPSIT_FSCfgDesc
"""


def main():
    which = sys.argv[1] if len(sys.argv) > 1 else "init"
    os.makedirs(OUT, exist_ok=True)
    if not os.path.isfile(PYOCD):
        print("ERROR: pyocd not found")
        return 2
    start_pyocd()
    print("pyocd gdbserver started on port %d" % PORT)

    if which == "deep":
        script = "usb_deep.gdb"
    elif which == "final":
        script = "usb_final.gdb"
    else:
        script = None

    if script is not None:
        # Hand written script; no generated `continue` chain that could hang.
        logfile = os.path.join(OUT, "pyocd_%s.log" % which)
        runfile = os.path.join(OUT, "_pyocd_%s_run.gdb" % which)
        with open(os.path.join(OUT, script), "r") as fh:
            body = fh.read()
        with open(runfile, "w") as fh:
            fh.write("set pagination off\nset confirm off\nset height 0\n")
            fh.write("file %s\n" % ELF.replace("\\", "/"))
            fh.write("target extended-remote localhost:%d\n" % PORT)
            fh.write("monitor reset halt\n")
            if which == "final":
                # Run freely, then interrupt via the host sleep.
                fh.write("continue\n")
                fh.write("shell ping -n 6 127.0.0.1 > nul\n")
                fh.write("interrupt\n")
            fh.write(body)
            fh.write("\ndetach\nquit\n")
        with open(logfile, "wb") as out:
            rc = subprocess.run([GDB, "--batch", "-x", runfile],
                                stdout=out, stderr=subprocess.STDOUT,
                                timeout=240).returncode
        print("gdb rc=%s log=%s" % (rc, logfile))
        with open(logfile, "r", errors="replace") as fh:
            print(fh.read())
        return 0

    run_gdb(INIT if which == "init" else STATE, which)
    return 0


if __name__ == "__main__":
    sys.exit(main())
