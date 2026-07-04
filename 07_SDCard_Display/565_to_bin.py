#!/usr/bin/env python3
"""
Convert a C header with PROGMEM array (e.g. clockhand.h) to a binary .b565 file.
Usage:
    python 565_to_bin.py input.h output.b565

The input file must contain:
    #define SOMETHING_WIDTH  <number>
    #define SOMETHING_HEIGHT <number>
and a 'static const uint16_t name[] PROGMEM = { ... }' with hex values.
The output is a raw binary file with:
    height (uint16_t, little‑endian),
    width  (uint16_t, little‑endian),
    pixel data (uint16_t, little‑endian) in row‑major order.
"""

import sys
import re
import struct

def parse_defines(text):
    """Extract width/height from any #define ending with WIDTH/HEIGHT."""
    width = height = None
    w_name = h_name = None
    for match in re.finditer(r'#define\s+(\w+)\s+(\d+)', text):
        name, val = match.group(1), int(match.group(2))
        if name.upper().endswith('WIDTH') and width is None:
            width, w_name = val, name
        elif name.upper().endswith('HEIGHT') and height is None:
            height, h_name = val, name
        if width is not None and height is not None:
            break
    return width, height, w_name, h_name

def parse_hex_numbers(text):
    """Extract all 0xXXXX tokens and return list of ints."""
    return [int(m, 16) for m in re.findall(r'0x[0-9A-Fa-f]{1,4}', text)]

def main():
    if len(sys.argv) != 3:
        print("Usage: python 565_to_bin.py input.h output.b565")
        sys.exit(1)

    input_file, output_file = sys.argv[1], sys.argv[2]

    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Extract dimensions
    width, height, w_name, h_name = parse_defines(content)
    if width is None or height is None:
        print("Error: Could not find WIDTH/HEIGHT defines in input.", file=sys.stderr)
        sys.exit(1)
    print(f"Detected dimensions: {width}x{height} (from {w_name}, {h_name})", file=sys.stderr)

    # 2. Extract pixel data
    pixels = parse_hex_numbers(content)
    expected = width * height
    if len(pixels) != expected:
        print(f"Warning: Found {len(pixels)} pixels, expected {expected}.", file=sys.stderr)
        if len(pixels) < expected:
            print("Padding missing pixels with 0x0000", file=sys.stderr)
            pixels += [0] * (expected - len(pixels))
        else:
            print("Truncating to expected count", file=sys.stderr)
            pixels = pixels[:expected]

    # 3. Write binary file
    with open(output_file, 'wb') as out:
        # height, width as little-endian uint16_t
        out.write(struct.pack('<HH', height, width))
        # pixel data as little-endian uint16_t
        for p in pixels:
            out.write(struct.pack('<H', p))

    print(f"Converted {input_file} -> {output_file}: {height}x{width}, {len(pixels)} pixels", file=sys.stderr)

if __name__ == "__main__":
    main()