# tex2img

[![PyPI supported Python versions](https://img.shields.io/pypi/pyversions/UnityPy.svg)](https://pypi.python.org/pypi/tex2img)
[![Win/Mac/Linux](https://img.shields.io/badge/platform-windows%20%7C%20macos%20%7C%20linux-informational)]()
[![MIT](https://img.shields.io/pypi/l/UnityPy.svg)](https://github.com/K0lb3/tex2img/blob/master/LICENSE)
[![Build Status (Travis)](https://travis-ci.com/K0lb3/tex2img.svg?token=EyD3NSmbq8Jfyqgp2mmq&branch=master)](https://travis-ci.com/K0lb3/tex2img)
[![Build Status (Github)](https://github.com/K0lb3/tex2img/workflows/Test%20and%20Publish/badge.svg?branch=master)](https://github.com/K0lb3/tex2img/actions?query=workflow%3A%22Test+and+Publish%22)

A texture decompression C++-extension for Python.

1. [Installation](https://github.com/K0lb3/tex2img#installation)
2. [Notes](https://github.com/K0lb3/tex2img#notes)
3. [Functions](https://github.com/K0lb3/tex2img#functions)
4. [Sources](https://github.com/K0lb3/tex2img#sources)

## Installation

```cmd
pip install tex2img
```

or download/clone the git and use

```cmd
python setup.py install
```

## Notes

* the PowerVR pvrtc-decompression causes a segfault error on Mac OS, so decompress_pvrtc will use basisu on Mac OS instead of the original

## Functions

All functions accept only args and aren't able to handle kwards atm.

### basisu_decompress

*Decompresses data to RGBA via basisu's unpack_block function.*

Args: (compressed data: bytes, width: int, height: int, format: int)

Returns: bytes

Formats:

| Format |             Mode             |                     Note                      |
|--------|------------------------------|-----------------------------------------------|
|      0 | cETC1                        | ETC1                                          |
|      1 | cETC1S                       | ETC1(subset:diff colors only, no subblocks)   |
|      2 | cETC2_RGB                    | ETC2 color block                              |
|      3 | cETC2_RGBA                   | ETC2 alpha block followed by ETC2 color block |
|      4 | cETC2_ALPHA                  | ETC2 EAC alpha block                          |
|      5 | cBC1                         | DXT1                                          |
|      6 | cBC3                         | DXT5(DXT5A block followed by a DXT1 block)    |
|      7 | cBC4                         | DXT5A                                         |
|      8 | cBC5                         | 3DC / DXN(two DXT5A blocks)                   |
|      9 | cBC7                         |                                               |
|     10 | cASTC4x4                     |                                               |
|     11 | cPVRTC1_4_RGB                |                                               |
|     12 | cPVRTC1_4_RGBA               |                                               |
|     13 | cATC_RGB                     |                                               |
|     14 | cATC_RGBA_INTERPOLATED_ALPHA |                                               |
|     15 | cFXT1_RGB                    |                                               |
|     16 | cPVRTC2_4_RGBA               |                                               |
|     17 | cETC2_R11_EAC                |                                               |
|     18 | cETC2_RG11_EAC               |                                               |

### decompress_astc

*Decompresses raw astc-compressed data to RGBA.*

Args: (compressed data: bytes, image width: int, image height: int, block_width: int, block_height: int, isSRGB: False)

Returns: bytes

### decompress_atc

*Decompresses raw atc-compressed data to RGB(A).*

Args: (compressed data: bytes, width: int, height: int, alpha: bool)

Returns: bytes

alpha = False for ATC_RGB
alpha = True for ATC_RGBA

### decompress_pvrtc

*Decompresses raw pvrtc-compressed data to RGBA.*

Args: (compressed data: bytes, width: int, height: int, do2bit_mode: 0)

Returns: bytes

### decompress_etc

*Decompresses raw etc-compressed data to RGB(A).*

Args: (compressed data: bytes, width: int, height: int, format: int)

This function has a memory leak, so it might crash if you use it on too many images at once (~500+).
The basisu_decompress function also supports the most common ETC formats, so it is a good alternative.

Formats:

| Format |               Mode               |
|--------|----------------------------------|
|      0 | ETC1_RGB_NO_MIPMAPS              |
|      1 | ETC2PACKAGE_RGB_NO_MIPMAPS       |
|      2 | ETC2PACKAGE_RGBA_NO_MIPMAPS_OLD  |
|      3 | ETC2PACKAGE_RGBA_NO_MIPMAPS      |
|      4 | ETC2PACKAGE_RGBA1_NO_MIPMAPS     |
|      5 | ETC2PACKAGE_R_NO_MIPMAPS         |
|      6 | ETC2PACKAGE_RG_NO_MIPMAPS        |
|      7 | ETC2PACKAGE_R_SIGNED_NO_MIPMAPS  |
|      8 | ETC2PACKAGE_RG_SIGNED_NO_MIPMAPS |
|      9 | ETC2PACKAGE_sRGB_NO_MIPMAPS      |
|     10 | ETC2PACKAGE_sRGBA_NO_MIPMAPS     |
|     11 | ETC2PACKAGE_sRGBA1_NO_MIPMAPS    |

### crunch_get_texture_info

*Retrieves texture information from the CRN file.*

Args: (data: bytes)

Returns: dict

### crunch_get_level_info

*Retrieves mipmap level specific information from the CRN file.*

Args: (data: bytes, mipmap_leve: int)

Returns: dict

### crunch_unpack_level

*Transcodes the specified mipmap level to a destination buffer.*

Args: (data: bytes, mipmap_level: int)

Returns: bytes

## Sources

### ATC & ASTC

The complete [BinomialLLC/basis_universal](https://github.com/BinomialLLC/basis_universal/) is used and supported. It's default for the ATC and ASTC decompression.

### ETC

The whole source of [Ericsson/ETCPACK](https://github.com/Ericsson/ETCPACK) is used for the ETC decompression.

### PVRTC

[PVRTDecompress.cpp](https://github.com/powervr-graphics/Native_SDK/blob/master/framework/PVRCore/texture/PVRTDecompress.cpp) and [PVRTDecompress.h](https://github.com/powervr-graphics/Native_SDK/blob/master/framework/PVRCore/texture/PVRTDecompress.h) of
 [powervr-graphics/Native_SDK](https://github.com/powervr-graphics/Native_SDK/tree/master/framework/PVRCore/texture) are used for the PVRTC decompression.

### crunch

A mixed version of [BinomialLLC/crunch](https://github.com/BinomialLLC/crunch) and
[Unity-Technologies/crunch](https://github.com/Unity-Technologies/crunch) is used.

The Unity fork doesn't yield correct results for the original modes, so the BiomialLLC version is used for all modes besides ETC, which was created byUnity.
