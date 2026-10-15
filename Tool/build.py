#!/usr/bin/env python3
"""Configure and build the STM32L4_TEST firmware using the bundled ST tools.

pwsh cannot be started in this session's sandbox, so cmake and ninja are
invoked directly with the executables that ship with the STM32Cube bundle.

Usage:
    python Tool/build.py [--configure-only] [--clean]
"""
import os
import subprocess
import sys

PROJECT = r"D:\folder\code\STM32L4_TEST"
BUNDLE = r"C:\Users\Administrator\AppData\Local\stm32cube\bundles"
CMAKE = os.path.join(BUNDLE, r"cmake\4.3.1+st.1\bin\cmake.exe")
NINJA = os.path.join(BUNDLE, r"ninja\1.13.2+st.1\bin\ninja.exe")
GCC_BIN = os.path.join(BUNDLE, r"gnu-tools-for-stm32\14.3.1+st.2\bin")
BUILD_DIR = os.path.join(PROJECT, "build", "Debug")
PRESET = "Debug"


def run(cmd, cwd=PROJECT, env=None):
    """Run a command with inherited stdio.

    Output is deliberately NOT captured: this sandbox forbids opening named
    pipes, so subprocess with the default stdio='pipe' fails with EPERM.
    """
    print("+ %s" % " ".join(cmd), flush=True)
    proc = subprocess.run(cmd, cwd=cwd, env=env if env is not None else os.environ)
    return proc.returncode


def main():
    args = sys.argv[1:]
    configure_only = "--configure-only" in args
    clean = "--clean" in args

    for path in (CMAKE, NINJA):
        if not os.path.isfile(path):
            print("ERROR: missing tool: %s" % path)
            return 2

    env = dict(os.environ)
    env["PATH"] = GCC_BIN + os.pathsep + env.get("PATH", "")

    if clean and os.path.isdir(BUILD_DIR):
        import shutil
        shutil.rmtree(BUILD_DIR, ignore_errors=True)
        print("removed %s" % BUILD_DIR)

    rc = run([CMAKE, "--preset", PRESET, "-G", "Ninja",
              "-DCMAKE_MAKE_PROGRAM=" + NINJA], env=env)
    if rc != 0:
        print("CONFIGURE FAILED (rc=%d)" % rc)
        return rc

    if configure_only:
        print("configure OK")
        return 0

    rc = run([CMAKE, "--build", "--preset", PRESET], env=env)
    if rc != 0:
        print("BUILD FAILED (rc=%d)" % rc)
    else:
        print("BUILD OK")
    return rc


if __name__ == "__main__":
    sys.exit(main())
