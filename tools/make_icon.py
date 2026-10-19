#!/usr/bin/env python3
import argparse, struct, zlib

def chunk(typ, data):
    return (struct.pack('>I', len(data)) + typ + data +
            struct.pack('>I', zlib.crc32(typ + data) & 0xFFFFFFFF))

def png_solid(w, h, rgba):
    raw = (bytes([0]) + bytes(rgba) * w) * h
    return (b'\x89PNG\r\n\x1a\n' +
            chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(raw, 9)) +
            chunk(b'IEND', b''))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    ap.add_argument('--color', default='3366cc')
    a = ap.parse_args()
    r = int(a.color[0:2], 16); g = int(a.color[2:4], 16); b = int(a.color[4:6], 16)
    with open(a.out, 'wb') as f:
        f.write(png_solid(512, 512, (r, g, b, 255)))
    print(f'Wrote {a.out}')

if __name__ == '__main__':
    main()
