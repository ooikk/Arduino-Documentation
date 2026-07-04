#!/usr/bin/env python3
# python draw_edge_line.py input-black.h output-red.h
"""
Usage:
    python draw_edge_line.py [input_file] [output_file]
If no input_file, reads from stdin. If no output_file, prints to stdout.

This script finds the edge pixels of a non‑background object in an RGB565 image
defined as a C PROGMEM array, and colours those edge pixels with EDGE_COLOR.
The image dimensions are read from #define WIDTH / #define HEIGHT lines
(any prefix is accepted, e.g. CLOCKHAND_WIDTH, ARROW_WIDTH, etc.).
"""

import sys
import re

# --- User settings ---
BG_COLOR   = 0x0000      # background colour (black)
EDGE_COLOR = 0xF800      # colour to replace edge pixels (red)

# --- Helper functions ---

def parse_defines(text):
    """
    Extract width and height from #define lines.
    Looks for any macro whose name ends with WIDTH or HEIGHT (case-insensitive).
    Returns (width, height, width_macro_name, height_macro_name).
    """
    width = None
    height = None
    w_name = None
    h_name = None

    define_pattern = re.compile(r'#define\s+(\w+)\s+(\d+)')
    for match in define_pattern.finditer(text):
        name = match.group(1)
        value = int(match.group(2))
        if name.upper().endswith('WIDTH') and width is None:
            width = value
            w_name = name
        elif name.upper().endswith('HEIGHT') and height is None:
            height = value
            h_name = name
        if width is not None and height is not None:
            break

    return width, height, w_name, h_name

def parse_hex_numbers(text):
    """Extract all 0xXXXX hex numbers from text, return list of ints."""
    pattern = r'0x[0-9A-Fa-f]{1,4}'
    matches = re.findall(pattern, text)
    return [int(m, 16) for m in matches]

def replace_edge_pixels(data, width, height, bg, edge):
    """
    data: list of lists (rows) of pixel values.
    Returns a new list of lists with edge pixels replaced.
    """
    out = [row[:] for row in data]

    for y in range(height):
        for x in range(width):
            if data[y][x] == bg:
                continue

            # Check if pixel is on the edge of the image
            if (x == 0 or x == width - 1 or y == 0 or y == height - 1):
                out[y][x] = edge
                continue

            # Check 4‑connected neighbours
            if (data[y-1][x] == bg or data[y+1][x] == bg or
                data[y][x-1] == bg or data[y][x+1] == bg):
                out[y][x] = edge

    return out

def format_array(array, width, name="clockhand"):
    """
    Format the array as a C PROGMEM array with one image row per line.
    Each line has exactly `width` numbers, ending with a comma.
    """
    lines = []
    lines.append(f"static const uint16_t {name}[] PROGMEM = {{")
    for i in range(0, len(array), width):
        row = array[i:i+width]
        row_str = ", ".join(f"0x{v:04x}" for v in row)
        lines.append("  " + row_str + ",")
    lines.append("};")
    return "\n".join(lines)

def main():
    # Read input
    if len(sys.argv) >= 2:
        with open(sys.argv[1], 'r') as f:
            text = f.read()
    else:
        text = sys.stdin.read()

    # Extract dimensions
    width, height, w_name, h_name = parse_defines(text)
    if width is None or height is None:
        print("Error: Could not find WIDTH/HEIGHT defines in input.", file=sys.stderr)
        sys.exit(1)

    print(f"Found dimensions: {width} x {height} (from {w_name}, {h_name})", file=sys.stderr)

    # Extract pixel data
    pixels = parse_hex_numbers(text)
    expected = width * height
    if len(pixels) != expected:
        print(f"Warning: Found {len(pixels)} pixels, expected {expected}. Continuing anyway.", file=sys.stderr)
        pixels = pixels[:expected]   # truncate if too many, pad if too few?

    # Build 2D grid
    data = [pixels[i*width:(i+1)*width] for i in range(height)]

    # Process edge detection
    result = replace_edge_pixels(data, width, height, BG_COLOR, EDGE_COLOR)

    # Flatten for output
    flat = [pixel for row in result for pixel in row]

    # Extract array name from input
    array_name = "clockhand"  # default
    name_match = re.search(r'static\s+const\s+uint16_t\s+(\w+)\s*\[\s*\]', text)
    if name_match:
        array_name = name_match.group(1)
    else:
        name_match = re.search(r'const\s+uint16_t\s+(\w+)\s*\[\s*\]', text)
        if name_match:
            array_name = name_match.group(1)

    # Build output header (preserve original macro names)
    header_lines = []
    if w_name and h_name:
        header_lines.append(f"#define {w_name} {width}")
        header_lines.append(f"#define {h_name} {height}")
    else:
        header_lines.append(f"#define WIDTH {width}")
        header_lines.append(f"#define HEIGHT {height}")

    output = "\n".join(header_lines) + "\n\n"
    output += format_array(flat, width, array_name)

    # Write output
    if len(sys.argv) >= 3:
        with open(sys.argv[2], 'w') as f:
            f.write(output)
        print(f"Output written to {sys.argv[2]}", file=sys.stderr)
    else:
        print(output)

if __name__ == "__main__":
    main()