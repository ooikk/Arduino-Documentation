## In your terminal (where Annecy.565 is located):
## run below command
## python 565_to_bin.py Annecy.565 Annecy.b565
##

import re
import sys

def convert_text_565_to_binary(input_path, output_path):
    with open(input_path, 'r') as f:
        content = f.read()
    
    # Remove all whitespace (spaces, newlines, tabs)
    compact = re.sub(r'\s+', '', content)
    
    # Format: height,width,{...};
    match = re.match(r'(\d+),(\d+),\{([^}]+)\};?', compact)
    if not match:
        raise ValueError("Invalid .565 text format: expected height,width,{...}")
    
    height = int(match.group(1))   # first number is ALWAYS height
    width  = int(match.group(2))   # second number is ALWAYS width
    hex_part = match.group(3).strip(',')
    
    # Split and clean
    hex_strings = [hs for hs in hex_part.split(',') if hs and hs.startswith('0x')]
    pixels = [int(hs, 16) for hs in hex_strings]
    
    expected = height * width
    if len(pixels) != expected:
        raise ValueError(f"Pixel count mismatch: {len(pixels)} vs {expected}")
    
    with open(output_path, 'wb') as out:
        out.write(height.to_bytes(2, 'little'))
        out.write(width.to_bytes(2, 'little'))
        for p in pixels:
            out.write(p.to_bytes(2, 'little'))
    
    print(f"Converted {input_path}: {height}x{width}, {len(pixels)} pixels -> {output_path}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python bin_conv.py input.565 output.bin")
    else:
        convert_text_565_to_binary(sys.argv[1], sys.argv[2])
