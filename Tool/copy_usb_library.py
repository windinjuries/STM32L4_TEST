#!/usr/bin/env python3
"""Copy the official STM32 USB Device Library into the project tree.

Source: STM32Cube_FW_L4_V1.18.2 (local CubeMX repository)
Target: <project>/Middlewares/ST/STM32_USB_Device_Library

Verbatim copy: byte-for-byte identical, no source modification.
Non-source assets (the _htmresc HTML resources directory) are skipped.
Already-identical files are skipped so re-running is idempotent.
"""
import hashlib
import os
import shutil
import sys

SRC = r"C:\Users\Administrator\STM32Cube\Repository\STM32Cube_FW_L4_V1.18.2\Middlewares\ST\STM32_USB_Device_Library"
DST = r"D:\folder\code\STM32L4_TEST\Middlewares\ST\STM32_USB_Device_Library"
SKIP_DIRS = {"_htmresc"}


def md5(path):
    h = hashlib.md5()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    if not os.path.isdir(SRC):
        print("ERROR: source not found: %s" % SRC)
        return 1

    copied = skipped = 0
    failed = []

    for root, dirs, files in os.walk(SRC):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for name in files:
            src_file = os.path.join(root, name)
            rel = os.path.relpath(src_file, SRC)
            dst_file = os.path.join(DST, rel)
            dst_dir = os.path.dirname(dst_file)
            try:
                if not os.path.isdir(dst_dir):
                    os.makedirs(dst_dir, exist_ok=True)
                if os.path.isfile(dst_file) and md5(src_file) == md5(dst_file):
                    skipped += 1
                    continue
                shutil.copy2(src_file, dst_file)
                copied += 1
            except Exception as exc:  # noqa: BLE001
                failed.append("%s -> %s" % (rel, exc))

    print("copied  : %d" % copied)
    print("skipped : %d (already identical)" % skipped)
    print("failed  : %d" % len(failed))
    for item in failed[:20]:
        print("   FAIL %s" % item)

    classes_dir = os.path.join(DST, "Class")
    if os.path.isdir(classes_dir):
        print("Class modules: %s" % ", ".join(sorted(os.listdir(classes_dir))))
    total = sum(len(f) for _, _, f in os.walk(DST))
    print("total files under target: %d" % total)
    return 0 if not failed else 1


if __name__ == "__main__":
    sys.exit(main())
