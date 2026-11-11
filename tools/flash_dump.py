"""Read the log a board recorded in its flash (radio-hello receiver_rec, espnow-hello), without restarting it.

Usage:  uv run --with pyserial python tools/flash_dump.py COM5 firmware/radio-hello/logs/walk.txt
        uv run --with pyserial python tools/flash_dump.py COM5 --erase
        (port, output file; --erase clears the board's log instead)

Plug the board into the PC first (it can keep running; the dump doesn't reset it).
Logs go in logs/ (git-ignored: they contain GPS positions).
"""
import sys
import time
from pathlib import Path

import serial

port, arg = sys.argv[1], sys.argv[2]

s = serial.Serial()
s.port, s.baudrate, s.timeout = port, 115200, 0.2
s.dtr = False          # set before open(), so opening the port doesn't reset the board
s.rts = False
s.open()
time.sleep(0.3)
s.reset_input_buffer()

if arg == "--erase":
    s.write(b"x")
    end = time.time() + 3
    buf = b""
    while time.time() < end and b"erased" not in buf:
        buf += s.read(256)
    print("erased" if b"erased" in buf else "no answer from the board (does its firmware have the flash recorder?)")
    sys.exit(0)

s.write(b"d")
buf = b""
end = time.time() + 120            # 1.4 MB at 115200 baud takes ~2 min at most
last = time.time()
while time.time() < end and b"=== DUMP END ===" not in buf:
    chunk = s.read(4096)
    buf += chunk
    if chunk:
        last = time.time()
    elif time.time() - last > 5 and b"=== DUMP START ===" not in buf:
        break                      # board never started a dump
s.close()

if b"=== DUMP START ===" not in buf:
    print("no dump received (does its firmware have the flash recorder, and is the port free?)")
    sys.exit(1)
body = buf.split(b"=== DUMP START ===", 1)[1].split(b"=== DUMP END ===", 1)[0].strip(b"\r\n")
out = Path(arg)
out.parent.mkdir(parents=True, exist_ok=True)
text = body.decode("utf-8", "replace").replace("\r\n", "\n")
out.write_text(text + "\n", encoding="ascii", errors="replace")
lines = text.count("\n") + 1 if text else 0
complete = b"=== DUMP END ===" in buf
print(f"{lines} lines -> {out}" + ("" if complete else "  (WARNING: dump cut off, end marker missing)"))
