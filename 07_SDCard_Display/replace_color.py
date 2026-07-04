#!/usr/bin/env python3
"""
Usage:
    python replace_color.py [input_file] [output_file]
Example:
    python replace_color.py input-black.h output-red.h
If no input_file, reads from stdin. If no output_file, prints to stdout.

Replaces pixels based on SEARCH_COLOR and REPLACE_MATCH:
- If REPLACE_MATCH == 1 : replace matching pixels with REPLACE_COLOR
- If REPLACE_MATCH == 0 : replace NON-matching pixels with REPLACE_COLOR
Image dimensions are read from #define lines (any macro ending with WIDTH / HEIGHT).
"""

import sys
import re

# ===========================================================================
#  USER SETTINGS – change these values as needed
# ===========================================================================
SEARCH_COLOR = 0x0000      # color to match (e.g., black)
REPLACE_COLOR = 0xf800     # color to replace with (e.g., red)
REPLACE_MATCH = 1          # 1 = replace matching pixels, 0 = replace non-matching
# ===========================================================================

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

def replace_color(pixels, search_col, replace_col, match_flag):
    """
    Return a new list with pixels replaced according to match_flag.
    If match_flag == 1: replace pixels equal to search_col.
    If match_flag == 0: replace pixels NOT equal to search_col.
    """
    if match_flag == 1:
        return [replace_col if p == search_col else p for p in pixels]
    else:
        return [replace_col if p != search_col else p for p in pixels]

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
    # Parse command line arguments (only input/output files, no colour args)
    if len(sys.argv) >= 2:
        input_file = sys.argv[1]
        with open(input_file, 'r') as f:
            text = f.read()
    else:
        text = sys.stdin.read()

    # Extract dimensions
    width, height, w_name, h_name = parse_defines(text)
    if width is None or height is None:
        print("Error: Could not find WIDTH/HEIGHT defines in input.", file=sys.stderr)
        sys.exit(1)

    # Determine mode description
    mode_desc = "matching" if REPLACE_MATCH == 1 else "non-matching"
    print(f"Found dimensions: {width} x {height} (from {w_name}, {h_name})", file=sys.stderr)
    print(f"Replacing {mode_desc} pixels of 0x{SEARCH_COLOR:04x} with 0x{REPLACE_COLOR:04x}", file=sys.stderr)

    # Extract pixel data
    pixels = parse_hex_numbers(text)
    expected = width * height
    if len(pixels) != expected:
        print(f"Warning: Found {len(pixels)} pixels, expected {expected}. Continuing anyway.", file=sys.stderr)
        pixels = pixels[:expected]  # truncate if too many

    # Show a few original pixels for debugging
    if len(pixels) > 0:
        sample = pixels[:10]
        print(f"First 10 original pixels: {', '.join(f'0x{p:04x}' for p in sample)}", file=sys.stderr)

    # Replace colours
    modified = replace_color(pixels, SEARCH_COLOR, REPLACE_COLOR, REPLACE_MATCH)

    # Show a few modified pixels for debugging
    if len(modified) > 0:
        sample = modified[:10]
        print(f"First 10 modified pixels: {', '.join(f'0x{p:04x}' for p in sample)}", file=sys.stderr)

    # Count affected pixels
    if REPLACE_MATCH == 1:
        affected_count = sum(1 for p in pixels if p == SEARCH_COLOR)
    else:
        affected_count = sum(1 for p in pixels if p != SEARCH_COLOR)
    print(f"Replaced {affected_count} pixels", file=sys.stderr)

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
    output += format_array(modified, width, array_name)

    # Write output
    if len(sys.argv) >= 3:
        output_file = sys.argv[2]
        with open(output_file, 'w') as f:
            f.write(output)
        print(f"Output written to {output_file}", file=sys.stderr)
    else:
        print(output)

if __name__ == "__main__":
    main()