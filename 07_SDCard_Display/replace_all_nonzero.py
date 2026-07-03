# Below are options to run the script
# python replace_all_nonzero.py input-black.txt output-all-red.txt


import sys
import re

RED = "0xf800"
ZERO = "0x0000"

def process_line(line):
    # Split by commas to get tokens, preserving empty tokens if any
    parts = line.split(',')
    new_parts = []
    for p in parts:
        p = p.strip()
        if p == "":
            new_parts.append("")
            continue
        # Normalize to lower case for comparison
        if p.lower() == ZERO:
            new_parts.append(ZERO)   # keep exactly "0x0000"
        else:
            new_parts.append(RED)    # replace with "0xf800"
    # Rejoin with comma and space, but keep original spaces if desired?
    # Here we join with ', ' for readability.
    return ', '.join(new_parts)

def main():
    if len(sys.argv) != 3:
        print("Usage: python replace_nonzero.py <input_file> <output_file>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    with open(input_file, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    processed_lines = []
    for line in lines:
        # Preserve empty lines
        if line.strip() == '':
            processed_lines.append('')
        else:
            processed_lines.append(process_line(line.rstrip('\n')))

    with open(output_file, 'w', encoding='utf-8') as f:
        f.write('\n'.join(processed_lines))

    print(f"Done. Processed {len(processed_lines)} lines. Output written to {output_file}")

if __name__ == '__main__':
    main()