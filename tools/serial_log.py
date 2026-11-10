"""Record one board's output to a file, with PC time on every line, without restarting it.

Usage:  uv run --with pyserial python tools/serial_log.py COM3 2400 logs/range-test.txt
        (port, seconds, output file)

Used for the range test: the receiver stays on the PC and this records the whole walk.
Logs go in logs/ (git-ignored: they contain GPS positions).
"""
import sys
import time
from datetime import datetime
from pathlib import Path

import serial

port, secs, out = sys.argv[1], float(sys.argv[2]), Path(sys.argv[3])
out.parent.mkdir(parents=True, exist_ok=True)

s = serial.Serial()
s.port, s.baudrate, s.timeout = port, 115200, 0.2
s.dtr = False          # set before open(), so opening the port doesn't reset the board
s.rts = False
s.open()

end = time.time() + secs
buf = b""
with out.open("a", encoding="ascii", errors="replace") as f:
    while time.time() < end:
        buf += s.read(512)
        while b"\n" in buf:
            line, buf = buf.split(b"\n", 1)
            text = line.decode("utf-8", "replace").strip()
            if text:
                f.write(f"{datetime.now():%H:%M:%S} {text}\n")
                f.flush()      # so the file is readable while recording
s.close()
