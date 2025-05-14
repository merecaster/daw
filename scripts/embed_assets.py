from PIL import Image
import sys
import os

def convert_name(filename):
    return filename.replace('.', '_').replace('-', '_').replace(' ', '_').upper()

directory = "fonts"
output = ["/* Auto-generated */", "#pragma once", "#include \"core.h\"", ""]
for filename in sorted(os.listdir(directory)):
    path = os.path.join(directory, filename)
    image = Image.open(path).convert("L")
    width, height = image.size
    name = convert_name(filename)
    var_name_width = f"FONT_{name}_WIDTH"
    var_name_height = f"FONT_{name}_HEIGHT"
    var_name_bytes = f"FONT_{name}_BYTES"
    output.append(f"const U32 {var_name_width} = {width};")
    output.append(f"const U32 {var_name_height} = {height};")
    output.append(f"const U8 {var_name_bytes}[] = {{")
    for y in range(0, height):
        line = "    "
        for x in range(0, width):
            pixel = image.getpixel((x, y))
            monochrome = 255 if pixel == 255 else 0
            line += f"0x{monochrome:02x}, "
        output.append(line)
    output.append(line)
    output.append("};")
    output.append("")

with open("src/assets.h", "w", encoding='utf-8') as out_file:
    out_file.write("\n".join(output))