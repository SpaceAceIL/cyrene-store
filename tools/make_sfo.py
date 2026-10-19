#!/usr/bin/env python3
"""Generate a minimal PS4 param.sfo.

Format codes:
  0x0404 = uint32 (4 bytes)
  0x0204 = UTF-8 string (null-terminated, size includes null)
  0x0004 = UTF-8 string null-terminated (rarely used)
"""
import argparse, struct, sys

def as_int(n):
    return (0x0404, struct.pack('<I', n & 0xFFFFFFFF))

def as_str(text, pad_to=None):
    data = text.encode('utf-8') + b'\x00'
    size = len(data)
    if pad_to and pad_to > size:
        # pad with extra nulls for fixed-size fields like APP_VER
        data = data + b'\x00' * (pad_to - size)
    return (0x0204, data, size)  # (format, data, reported_size)

def build(entries):
    keys = sorted(entries.keys())

    # Header: 20 bytes
    # Then index: 16 bytes per entry
    # Then key table: null-terminated keys, padded to 4
    # Then data table: values, padded to 4

    header_size = 20
    index_size = len(keys) * 16
    key_table_start = header_size + index_size

    key_table = b''
    key_offsets = {}
    for k in keys:
        key_offsets[k] = len(key_table)
        key_table += k.encode('ascii') + b'\x00'
    key_table += b'\x00' * ((-len(key_table)) % 4)

    data_start = key_table_start + len(key_table)
    data_table = b''
    data_offsets = {}
    data_sizes = {}
    for k in keys:
        data_offsets[k] = len(data_table)
        entry = entries[k]
        if entry[0] == 0x0404:
            data_table += entry[1]
            data_sizes[k] = len(entry[1])
        else:
            data_table += entry[1]
            data_sizes[k] = entry[2]  # reported size (may be less than padded)
        data_table += b'\x00' * ((-len(data_table)) % 4)

    out = b'\x00PSF'
    out += struct.pack('<I', 0x0101)           # version
    out += struct.pack('<I', key_table_start)  # key table offset
    out += struct.pack('<I', data_start)       # data table offset
    out += struct.pack('<I', len(keys))        # entry count

    for k in keys:
        fmt = entries[k][0]
        size = data_sizes[k]
        max_size = size
        if fmt == 0x0204:
            # max_size should cover null-padded full width
            max_size = max(size, ((size + 3) // 4) * 4)
        out += struct.pack('<HHIII',
            key_offsets[k], fmt, size, max_size, data_offsets[k])

    out += key_table
    out += data_table
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', required=True)
    ap.add_argument('--title', required=True)
    ap.add_argument('--title-id', required=True)
    ap.add_argument('--version', default='01.00')
    ap.add_argument('--content-id', required=True)
    a = ap.parse_args()

    entries = {
        'APP_TYPE':        as_int(0),                       # paid app
        'APP_VER':         as_str(a.version, pad_to=8),     # "01.00\0\0\0"
        'ATTRIBUTE':       as_int(0),
        'CATEGORY':        as_str('gd'),                    # game digital
        'CONTENT_ID':      as_str(a.content_id),
        'FORMAT':          as_str('obs'),
        'PARENTAL_LEVEL':  as_int(1),
        'PUBTOOLINFO':     as_str('c_date=20240101,encryption_key=1,content_ver=01.00,submission=0'),
        'SDK_VER':         as_str('03000000'),              # 3.00
        'SYSTEM_VER':      as_int(0),
        'TITLE':           as_str(a.title, pad_to=128),
        'TITLE_ID':        as_str(a.title_id, pad_to=12),   # 9 chars + null + pad
        'VERSION':         as_str(a.version, pad_to=8),
    }

    data = build(entries)
    with open(a.out, 'wb') as f:
        f.write(data)
    print(f'Wrote {a.out} ({len(data)} bytes)')

if __name__ == '__main__':
    main()
