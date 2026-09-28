"""Writes a version 2 save with chosen friends, for screenshots and tests.
Usage: make_save.py FILE FOUND_IDS FOLLOWER_IDS [GATES [AREA]]   (comma lists, e.g. 0,3,7 5,2)
GATES: bit n = the way into area n + 1 is built (its scene played). By
default (or '-') every way that is open is built, so tests skip the gate
scene. AREA: the area Pip starts in (at its spawn), default 0."""
import struct
import sys


def checksum(b):
    s = 0x1234ABCD
    for v in b:
        s = ((s << 5) + (s >> 27) + v) & 0xFFFFFFFF
    return s


found = [int(v) for v in sys.argv[2].split(',') if v]
follow = [int(v) for v in sys.argv[3].split(',') if v] if len(sys.argv) > 3 else []
f = bytearray(80)
for i in found:
    f[i] = 1
fol = bytearray(3)
for k, i in enumerate(follow[:3]):
    fol[k] = i + 1
body = struct.pack('<IHHHHI', 0x53495153, 2, 1, 1, 0, 0) + bytes(f) + struct.pack('<II', len(found), 0x1234567)
gates = sum(1 << (a - 1) for a in (1, 2, 3) if sum(f[(a - 1) * 20:a * 20]) >= 10)
if len(sys.argv) > 4 and sys.argv[4] != '-':
    gates = int(sys.argv[4])
area = int(sys.argv[5]) if len(sys.argv) > 5 else 0      # AREA: where Pip starts (at the area's spawn)
body += bytes(fol) + b'\0' + struct.pack('<HHBB', 0, 0, area, gates) + bytes(26)
assert len(body) == 140
slot = body + struct.pack('<I', checksum(body))
sram = bytearray(b'\xff' * 0x8000)
sram[0:len(slot)] = slot
open(sys.argv[1], 'wb').write(sram)
