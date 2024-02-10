import os
import shutil
from PIL import Image
from struct import pack

input_folder = "./images"
output_folder = "./icons"


if __name__ == '__main__':
    # check if input folder exists
    if not os.path.isdir(input_folder):
        print("Input folder is empty.")
        quit()

    # check if input folder contains any images
    files = os.listdir(input_folder)
    for file in files:
        if not ".png" in file:
            files.remove(file)
    print(f"Images found: {len(files)}")
    if len(files) == 0:
        quit()

    # clear output folder
    if os.path.isdir(output_folder):
        shutil.rmtree(output_folder)
    os.mkdir(output_folder)

    for file in files:
        image = Image.open(f"{input_folder}/{file}", 'r')

        if image.mode != "RGBA":
            print(f"{file} is not in RGBA format.")
            continue

        width, height = image.size
        data = []
        for y in range(height):
            for x in range(width):
                r, g, b, a = image.getpixel((x, y))
                pxl = int(a == 255) << 5
                pxl = (pxl + int(31 * r / 255)) << 5
                pxl = (pxl + int(31 * g / 255)) << 5
                pxl += int(31 * b / 255)
                data.append(pxl)
        name = file.split('.')[0]

        path = f"{output_folder}/{name}.icon"

        with open(path, "wb") as f:
            f.write(width.to_bytes(4, 'little'))
            f.write(height.to_bytes(4, 'little'))
            for px in data:
                f.write(px.to_bytes(2, 'little'))

        print(f"Converted {file}")
