"""Restart an ESP32 and print what it sends over USB for a few seconds.

Usage:  uv run --with pyserial python tools/serial_read.py COM3 10
        (port, seconds; default 6 s)

The board is restarted (EN pulse via the RTS line) so you see its startup
messages too. Close any `pio device monitor` on the same port first.
"""
import sys
import time

import serial

port = sys.argv[1]
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 6

s = serial.Serial(port, 115200, timeout=0.2)
s.dtr = False
s.rts = True          # hold EN low = reset
time.sleep(0.1)
s.rts = False         # release = boot normally
end = time.time() + secs
buf = b""
while time.time() < end:
    buf += s.read(512)
s.close()
# ASCII only, so radio noise bytes can't crash the Windows console
sys.stdout.buffer.write(buf.decode("utf-8", "replace").encode("ascii", "replace"))
print()
