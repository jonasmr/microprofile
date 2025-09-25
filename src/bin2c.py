import sys
import os
import struct

if len(sys.argv) < 3:
    print(f"Usage: {sys.argv[0]} output.c input1.bin input2.bin ...")
    sys.exit(1)

out_file = sys.argv[1]
input_files = sys.argv[2:]

with open(out_file, "w") as out:
    out.write("#ifdef MICROPROFILE_EMBED_HTML\n")
    out.write("#include <stdint.h>\n\n")

    for infile in input_files:
        # Generate a valid C identifier from filename
        name = os.path.splitext(os.path.basename(infile))[0]
        name = name.replace("-", "_").replace(".", "_")

        data = open(infile, "rb").read()
        orig_len = len(data)
        # Pad to multiple of 4 bytes
        if len(data) % 4 != 0:
            data += b"\0" * (4 - (len(data) % 4))

        words = list(struct.iter_unpack("<I", data))

        out.write(f"const uint32_t {name}[] = {{\n")

        for i, (w,) in enumerate(words):
            # Start new line every 16 entries
            if i % 16 == 0:
                out.write("  ")
            out.write(f"0x{w:08x}, ")
            if (i + 1) % 16 == 0:
                out.write("\n")

        # If last line didn’t end with newline
        if len(words) % 16 != 0:
            out.write("\n")

        out.write("};\n\n")
        out.write(f"const uint32_t {name}_len = {orig_len};\n\n")

    out.write("#endif //MICROPROFILE_EMBED_HTML\n")
