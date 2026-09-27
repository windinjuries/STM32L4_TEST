#!/usr/bin/env python3
"""Send PC CPU, memory and time samples to the STM32 over USB CDC.

Install dependencies with:
    pip install psutil pyserial

Example:
    python Tool/system_info_sender.py
    python Tool/system_info_sender.py --port COM5
"""

import argparse
import datetime
import sys

try:
    import psutil
except ImportError:
    psutil = None

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    serial = None
    list_ports = None


STM32_VID = 0x0483
STM32_PID = 0x5740
STM32_PRODUCT = "STM32 Virtual ComPort"


def find_port(requested_port):
    if requested_port:
        return requested_port

    ports = list(list_ports.comports())
    matching = [
        port
        for port in ports
        if port.vid == STM32_VID and port.pid == STM32_PID
    ]
    if not matching:
        matching = [
            port
            for port in ports
            if STM32_PRODUCT.lower() in (port.product or "").lower()
        ]
    if len(matching) == 1:
        return matching[0].device
    if len(matching) > 1:
        names = ", ".join(port.device for port in matching)
        raise RuntimeError(f"Multiple STM32 CDC ports found: {names}; use --port")
    if len(ports) == 1:
        return ports[0].device
    if not ports:
        raise RuntimeError("No serial port found; connect the STM32 or use --port")

    names = ", ".join(port.device for port in ports)
    raise RuntimeError(f"Cannot identify STM32 CDC port; available ports: {names}")


def require_dependencies():
    if psutil is None:
        raise RuntimeError("Missing dependency: pip install psutil")
    if serial is None:
        raise RuntimeError("Missing dependency: pip install pyserial")


def read_metrics(sample_interval):
    cpu_percent = psutil.cpu_percent(interval=sample_interval, percpu=False)
    memory_percent = psutil.virtual_memory().percent
    current_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    return cpu_percent, memory_percent, current_time


def build_message(cpu_percent, memory_percent, current_time):
    return (
        f"SYS,cpu={cpu_percent:.1f},"
        f"mem={memory_percent:.1f},"
        f"time={current_time}\n"
    )


def main():
    parser = argparse.ArgumentParser(
        description="Send host system information to the STM32 USB CDC port."
    )
    parser.add_argument("--port", help="Serial port, for example COM5 or /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument(
        "--interval",
        type=float,
        default=1.0,
        help="CPU sampling window and send interval in seconds (default: 1.0)",
    )
    parser.add_argument("--once", action="store_true", help="Send one sample and exit")
    args = parser.parse_args()

    if args.interval <= 0:
        parser.error("--interval must be greater than zero")

    try:
        require_dependencies()
        port_name = find_port(args.port)
        print(f"Using USB CDC port: {port_name}")

        with serial.Serial(
            port=port_name,
            baudrate=args.baud,
            timeout=1,
            write_timeout=1,
        ) as device:
            psutil.cpu_percent(interval=None)
            while True:
                message = build_message(*read_metrics(args.interval))
                device.write(message.encode("ascii"))
                print(message.rstrip(), flush=True)
                if args.once:
                    break
    except KeyboardInterrupt:
        print("\nStopped.")
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
