# scale_image.py
# python scale_image.py 0.5 clockhand.h scaled_clockhand.h
# input file format:
"""
#define CLOCKHAND_WIDTH 28
#define CLOCKHAND_HEIGHT 185

// array size is 10360
static const uint16_t clockhand[] PROGMEM = {
  0x0000, 0x0000, 0x0000, 0x0000, 
  :
  :
  0x0000, 0x0000, 0x0000
};

"""
#!/usr/bin/env python3
"""
Scale an RGB565 image from a C PROGMEM array definition.
Usage: python3 scale_image.py <scale> [input_file] [output_file]
Example: python3 scale_image.py 0.5 clockhand.h scaled_clockhand.h
If no input_file, reads from stdin. If no output_file, prints to stdout.
"""

import sys
import re

# Default dimensions (fallback if not found in file)
DEFAULT_WIDTH = 28
DEFAULT_HEIGHT = 185

def parse_defines(text):
    """
    Extract width and height from #define lines.
    Looks for any macro whose name ends with WIDTH or HEIGHT.
    Returns (width, height, found_width_name, found_height_name)
    """
    width = DEFAULT_WIDTH
    height = DEFAULT_HEIGHT
    found_w = None
    found_h = None

    # Find all #define lines: #define NAME VALUE
    define_pattern = re.compile(r'#define\s+(\w+)\s+(\d+)')
    for match in define_pattern.finditer(text):
        name = match.group(1)
        value = int(match.group(2))
        # Check if name ends with WIDTH or HEIGHT (case-insensitive)
        if name.upper().endswith('WIDTH') and found_w is None:
            width = value
            found_w = name
        elif name.upper().endswith('HEIGHT') and found_h is None:
            height = value
            found_h = name

    return width, height, found_w, found_h

def parse_hex_numbers(text):
    """Extract all 0xXXXX hex numbers from text, return list of ints."""
    pattern = r'0x[0-9A-Fa-f]{1,4}'
    matches = re.findall(pattern, text)
    return [int(m, 16) for m in matches]

def scale_image(pixels, orig_w, orig_h, scale):
    """Nearest-neighbour scaling of a flat list (row-major)."""
    out_w = max(1, int(round(orig_w * scale)))
    out_h = max(1, int(round(orig_h * scale)))
    scaled = []
    for y in range(out_h):
        src_y = (y * orig_h) // out_h
        for x in range(out_w):
            src_x = (x * orig_w) // out_w
            idx = src_y * orig_w + src_x
            scaled.append(pixels[idx])
    return out_w, out_h, scaled

def format_array(array, width, name="scaledImage", line_width=12):
    """Format the array as a C PROGMEM array with line breaks."""
    lines = []
    lines.append(f"const uint16_t {name}[] PROGMEM = {{")
    for i, val in enumerate(array):
        if i % line_width == 0:
            lines.append("  ")
        lines[-1] += f"0x{val:04x}, "
        if (i + 1) % line_width == 0:
            lines.append("")
    # Remove trailing comma and space from last line
    if lines[-1].endswith(", "):
        lines[-1] = lines[-1][:-2]
    lines.append("};")
    return "\n".join(lines)

def main():
    # Parse command line arguments
    if len(sys.argv) < 2:
        print("Usage: python3 scale_image.py <scale> [input_file] [output_file]")
        sys.exit(1)
    
    try:
        scale = float(sys.argv[1])
    except ValueError:
        print("Error: scale must be a number")
        sys.exit(1)

    if not (0 < scale < 1):
        print("Warning: scale is not < 1 (downscaling), but will still work.")

    # Read input
    if len(sys.argv) >= 3:
        with open(sys.argv[2], 'r') as f:
            text = f.read()
    else:
        text = sys.stdin.read()

    # Extract dimensions from #define lines
    orig_w, orig_h, found_w, found_h = parse_defines(text)

    # Print what was found
    if found_w:
        print(f"Found width define: {found_w} = {orig_w}", file=sys.stderr)
    else:
        print(f"Warning: No WIDTH define found, using default {DEFAULT_WIDTH}", file=sys.stderr)

    if found_h:
        print(f"Found height define: {found_h} = {orig_h}", file=sys.stderr)
    else:
        print(f"Warning: No HEIGHT define found, using default {DEFAULT_HEIGHT}", file=sys.stderr)

    print(f"Using original dimensions: {orig_w}x{orig_h}", file=sys.stderr)

    # Extract pixel data
    pixels = parse_hex_numbers(text)
    expected = orig_w * orig_h
    if len(pixels) != expected:
        print(f"Warning: Found {len(pixels)} pixels, expected {expected}. Continuing anyway.", file=sys.stderr)

    # Scale
    out_w, out_h, scaled = scale_image(pixels, orig_w, orig_h, scale)
    print(f"Scaling to {out_w}x{out_h} pixels...", file=sys.stderr)

    # Generate output
    header = f"// Scaled from {orig_w}x{orig_h} by factor {scale:.3f}\n"
    header += f"#define SCALED_WIDTH  {out_w}\n"
    header += f"#define SCALED_HEIGHT {out_h}\n\n"
    body = format_array(scaled, out_w)

    output_text = header + body

    # Write output
    if len(sys.argv) >= 4:
        with open(sys.argv[3], 'w') as f:
            f.write(output_text)
        print(f"Output written to {sys.argv[3]}", file=sys.stderr)
    else:
        print(output_text)

if __name__ == "__main__":
    main()
