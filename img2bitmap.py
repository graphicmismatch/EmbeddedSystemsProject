#!/usr/bin/env python3

import argparse
import os
import re
import sys

from PIL import Image

VALID_EXT = (".png", ".bmp", ".jpg", ".jpeg")


def sanitize_symbol(name):
    return re.sub(r"[^a-zA-Z0-9_]", "_", name)


def load_image(path):
    source = Image.open(path).convert("RGBA")
    background = Image.new("RGBA", source.size, (255, 255, 255, 255))
    composited = Image.alpha_composite(background, source)
    return composited.convert("L")


def convert_image(path, threshold, invert):
    img = load_image(path)
    width, height = img.size

    pixels = img.load()
    data = []

    # Adafruit GFX bitmap format:
    #  - row-major
    #  - each byte = 8 horizontal pixels
    byte_width = (width + 7) // 8

    for y in range(height):
        for bx in range(byte_width):
            byte = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < width:
                    lit = pixels[x, y] < threshold
                    if invert:
                        lit = not lit
                    if lit:
                        byte |= (1 << (7 - bit))
            data.append(byte)

    name = sanitize_symbol(os.path.splitext(os.path.basename(path))[0])

    return name, width, height, data


def emit_bitmap(name, width, height, data):
    print(f"// {name}: {width}x{height}")
    print(f"const uint8_t {name}_data[] PROGMEM = {{")

    for i in range(0, len(data), 16):
        line = ", ".join(f"0x{b:02X}" for b in data[i:i+16])
        print("  " + line + ",")

    print("};\n")

    print(f"const Bitmap {name} = {{")
    print(f"    {name}_data,")
    print(f"    {width},")
    print(f"    {height}")
    print("};\n")


def process_path(path, threshold, invert):
    if os.path.isdir(path):
        files = sorted(
            f for f in os.listdir(path)
            if f.lower().endswith(VALID_EXT)
        )

        if not files:
            print("No images found.")
            return

        for f in files:
            name, w, h, data = convert_image(os.path.join(path, f), threshold, invert)
            emit_bitmap(name, w, h, data)

    else:
        name, w, h, data = convert_image(path, threshold, invert)
        emit_bitmap(name, w, h, data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Convert monochrome sprite PNGs into Adafruit_GFX Bitmap arrays."
    )
    parser.add_argument("path", help="A single image file or a directory of images.")
    parser.add_argument(
        "--threshold",
        type=int,
        default=128,
        help="Pixels darker than this value are treated as lit. Default: 128.",
    )
    parser.add_argument(
        "--invert",
        action="store_true",
        help="Invert which pixels are treated as lit.",
    )
    args = parser.parse_args()

    if args.threshold < 0 or args.threshold > 255:
        print("Threshold must be between 0 and 255.", file=sys.stderr)
        sys.exit(1)

    process_path(args.path, args.threshold, args.invert)
