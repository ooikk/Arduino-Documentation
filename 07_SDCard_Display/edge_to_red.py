# python edge_to_red.py < input.txt > output.txt
# Use below command if above doesn't work.
# cmd /c "python edge_to_red.py < input-black.txt > output-black.txt"

import sys

RED = 0xF800
WIDTH = 28

def replace_edge_pixels(data):
    height = len(data)
    out = [row[:] for row in data]

    for y in range(height):
        for x in range(WIDTH):
            if data[y][x] == 0:
                continue

            # Pixel is on the edge if any 4-connected neighbor is background/out of bounds
            if (x == 0 or x == WIDTH - 1 or y == 0 or y == height - 1 or
                data[y - 1][x] == 0 or data[y + 1][x] == 0 or
                data[y][x - 1] == 0 or data[y][x + 1] == 0):
                out[y][x] = RED

    return out


# Read all hex tokens from stdin
text = sys.stdin.read().replace('\n', ' ').replace(',', ' ')
tokens = [int(t, 0) for t in text.split() if t.startswith('0x')]

# Assume width is 28; adjust WIDTH if needed
data = [tokens[i * WIDTH:(i + 1) * WIDTH] for i in range(len(tokens) // WIDTH)]

result = replace_edge_pixels(data)

for row in result:
    print(', '.join(f'0x{v:04x}' for v in row) + ',')