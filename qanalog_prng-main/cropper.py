#!/usr/bin/env python3

import sys
import os


def count_and_crop_bits(input_file, output_file, target_bits=100_000_000):
    try:
        # Get file size in bytes
        file_size = os.path.getsize(input_file)
        total_bits = file_size * 8

        if total_bits < target_bits:
            print(
                f"Error: Input file contains only {total_bits:,} bits, but {target_bits:,} bits are required"
            )
            sys.exit(1)

        # Calculate how many bytes we need to read
        needed_bytes = (target_bits + 7) // 8
        remaining_bits = target_bits % 8

        with open(input_file, "rb") as infile, open(output_file, "wb") as outfile:
            # Read the required number of bytes
            data = infile.read(needed_bytes)

            # If we need exact number of bits and it's not byte-aligned
            if remaining_bits > 0:
                # Convert the last byte to a list of bits, keep only what we need
                # and convert back to a byte
                mask = ~((1 << (8 - remaining_bits)) - 1) & 0xFF
                modified_last_byte = bytes([data[-1] & mask])

                # Write all bytes except the last one
                outfile.write(data[:-1])
                # Write the modified last byte
                outfile.write(modified_last_byte)
            else:
                # If we need an exact number of bytes, just write them all
                outfile.write(data)

        print(f"Successfully cropped file to exactly {target_bits:,} bits")
        print(f"Output written to: {output_file}")

    except FileNotFoundError:
        print(f"Error: Could not find input file '{input_file}'")
        sys.exit(1)
    except Exception as e:
        print(f"Error: An unexpected error occurred: {str(e)}")
        sys.exit(1)


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <input_file> [output_file]")
        print("If output_file is not specified, will use 'output.bin'")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2] if len(sys.argv) > 2 else "output.bin"

    count_and_crop_bits(input_file, output_file)


if __name__ == "__main__":
    main()
