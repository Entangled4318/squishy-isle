"""Writes a step 1/2 style (version 1) save, to test the upgrade to the new layout.
Usage: make_v1_save.py FILE"""
import struct
import sys

sram = bytearray(b'\xff' * 0x8000)
slot = struct.pack('<IHHHHI', 0x53495153, 1, 7, 3, 2, 42) + bytes(32) + struct.pack('<I', 0xDEADBEEF)
sram[0:len(slot)] = slot
open(sys.argv[1], 'wb').write(sram)
