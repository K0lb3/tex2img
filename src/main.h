#include <stdint.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef short int16;

// Functions
void _basisu_decompress(uint8_t *src, uint8_t *dst, int &width, int &height, int &format);
void _basisu_pvrtc(uint8_t *src, uint32_t src_size, uint8_t *dst, int &width, int &height);
void _decompress_astc(uint8_t* src, uint8_t* dst, int& width, int& height, int& block_width, int& block_height, bool& isSRGB);
void _decompress_atc(uint8_t* src, uint8_t* dst, int& width, int& height, bool& alpha);
void _decompress_etc(uint8* src, uint8* &img, uint8 *&alphaimg, int& active_width, int& active_height, int& format);

// ETC Formats
enum{
    ETC1_RGB_NO_MIPMAPS,
    ETC2PACKAGE_RGB_NO_MIPMAPS,
    ETC2PACKAGE_RGBA_NO_MIPMAPS_OLD,
    ETC2PACKAGE_RGBA_NO_MIPMAPS,
    ETC2PACKAGE_RGBA1_NO_MIPMAPS,
    ETC2PACKAGE_R_NO_MIPMAPS,
    ETC2PACKAGE_RG_NO_MIPMAPS,
    ETC2PACKAGE_R_SIGNED_NO_MIPMAPS,
    ETC2PACKAGE_RG_SIGNED_NO_MIPMAPS,
    ETC2PACKAGE_sRGB_NO_MIPMAPS,
    ETC2PACKAGE_sRGBA_NO_MIPMAPS,
    ETC2PACKAGE_sRGBA1_NO_MIPMAPS
};

