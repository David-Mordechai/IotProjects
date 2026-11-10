"""Watch two ESP32 boards at the same time, with timestamps, without restarting them.

Usage:  uv run --with pyserial python tools/monitor_two.py COM3 COM5 60
        (first port, second port, seconds)

Useful for the radio projects: sender on one port, receiver on the other.
Close any `pio device monitor` on these ports first.
"""
import sys
import threading
import time

import serial

port_a, port_b, secs = sys.argv[1], sys.argv[2], float(sys.argv[3])
lines = []


def watch(port):
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = port, 115200, 0.2
    s.dtr = False      # set before open(), so opening the port doesn't reset the board
    s.rts = False
    s.open()
    t0 = time.time()
    while time.time() - t0 < secs:
        chunk = s.read(256)
        for line in chunk.decode("utf-8", "replace").splitlines():
            if line.strip():
                lines.append((time.time(), port, line.strip()))
    s.close()


threads = [threading.Thread(target=watch, args=(p,)) for p in (port_a, port_b)]
for t in threads:
    t.start()
for t in threads:
    t.join()

start = min((t for t, _, _ in lines), default=0)
for t, port, line in sorted(lines):
    print(f"{t - start:6.2f}s {port}  {line}".encode("ascii", "replace").decode())
