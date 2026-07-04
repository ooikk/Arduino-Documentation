#!/usr/bin/env python3
"""
Batch convert all .h files in the current directory to .b565 binary files.
Usage:
    python 565_to_bin_batch.py

The input .h files must contain:
    #define SOMETHING_WIDTH  <number>
    #define SOMETHING_HEIGHT <number>
and a 'static const uint16_t name[] PROGMEM = { ... }' with hex values.
Output files are named <basename>.b565.
"""

import re
import glob
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

def convert_h_to_binary(input_path, output_path):
    """Convert a single .h header file to binary .b565."""
    with open(input_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Extract dimensions
    width, height, w_name, h_name = parse_defines(content)
    if width is None or height is None:
        raise ValueError("Could not find WIDTH/HEIGHT defines in input.")

    # Extract pixel data
    pixels = parse_hex_numbers(content)
    expected = width * height
    if len(pixels) != expected:
        print(f"⚠️  {input_path}: Found {len(pixels)} pixels, expected {expected}. Padding/truncating.")
        if len(pixels) < expected:
            pixels += [0] * (expected - len(pixels))
        else:
            pixels = pixels[:expected]

    # Write binary file
    with open(output_path, 'wb') as out:
        out.write(struct.pack('<HH', height, width))   # height, width (little-endian)
        for p in pixels:
            out.write(struct.pack('<H', p))

    print(f"✅ {input_path}: {height}x{width}, {len(pixels)} pixels -> {output_path}")

def batch_convert():
    files = glob.glob("*.h")
    if not files:
        print("No .h files found in current directory.")
        return

    print(f"Found {len(files)} .h file(s). Converting...\n")
    for input_file in files:
        output_file = input_file.replace(".h", ".b565")
        try:
            convert_h_to_binary(input_file, output_file)
        except Exception as e:
            print(f"❌ Error converting {input_file}: {e}\n")

if __name__ == "__main__":
    batch_convert()