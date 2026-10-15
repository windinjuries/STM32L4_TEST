#!/usr/bin/env python3
"""Drive the ST-LINK GDB server and GDB to verify the USB composite device.

Two stages:
  flash  - load the current ELF, let it run, capture USBD_RegisterClassComposite
           hits (must be 2: Audio then CDC)
  run    - load, run a few seconds, halt and read the full device state plus the
           configuration descriptor bytes

Everything is written to Tool/gdb/<stage>.log and echoed in summary form.

Usage:
    python Tool/gdb_verify.py flash
    python Tool/gdb_verify.py run
"""
import os
import subprocess
import sys
import time

PROJECT = r"D:\folder\code\STM32L4_TEST"
BUNDLE = r"C:\Users\Administrator\AppData\Local\stm32cube\bundles"
GDBSERVER = os.path.join(BUNDLE, r"stlink-gdbserver\7.14.0+st.2\bin\ST-LINK_gdbserver.exe")
CUBEPROG = os.path.join(BUNDLE, r"programmer\2.22.0+st.1\bin")
GDB = os.path.join(BUNDLE, r"gnu-tools-for-stm32\14.3.1+st.2\bin\arm-none-eabi-gdb.exe")
ELF = os.path.join(PROJECT, r"build\Debug\TEST.elf")
GDBDIR = os.path.join(PROJECT, "Tool", "gdb")
PORT = 61234


def start_server():
    """Start the GDB server detached; it keeps listening between GDB runs."""
    log = open(os.path.join(GDBDIR, "server.log"), "wb")
    proc = subprocess.Popen(
        [GDBSERVER, "-p", str(PORT), "-l", "1", "-e", "--frequency", "1000",
         "-cp", CUBEPROG],
        stdout=log, stderr=subprocess.STDOUT,
        creationflags=getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0))
    time.sleep(6)
    return proc


def run_gdb(cmds, log_name):
    """Feed GDB a command file and capture its output."""
    cmdfile = os.path.join(GDBDIR, "_auto.gdb")
    logfile = os.path.join(GDBDIR, log_name)
    with open(cmdfile, "w") as fh:
        fh.write("set pagination off\nset confirm off\nset height 0\n")
        fh.write("file %s\n" % ELF.replace("\\", "/"))
        fh.write("target extended-remote localhost:%d\n" % PORT)
        fh.write("\n".join(cmds) + "\n")
        fh.write("detach\nquit\n")
    with open(logfile, "wb") as out:
        proc = subprocess.run([GDB, "--batch", "-x", cmdfile],
                              stdout=out, stderr=subprocess.STDOUT,
                              timeout=180)
    return proc.returncode, logfile


def main():
    stage = sys.argv[1] if len(sys.argv) > 1 else "run"
    if not os.path.isfile(GDB):
        print("ERROR: gdb not found: %s" % GDB)
        return 2
    os.makedirs(GDBDIR, exist_ok=True)

    start_server()
    print("gdb server started on port %d" % PORT)

    if stage == "flash":
        cmds = [
            "load " + ELF.replace("\\", "/"),
            "monitor reset halt",
            "break USBD_RegisterClassComposite",
            "echo \\n=== HIT 1 ===\\n",
            "continue",
            "bt 4",
            "p classtype",
            "echo \\n=== HIT 2 ===\\n",
            "continue",
            "bt 4",
            "p classtype",
            "echo \\n=== AFTER 2ND REG ===\\n",
            "p hUsbDeviceFS.pClass[0]",
            "p hUsbDeviceFS.pClass[1]",
            "p hUsbDeviceFS.pClass[2]",
            "p hUsbDeviceFS.NumClasses",
            "p (int)CurrFSConfDescSz",
        ]
    else:
        cmds = [
            "load " + ELF.replace("\\", "/"),
            "monitor reset halt",
            "continue",
            "shell ping -n 6 127.0.0.1 > nul",
            "interrupt",
            "echo \\n=== STATE AFTER RUN ===\\n",
            "p/x $pc",
            "bt 12",
            "p hUsbDeviceFS.NumClasses",
            "p hUsbDeviceFS.pClass[0]",
            "p hUsbDeviceFS.pClass[1]",
            "p hUsbDeviceFS.pClass[2]",
            "p hUsbDeviceFS.pUserData[0]",
            "p hUsbDeviceFS.pUserData[1]",
            "p hUsbDeviceFS.tclasslist[1].Active",
            "p hUsbDeviceFS.tclasslist[1].ClassType",
            "p hUsbDeviceFS.tclasslist[1].NumIf",
            "p hUsbDeviceFS.tclasslist[1].NumEps",
            "p usbd_static_used",
            "p (int)CurrFSConfDescSz",
            "echo \\n=== CONFIG DESC ===\\n",
            "x/200xb USBD_CMPSIT_FSCfgDesc",
            "echo \\n=== DEVICE DESC ===\\n",
            "x/18xb USBD_FS_DeviceDesc",
        ]

    rc, logfile = run_gdb(cmds, "%s.log" % stage)
    print("gdb exit=%s  log=%s" % (rc, logfile))
    print("=" * 70)
    with open(logfile, "r", errors="replace") as fh:
        print(fh.read())
    return 0


if __name__ == "__main__":
    sys.exit(main())
