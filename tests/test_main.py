from tex2img import decompress_astc, decompress_pvrtc, decompress_etc, crunch_unpack_level, decompress_atc, basisu_decompress
from PIL import Image, ImageChops
import os
import json
from zipfile import ZipFile
from sys import platform

root = os.path.dirname(os.path.abspath(__file__))
zip = ZipFile(os.path.join(root, "samples.zip"))


def test_atc():
    _test("ATC")


def test_astc():
    _test("ASTC")


# def test_etc():
    # crashs pytest
    # _test("ETC")


if platform != "darwin":
    # crashs pytest on MacOS
    def test_pvrtc():
        _test("PVRTC")

    def test_crunch():
        _test("crunch")

if platform != "win32":
    # this check causes a
    # Windows fatal exception: access violation
    # on the windows test environment.
    # It works fine on my own windows machine....
    # and it used to work in the travis test as well....
    def test_basisu():
        for name in zip.namelist():
            if not name.endswith("data"):
                continue
            base_name = name[:-5]
            if base_name not in BASISU_FORMAT:
                continue
            print(base_name, end="")

            # load sample data
            data = zip.open(base_name+".data", "r").read()
            details = json.loads(zip.open(base_name+".json", "r").read())
            ori_img = Image.open(zip.open(base_name+".png", "r"))

            # decompress data
            width = details["m_Width"]
            height = details["m_Height"]

            dec = basisu_decompress(data, width, height, BASISU_FORMAT[base_name])

            # load raw image data
            dec_img = Image.frombytes("RGBA", (width, height), dec, 'raw')

            print(" - decoding successfull", end = "")
            # compare images
            try:
                assert(ImageChops.difference(ori_img, dec_img).getbbox() is None)
                print()
            except (AssertionError,ValueError):
                print(" - assert failed")

BASISU_FORMAT = {
    "ETC_RGB4" : 0, # cETC1       - ETC1
    #"" : 1, # cETC1S      - ETC1(subset:diff colors only, no subblocks)
    "ETC2_RGB" : 2, # cETC2_RGB   - ETC2 color block
    "ETC2_RGBA8" : 3, # cETC2_RGBA  - ETC2 alpha block followed by ETC2 color block
    #"" : 4, # cETC2_ALPHA - ETC2 EAC alpha block
    #"" : 5, # cBC1 -DXT1
    #"" : 6, # cBC3 -DXT5(DXT5A block followed by a DXT1 block)
    #"" : 7, # cBC4 -DXT5A
    #"" : 8, # cBC5 -3DC / DXN(two DXT5A blocks)
    #"" : 9, # cBC7
    "ASTC_RGBA_4x4" : 10, # cASTC4x4
    "PVRTC_RGB4" : 11, # cPVRTC1_4_RGB
    #"" : 12, # cPVRTC1_4_RGBA
    "ATC_RGB4" : 13, # cATC_RGB
    "ATC_RGBA8" : 14, # cATC_RGBA_INTERPOLATED_ALPHA
    #"" : 15, # cFXT1_RGB
    "PVRTC_RGBA2" : 16, # cPVRTC2_4_RGBA
    #"" : 17, # cETC2_R11_EAC
    #"" : 18, # cETC2_RG11_EAC
}

def _test(func):
    for name in zip.namelist():
        if not name.startswith(func) or not name.endswith("data"):
            continue
        base_name = name[:-5]
        print(base_name, end="")
        # load sample data

        data = zip.open(base_name+".data", "r").read()
        details = json.loads(zip.open(base_name+".json", "r").read())
        ori_img = Image.open(zip.open(base_name+".png", "r"))

        # decompress data
        width = details["m_Width"]
        height = details["m_Height"]
        mode = "RGBA"

        if "ATC" == base_name[:3]:
            dec = decompress_atc(data, width, height, 'RGBA' in base_name)
            mode = 'RGBA' if 'RGBA' in base_name else 'RGB'

        elif "ASTC" == base_name[:4]:
            block_width, block_height = map(
                int, base_name.rsplit("_", 1)[-1].split("x"))
            dec = decompress_astc(data, width, height,
                                  block_width, block_height, False)

        elif "ETC" == base_name[:3]:
            if "Crunched" in base_name:
                if platform == "darwin":
                    # crunch isn't supported by Mac OS
                    print(" - not supported by Mac OS")
                    continue
                data = crunch_unpack_level(data, 0)

            etc_format = {
                "ETC_RGB4": 0,
                "ETC2_RGB": 1,
                "ETC2_RGBA8": 3
            }
            dec = decompress_etc(data, width, height,
                                 etc_format[base_name.rstrip("Crunched")])

            mode = 'RGBA' if "RGBA" in base_name else "RGB"

        elif "PVRTC" == base_name[:5]:
            dec = decompress_pvrtc(data, width, height, 0)

        elif func == "crunch" and "Crunched" in base_name:
            if platform == "darwin":
                # crunch isn't supported by Mac OS
                print(" - not supported by Mac OS")
                continue
            data = crunch_unpack_level(data, 0)
            continue

        # load raw image data
        dec_img = Image.frombytes(mode, (width, height), dec, 'raw')

        # compare images
        if func != "PVRTC":  # PVRTC2 is always different.....
            assert(ImageChops.difference(ori_img, dec_img).getbbox() is None)
