#!/usr/bin/env python3
"""Verify the USB composite device through the pyOCD Python API.

GDB scripting proved unreliable here (batch `continue`/`interrupt` hangs), so
this talks to the target directly: reset, run, halt, read memory.

Run with the pyenv Python that has pyocd installed:
    C:\\Users\\Administrator\\.pyenv\\pyenv-win\\versions\\3.10.11\\python.exe \\
        Tool/pyocd_api.py
"""
import os
import sys
import time

sys.path.insert(0, r"C:\Users\Administrator\.pyenv\pyenv-win\versions\3.10.11\Lib\site-packages")

from pyocd.core.helpers import ConnectHelper          # noqa: E402
from pyocd.core.target import Target                  # noqa: E402

# Addresses resolved from build/Debug/TEST.elf (arm-none-eabi-nm).
ADDR = {
    "CurrFSConfDescSz": 0x20000314,
    "USBD_CMPSIT_FSCfgDesc": 0x200001e8,
    "hUsbDeviceFS": 0x2000cb84,
    "usbd_static_used": 0x2000cb80,
    "hf_stacked_pc": 0x20009334,
    "hf_bfar": 0x2000933c,
    "hf_cfsr": 0x20009334 - 8,   # placeholder, corrected below if needed
}
ELF = r"D:\folder\code\STM32L4_TEST\build\Debug\TEST.elf"

# hUsbDeviceFS layout offsets (from usbd_def.h).
OFF_NumClasses = 0x00
OFF_classId = 0x04
OFF_pClass = 0x1E4          # placeholder, computed at run time below


def u32(target, addr):
    return target.read32(addr)


def dump_desc(target, base, size):
    data = target.read_memory_block8(base, size)
    return bytes(data)


def main():
    session = ConnectHelper.session_with_chosen_probe(
        target_override="stm32l475xg", frequency=1000000, blocking=False)
    if session is None:
        print("ERROR: no probe/session")
        return 2

    with session:
        target = session.target
        print("connected: %s" % target.part_number)

        target.reset_and_halt()
        print("reset and halted")

        target.resume()
        print("running for 6 s ...")
        time.sleep(6)
        target.halt()
        print("halted\n")

        # ---- where is the CPU ----
        for name in ("pc", "lr", "sp", "msp", "psp"):
            try:
                print("%-4s = 0x%08x" % (name.upper(), target.read_core_register(name)))
            except Exception as exc:  # noqa: BLE001
                print("%-4s = <error %s>" % (name.upper(), exc))

        print("\n=== descriptor size ===")
        print("CurrFSConfDescSz = %d" % u32(target, ADDR["CurrFSConfDescSz"]))
        print("usbd_static_used = %d" % u32(target, ADDR["usbd_static_used"]))

        print("\n=== hUsbDeviceFS head (64 bytes) ===")
        head = target.read_memory_block8(ADDR["hUsbDeviceFS"], 64)
        for i in range(0, len(head), 16):
            print("  +0x%03X: %s" % (i, " ".join("%02x" % b for b in head[i:i + 16])))

        print("\n=== config descriptor (256 bytes) ===")
        desc = dump_desc(target, ADDR["USBD_CMPSIT_FSCfgDesc"], 256)
        for i in range(0, 160, 16):
            print("  +0x%03X: %s" % (i, " ".join("%02x" % b for b in desc[i:i + 16])))

        # Parse just enough to answer the question.
        total = desc[2] | (desc[3] << 8)
        nif = desc[4]
        print("\nwTotalLength=%d  bNumInterfaces=%d" % (total, nif))
        has_cdc = any(desc[i + 1] == 0x04 and desc[i + 5] == 0x02 and desc[i + 6] == 0x02
                      for i in range(9, min(len(desc) - 8, 200)))
        print("CDC control interface present: %s" % has_cdc)

        fault_pc = u32(target, ADDR["hf_stacked_pc"])
        if fault_pc:
            print("hf_stacked_pc = 0x%08x  (non-zero => a fault was recorded)" % fault_pc)

    return 0


if __name__ == "__main__":
    sys.exit(main())
