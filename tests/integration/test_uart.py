import os
import re
import select
import subprocess
import time

EXE = "build/zephyr/zephyr.exe"


def read_until(fd, token, timeout=3):
    buf, end = b"", time.time() + timeout
    while time.time() < end and token not in buf:
        if select.select([fd], [], [], 0.1)[0]:
            buf += os.read(fd, 256)
    return buf


def test_session():
    p = subprocess.Popen([EXE], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    path = None
    for line in p.stdout:
        m = re.search(r"uart_1 connected to pseudotty: (\S+)", line)
        if m:
            path = m.group(1)
            break
    assert path
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY)
    try:
        read_until(fd, b"> ")
        for cmd, expect in [
            ("HELP", b"ECHO"),
            ("LED ON", b"OK"),
            ("LED", b"LED ON"),
            ("BTN", b"BTN"),
            ("ECHO oi", b"oi"),
            ("XYZ", b"ERR: unknown command"),
            ("a" * 100, b"ERR: line too long"),
        ]:
            os.write(fd, (cmd + "\r").encode())
            assert expect in read_until(fd, expect)
    finally:
        os.close(fd)
        p.kill()
