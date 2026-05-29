import re
import struct
import sys
import os

def extract_and_pack_version(header_path, output_bin_path):
    if not os.path.exists(header_path):
        print(f"Error: Header file not found at {header_path}")
        sys.exit(1)

    with open(header_path, 'r') as f:
        content = f.read()

    # Regex to extract the numbers inside CURRENT_VERSION {major, minor, patch}
    match = re.search(r'#define\s+CURRENT_VERSION\s*\{\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}', content)

    if not match:
        print("Error: Could not find a valid CURRENT_VERSION definition.")
        sys.exit(1)

    major = int(match.group(1))
    minor = int(match.group(2))
    patch = int(match.group(3))

    print(f"Found 8-bit Version: {major}.{minor}.{patch}")

    # '3B' packs 3 separate unsigned 8-bit integers (0-255)
    # Total output file size will be exactly 3 bytes.
    binary_data = struct.pack('3B', major, minor, patch)

    with open(output_bin_path, 'wb') as bin_file:
        bin_file.write(binary_data)

    print(f"Successfully wrote 3 bytes of binary version data to: {output_bin_path}")

if __name__ == "__main__":
    default_header = "main/version.h" 
    default_output = "pub/version"

    header = sys.argv[1] if len(sys.argv) > 1 else default_header
    output = sys.argv[2] if len(sys.argv) > 2 else default_output

    extract_and_pack_version(header, output)
