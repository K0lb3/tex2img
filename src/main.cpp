#include "main.h"
#include <stdio.h>
#include <stdlib.h>

/*
 *************************************************
 * 
 * basisu block decompression
 * 
 ************************************************
*/

#include "basisu/basisu_gpu_texture.h"
#include "basisu/transcoder/basisu.h"

void _basisu_decompress(uint8_t *src, uint8_t *dst, int &width, int &height, int &format)
{
    //fetch compression block settings
    basisu::texture_format fmt = (basisu::texture_format)format;

    uint32_t block_width = basisu::get_block_width(fmt);
    uint32_t block_height = basisu::get_block_height(fmt);
    uint32_t bytes_per_block = basisu::get_bytes_per_block(fmt);
    uint32_t blocks_x = (width + block_width - 1) / block_width;
    uint32_t blocks_y = (height + block_height - 1) / block_height;

    basisu::color_rgba pPixels[12 * 12];

    uint8_t *block;

    uint32_t bx, by, row_length;
    uint32_t py, y, dst_pixel_pos, block_pixel_pos;

    uint32_t normal_row_length = block_width * 4;
    uint32_t short_row_length = (width - block_width * (blocks_x - 1)) * 4;

    for (by = 0; by < blocks_y; by++)
        for (bx = 0; bx < blocks_x; bx++)
        {
            // decode the block
            basisu::unpack_block(fmt, src, pPixels);
            block = (uint8_t *)pPixels;
            src += bytes_per_block;

            // write the block to the correct position

            // select the required row length
            if (bx < blocks_x - 1)
                row_length = normal_row_length;
            else
                row_length = short_row_length;

            for (y = 0; y < block_height; y++)
            {
                // y position in the decoded image
                py = y + by * block_height;
                // we can ignore it if it's above the height -> out of the original image
                if (py >= height)
                    break;

                // calculate the correct position in the decoded image
                dst_pixel_pos = (py * width + bx * block_width) * 4;
                block_pixel_pos = (y * block_width) * 4;
                memcpy(dst + dst_pixel_pos, block + block_pixel_pos, row_length);
            }
        }
}

#include "basisu/basisu_pvrtc1_4.h"

void _basisu_pvrtc(uint8_t *src, uint32_t src_size, uint8_t *dst, int &width, int &height)
{
    basisu::pvrtc4_image pi(width, height);
    pi.set_to_black();
    memcpy(&pi.get_blocks()[0], src, src_size);
    pi.deswizzle();
    for (uint32_t y = 0; y < height; y++)
        for (uint32_t x = 0; x < width; x++)
        {
            memcpy(dst, pi.get_pixel(x, y).m_comps, 4);
            dst += 4;
        }
}

void _decompress_atc(uint8_t *src, uint8_t *dst, int &width, int &height, bool &alpha)
{
    uint8_t block_width = 4;
    uint8_t block_height = 4;
    uint32_t blocks_x = (width + block_width - 1) / block_width;
    uint32_t blocks_y = (height + block_height - 1) / block_height;
    uint8_t block_size = alpha ? 16 : 8;

    basisu::color_rgba pPixels[12 * 12];
    basisu::color_rgba pix;
    uint32_t y, x, by, bx, px, py, dxt;

    for (by = 0; by < blocks_y; by++)
    {
        for (bx = 0; bx < blocks_x; bx++)
        {
            if (alpha)
            {
                basisu::unpack_atc(src + 8, pPixels);
                basisu::unpack_bc4(src, &pPixels[0].a, sizeof(basisu::color_rgba));
            }
            else
                basisu::unpack_atc(src, pPixels);

            src += block_size;

            for (y = 0; y < block_height; y++)
            {
                py = y + by * block_height;
                if (py >= height)
                    break;
                for (x = 0; x < block_width; x++)
                {
                    px = x + bx * block_width;
                    if (px >= width)
                        break;
                    dxt = (py * width + px) * (alpha ? 4 : 3);
                    pix = pPixels[y * block_width + x];
                    dst[dxt + 0] = pix.r;
                    dst[dxt + 1] = pix.g;
                    dst[dxt + 2] = pix.b;
                    if (alpha)
                        dst[dxt + 3] = pix.a;
                }
            }
        }
    }
}

/*
 *************************************************
 * 
 * ASTC Decompresser
 * 
 ************************************************
*/

#include "basisu/basisu_astc_decomp.h"

static constexpr size_t k_size_in_bytes = 16;
static constexpr size_t k_bytes_per_pixel_unorm8 = 4;
static uint8_t block[256];

/*
    The input is a bytearray that consists of multiple encoded "blocks" which are queued one after another.
    The decoded "blocks" correspond to specific regions of the image.

    e.g.
        image size      -   1024x1024
        block size      -   10x10
        block number    -   103
        ->
        this block corresponds to the section
        (1020, 0, 1030, 10)
        so we have to cut of the last 6 decoded pixels

        -> blocks in x:  floor(width + block_width - 1)/block_width
        -> blocks in y:  floor(height + block_height - 1)/block_height
    
    Since the image is saved linear,
    we have to copy each line of the decoded block to the correct position in the image array.
    
*/

void _decompress_astc(uint8_t *src, uint8_t *dst, int &width, int &height, int &block_width, int &block_height, bool &isSRGB)
{
    int blocks_x = (width + block_width - 1) / block_width;
    int blocks_y = (height + block_height - 1) / block_height;

    int bx, by, row_length;
    int py, y, dst_pixel_pos, block_pixel_pos;

    int normal_row_length = block_width * k_bytes_per_pixel_unorm8;
    int short_row_length = (width - block_width * (blocks_x - 1)) * k_bytes_per_pixel_unorm8;

    for (by = 0; by < blocks_y; by++)
        for (bx = 0; bx < blocks_x; bx++)
        {
            // decode the block
            if (!basisu_astc::astc::decompress(block, src, isSRGB, block_width, block_height))
                printf("failed decoding");
            src += k_size_in_bytes;

            // write the block to the correct position

            // select the required row length
            if (bx < blocks_x - 1)
                row_length = normal_row_length;
            else
                row_length = short_row_length;

            for (y = 0; y < block_height; y++)
            {
                // y position in the decoded image
                py = y + by * block_height;
                // we can ignore it if it's above the height -> out of the original image
                if (py >= height)
                    break;

                // calculate the correct position in the decoded image
                dst_pixel_pos = (py * width + bx * block_width) * k_bytes_per_pixel_unorm8;
                block_pixel_pos = (y * block_width) * k_bytes_per_pixel_unorm8;
                memcpy(dst + dst_pixel_pos, block + block_pixel_pos, row_length);
            }
        }
}

/*
 *************************************************
 * 
 * ETC
 * 
 ************************************************
*/
void decompressBlockAlphaC(uint8 *data, uint8 *img, int width, int height, int ix, int iy, int channels);
void decompressBlockETC21BitAlphaC(unsigned int block_part1, unsigned int block_part2, uint8 *img, uint8 *alphaimg, int width, int height, int startx, int starty, int channelsRGB);
void decompressBlockETC2c(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty, int channels);
void setupAlphaTableAndValtab();
void decompressBlockAlpha(uint8 *data, uint8 *img, int width, int height, int ix, int iy);
void decompressBlockAlphaC(uint8 *data, uint8 *img, int width, int height, int ix, int iy, int channels);
void decompressBlockETC21BitAlpha(unsigned int block_part1, unsigned int block_part2, uint8 *img, uint8 *alphaimg, int width, int height, int startx, int starty);
void decompressBlockETC2(unsigned int block_part1, unsigned int block_part2, uint8 *img, int width, int height, int startx, int starty);
void decompressBlockAlpha16bit(uint8 *data, uint8 *img, int width, int height, int ix, int iy);

void conv_big_endian_4byte_word(unsigned int *blockadr, uint8 *bytes)
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

void _decompress_etc(uint8 *src, uint8 *&img, uint8 *&alphaimg, int &active_width, int &active_height, int &format)
{
    int width, height;
    unsigned int block_part1, block_part2;
    uint8 *newimg, *newalphaimg, *alphaimg2;
    unsigned short w, h;
    int xx, yy;

    w = ((active_width + 3) / 4) * 4;
    h = ((active_height + 3) / 4) * 4;
    width = w;
    height = h;

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
                src += 4;
                conv_big_endian_4byte_word(&block_part2, src);
                src += 4;
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
