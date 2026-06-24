#!/usr/bin/env python3
import struct, zlib, os, math, sys

W, H = 128, 128

def chunk(t, d):
    return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)

def png(buf, w, h):
    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)
    raw = b''.join(b'\x00' + buf[y * w * 4:(y + 1) * w * 4] for y in range(h))
    return sig + chunk(b'IHDR', ihdr) + chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b'')

pixels = bytearray()
for y in range(H):
    for x in range(W):
        u, v = x / (W - 1), y / (H - 1)
        cx, cy = u - 0.5, v - 0.5
        r = math.sqrt(cx * cx + cy * cy)
        ang = math.atan2(cy, cx)
        warm = (math.sin(ang * 3.0) * 0.5 + 0.5) * 0.18
        base_r = int(220 - r * 260 + warm * 40)
        base_g = int(170 - r * 220 + warm * 20)
        base_b = int(110 - r * 200)
        ring = 1.0 if abs(r - 0.30) < 0.012 else 0.0
        ring += 1.0 if abs(r - 0.18) < 0.008 else 0.0
        center = 1.0 - min(1.0, r / 0.06)
        r2 = max(0, min(255, base_r + int(ring * 90) + int(center * 180)))
        g2 = max(0, min(255, base_g + int(ring * 70) + int(center * 150)))
        b2 = max(0, min(255, base_b + int(ring * 40) + int(center * 120)))
        pixels += bytes([r2, g2, b2, 255])

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'pack.png')
with open(out, 'wb') as f:
    f.write(png(bytes(pixels), W, H))
print('wrote', out, os.path.getsize(out), 'bytes')
