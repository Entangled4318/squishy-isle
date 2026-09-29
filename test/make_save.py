"""Writes a version 2 save with chosen friends, for screenshots and tests.
Usage: make_save.py FILE FOUND_IDS FOLLOWER_IDS [GATES [AREA [PARADES [POS]]]]   (comma lists, e.g. 0,3,7 5,2)
POS: x,y where Pip stood still (Continue puts him there); default none (the area's start).
PARADES: bit n = area n's full-page parade has played (default: every full area).
GATES: bit n = the way into area n + 1 is built (its scene played). By
default (or '-') every way that is open is built, so tests skip the gate
scene. AREA: the area Pip starts in (at its spawn), default 0."""
import struct
import zlib
import sys


def checksum(b):
    return zlib.crc32(bytes(b)) & 0xFFFFFFFF      # CRC-32, as save.c writes (step 7)


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
parades = sum(1 << a for a in range(4) if sum(f[a * 20:a * 20 + 20]) == 20)   # full pages: parade already seen
if len(sys.argv) > 6 and sys.argv[6] != '-':
    parades = int(sys.argv[6])                               # PARADES: bit n = area n's parade played
pos = (0, 0)
if len(sys.argv) > 7:
    pos = tuple(int(v) for v in sys.argv[7].split(','))      # POS: Pip's saved spot
body += bytes(fol) + b'\0' + struct.pack('<HHBB', pos[0], pos[1], area, gates) + bytes(11) + bytes([parades]) + bytes(14)
assert len(body) == 140
slot = body + struct.pack('<I', checksum(body))
sram = bytearray(b'\xff' * 0x8000)
sram[0:len(slot)] = slot
open(sys.argv[1], 'wb').write(sram)
