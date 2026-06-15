## run below command
## python 565_to_bin_batch.py
## all the file *.565 in current Directory will be converted to *.b565
##

import re
import os
import glob

def convert_text_565_to_binary(input_path, output_path):
    """Convert a single .565 text file to binary format."""
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
    
    print(f"✅ {input_path}: {height}x{width}, {len(pixels)} pixels -> {output_path}")

def batch_convert():
    # Find all .565 files in the current directory
    files = glob.glob("*.565")
    if not files:
        print("No .565 files found in current directory.")
        return
    
    print(f"Found {len(files)} .565 file(s). Converting...\n")
    
    for input_file in files:
        # Generate output filename: replace .565 with .b565
        output_file = input_file.replace(".565", ".b565")
        try:
            convert_text_565_to_binary(input_file, output_file)
        except Exception as e:
            print(f"❌ Error converting {input_file}: {e}\n")

if __name__ == "__main__":
    batch_convert()