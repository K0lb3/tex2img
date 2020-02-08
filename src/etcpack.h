#include <stdlib.h>
#include <stdio.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef short int16;

void setupAlphaTable();
void decompressBlockAlphaC(uint8* data, uint8* img, int width, int height, int ix, int iy, int channels);
void decompressBlockETC21BitAlphaC(unsigned int block_part1, unsigned int block_part2, uint8 *img, uint8* alphaimg, int width, int height, int startx, int starty, int channelsRGB);
void decompressBlockETC2c(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty, int channels);
bool readCompressParams(void);
void setupAlphaTableAndValtab();
void decompressBlockAlpha(uint8* data, uint8* img, int width, int height, int ix, int iy);
void decompressBlockAlphaC(uint8* data, uint8* img, int width, int height, int ix, int iy, int channels);
void decompressBlockETC21BitAlpha(unsigned int block_part1, unsigned int block_part2, uint8 *img, uint8* alphaimg, int width, int height, int startx, int starty);
void decompressBlockETC2(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty);
void decompressBlockAlpha16bit(uint8* data, uint8* img, int width, int height, int ix, int iy);

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

//uint8 alphablock[8];

void conv_big_endian_4byte_word(unsigned int *blockadr, uint8* bytes)
{
    unsigned int block;
	block = 0;
	block |= bytes[0];
	block = block << 8;
	block |= bytes[1];
	block = block << 8;
	block |= bytes[2];
	block = block << 8;
	block |= bytes[3];

	blockadr[0] = block;
}

void _decompress_etc(uint8* src, uint8* &img, uint8 *&alphaimg, int& active_width, int& active_height, int format){
    int width,height;
	unsigned int block_part1, block_part2;
	uint8 *newimg, *newalphaimg, *alphaimg2;
	unsigned short w, h;
	int xx, yy;

    w = ((active_width+3)/4)*4;
    h = ((active_height+3)/4)*4;
    width=w;
    height=h;

    /*
    if (format=ETC2PACKAGE_R_NO_MIPMAPS || format=ETC2PACKAGE_RG_NO_MIPMAPS)
		formatSigned=1;

	else if (format=ETC1_RGB_NO_MIPMAPS)
		codec=CODEC_ETC;
    */

    if (format == ETC2PACKAGE_RG_NO_MIPMAPS)
        img = (uint8 *)malloc(3 * width * height * 2);
    else
        img = (uint8 *)malloc(3 * width * height);

    if (!img)
    {
        printf("Error: could not allocate memory\n");
        //exit(0);
    }
    if (format == ETC2PACKAGE_RGBA_NO_MIPMAPS || format == ETC2PACKAGE_R_NO_MIPMAPS || format == ETC2PACKAGE_RG_NO_MIPMAPS || format == ETC2PACKAGE_RGBA1_NO_MIPMAPS || format == ETC2PACKAGE_sRGBA_NO_MIPMAPS || format == ETC2PACKAGE_sRGBA1_NO_MIPMAPS)
    {
        //printf("alpha channel decompression\n");
        alphaimg = (uint8 *)malloc(width * height * 2);
        setupAlphaTableAndValtab();
        if (!alphaimg)
        {
            printf("Error: could not allocate memory for alpha\n");
            exit(0);
        }
    }
    if (format == ETC2PACKAGE_RG_NO_MIPMAPS)
    {
        alphaimg2 = (uint8 *)malloc(width * height * 2);
        if (!alphaimg2)
        {
            printf("Error: could not allocate memory\n");
            exit(0);
        }
    }

    for (int y = 0; y < height / 4; y++)
    {
        for (int x = 0; x < width / 4; x++)
        {
            //decode alpha channel for RGBA
            if (format == ETC2PACKAGE_RGBA_NO_MIPMAPS || format == ETC2PACKAGE_sRGBA_NO_MIPMAPS)
            {
                //memcpy(alphablock, src+offset, 8);
                //fread(alphablock, 1, 8, f);
                //decompressBlockAlpha(alphablock, alphaimg, width, height, 4 * x, 4 * y);
                decompressBlockAlpha(src, alphaimg, width, height, 4 * x, 4 * y);
                src += 8;
            }
            //color channels for most normal modes
            if (format != ETC2PACKAGE_R_NO_MIPMAPS && format != ETC2PACKAGE_RG_NO_MIPMAPS)
            {
                //we have normal ETC2 color channels, decompress these
                conv_big_endian_4byte_word(&block_part1, src);
                src+=4;
                conv_big_endian_4byte_word(&block_part2, src);
                src+=4;
                if (format == ETC2PACKAGE_RGBA1_NO_MIPMAPS || format == ETC2PACKAGE_sRGBA1_NO_MIPMAPS)
                    decompressBlockETC21BitAlpha(block_part1, block_part2, img, alphaimg, width, height, 4 * x, 4 * y);
                else
                    decompressBlockETC2(block_part1, block_part2, img, width, height, 4 * x, 4 * y);
            }
            //one or two 11-bit alpha channels for R or RG.
            if (format == ETC2PACKAGE_R_NO_MIPMAPS || format == ETC2PACKAGE_RG_NO_MIPMAPS)
            {
                //fread(alphablock, 1, 8, f);
                //memcpy(alphablock, src+offset, 8);
                //decompressBlockAlpha16bit(alphablock, alphaimg, width, height, 4 * x, 4 * y);
                decompressBlockAlpha16bit(src, alphaimg, width, height, 4 * x, 4 * y);
                src += 8;
            }
            if (format == ETC2PACKAGE_RG_NO_MIPMAPS)
            {
                //fread(alphablock, 1, 8, f);
                //memcpy(alphablock, src+offset, 8);
                //decompressBlockAlpha16bit(alphablock, alphaimg2, width, height, 4 * x, 4 * y);
                decompressBlockAlpha16bit(src, alphaimg2, width, height, 4 * x, 4 * y);
                src += 8;
            }
        }
    }
    if (format == ETC2PACKAGE_RG_NO_MIPMAPS)
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                img[6 * (y * width + x)] = alphaimg[2 * (y * width + x)];
                img[6 * (y * width + x) + 1] = alphaimg[2 * (y * width + x) + 1];
                img[6 * (y * width + x) + 2] = alphaimg2[2 * (y * width + x)];
                img[6 * (y * width + x) + 3] = alphaimg2[2 * (y * width + x) + 1];
                img[6 * (y * width + x) + 4] = 0;
                img[6 * (y * width + x) + 5] = 0;
            }
        }
    }

    // Ok, and now only write out the active pixels to the .ppm file.
    // (But only if the active pixels differ from the total pixels)

    if (!(height == active_height && width == active_width))
    {
        if (format == ETC2PACKAGE_RG_NO_MIPMAPS)
            newimg = (uint8 *)malloc(3 * active_width * active_height * 2);
        else
            newimg = (uint8 *)malloc(3 * active_width * active_height);

        if (format == ETC2PACKAGE_RGBA_NO_MIPMAPS || format == ETC2PACKAGE_RGBA1_NO_MIPMAPS || format == ETC2PACKAGE_R_NO_MIPMAPS || format == ETC2PACKAGE_sRGBA_NO_MIPMAPS || format == ETC2PACKAGE_sRGBA1_NO_MIPMAPS)
        {
            newalphaimg = (uint8 *)malloc(active_width * active_height * 2);
        }

        if (!newimg)
        {
            printf("Error: could not allocate memory\n");
            exit(0);
        }

        // Convert from total area to active area:

        for (yy = 0; yy < active_height; yy++)
        {
            for (xx = 0; xx < active_width; xx++)
            {
                if (format != ETC2PACKAGE_R_NO_MIPMAPS && format != ETC2PACKAGE_RG_NO_MIPMAPS)
                {
                    newimg[(yy * active_width) * 3 + xx * 3 + 0] = img[(yy * width) * 3 + xx * 3 + 0];
                    newimg[(yy * active_width) * 3 + xx * 3 + 1] = img[(yy * width) * 3 + xx * 3 + 1];
                    newimg[(yy * active_width) * 3 + xx * 3 + 2] = img[(yy * width) * 3 + xx * 3 + 2];
                }
                else if (format == ETC2PACKAGE_RG_NO_MIPMAPS)
                {
                    newimg[(yy * active_width) * 6 + xx * 6 + 0] = img[(yy * width) * 6 + xx * 6 + 0];
                    newimg[(yy * active_width) * 6 + xx * 6 + 1] = img[(yy * width) * 6 + xx * 6 + 1];
                    newimg[(yy * active_width) * 6 + xx * 6 + 2] = img[(yy * width) * 6 + xx * 6 + 2];
                    newimg[(yy * active_width) * 6 + xx * 6 + 3] = img[(yy * width) * 6 + xx * 6 + 3];
                    newimg[(yy * active_width) * 6 + xx * 6 + 4] = img[(yy * width) * 6 + xx * 6 + 4];
                    newimg[(yy * active_width) * 6 + xx * 6 + 5] = img[(yy * width) * 6 + xx * 6 + 5];
                }
                if (format == ETC2PACKAGE_R_NO_MIPMAPS)
                {
                    newalphaimg[((yy * active_width) + xx) * 2] = alphaimg[2 * ((yy * width) + xx)];
                    newalphaimg[((yy * active_width) + xx) * 2 + 1] = alphaimg[2 * ((yy * width) + xx) + 1];
                }
                if (format == ETC2PACKAGE_RGBA_NO_MIPMAPS || format == ETC2PACKAGE_RGBA1_NO_MIPMAPS || format == ETC2PACKAGE_sRGBA_NO_MIPMAPS || format == ETC2PACKAGE_sRGBA1_NO_MIPMAPS)
                {
                    newalphaimg[((yy * active_width) + xx)] = alphaimg[((yy * width) + xx)];
                }
            }
        }

        free(img);
        img = newimg;
        if (format == ETC2PACKAGE_RGBA_NO_MIPMAPS || format == ETC2PACKAGE_RGBA1_NO_MIPMAPS || format == ETC2PACKAGE_R_NO_MIPMAPS || format == ETC2PACKAGE_sRGBA_NO_MIPMAPS || format == ETC2PACKAGE_sRGBA1_NO_MIPMAPS)
        {
            free(alphaimg);
            alphaimg = newalphaimg;
        }
        if (format == ETC2PACKAGE_RG_NO_MIPMAPS)
        {
            free(alphaimg);
            free(alphaimg2);
            alphaimg = NULL;
            alphaimg2 = NULL;
        }
    }
}