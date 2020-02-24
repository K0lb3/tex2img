#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "main.h"

/*
 *************************************************
 * 
 * general basisu decompression
 * 
 ************************************************
*/

static PyObject *basisu_decompress(PyObject *self, PyObject *args)
{
    uint8_t *src;
    size_t src_size;
    int width, height;
    int format;

    if (!PyArg_ParseTuple(args, "y#iii", &src, &src_size, &width, &height, &format))
        return NULL;

    if (width == 0 || height == 0)
        return NULL;
    
    uint8_t* dst = (uint8_t*) malloc(width * height * 4);

    if (format == 11 || format == 12)
        _basisu_pvrtc(src, src_size, dst, width, height);
    else
        _basisu_decompress(src, dst, width, height, format);

    PyObject *ret = Py_BuildValue("y#", dst, width*height*4);
    free(dst);
    return ret;
}

/*
 *************************************************
 * 
 * ASTC
 * 
 ************************************************
*/

static PyObject *decompress_astc(PyObject *self, PyObject *args)
{
    uint8_t *src;
    size_t src_size;
    int block_width, block_height;
    int width, height;
    bool isSRGB;

    if (!PyArg_ParseTuple(args, "y#iiiib", &src, &src_size, &width, &height, &block_width, &block_height, &isSRGB))
    {
        return NULL;
    }

    if (width == 0 || height == 0)
    {
        return NULL;
    }

    uint8_t *dst = (uint8_t *)malloc(width * height * 4);

    _decompress_astc(src, dst, width, height, block_width, block_height, isSRGB);

    PyObject *ret = Py_BuildValue("y#", dst, width * height * 4);
    free(dst);
    return ret;
}

/*
 *************************************************
 * 
 * ATC
 * 
 ************************************************
*/

static PyObject *decompress_atc(PyObject *self, PyObject *args)
{
    uint8_t *src;
    size_t src_size;
    int width, height;
    bool alpha;

    if (!PyArg_ParseTuple(args, "y#iib", &src, &src_size, &width, &height, &alpha))
    {
        return NULL;
    }

    if (width == 0 || height == 0)
    {
        return NULL;
    }
    uint8_t *dst = (uint8_t *)malloc(width * height * (alpha ? 4 : 3));

    _decompress_atc(src, dst, width, height, alpha);

    PyObject *ret = Py_BuildValue("y#", dst, width * height * (alpha ? 4 : 3));
    free(dst);
    return ret;
}

/*
 *************************************************
 * 
 * ETC
 * 
 ************************************************
*/

static PyObject *decompress_etc(PyObject *self, PyObject *args)
{
    // parse input
    uint8 *src;
    size_t src_size;
    int width, height, format;

    if (!PyArg_ParseTuple(args, "y#iii", &src, &src_size, &width, &height, &format))
    {
        return NULL;
    }

    if (width == 0 || height == 0)
    {
        return NULL;
    }

    uint8 *img = (uint8*)malloc(0);
    uint8 *alphaimg = (uint8*)malloc(0);
    // moved to a seperate .h to be able to use the etcpack.cxx directly
    _decompress_etc(src, img, alphaimg, width, height, format);

    PyObject *ret;
    if (format == ETC1_RGB_NO_MIPMAPS || format == ETC2PACKAGE_RGB_NO_MIPMAPS || format == ETC2PACKAGE_sRGB_NO_MIPMAPS)
    {
        ret = Py_BuildValue("y#", img, height * width * 3);
    }
    else if (format == ETC2PACKAGE_RGBA_NO_MIPMAPS_OLD || format == ETC2PACKAGE_RGBA_NO_MIPMAPS || format == ETC2PACKAGE_RGBA1_NO_MIPMAPS)
    {
        // merge img and alphaimg
        uint8 *newimg = (uint8 *)malloc(height * width * 4);
        for (uint32_t pos = 0; pos < height * width; pos++)
        {
            memcpy(newimg + pos * 4, img + pos * 3, 3);
            newimg[pos * 4 + 3] = alphaimg[pos];
        }
        ret = Py_BuildValue("y#", newimg, height * width * 4);
        free(newimg);
    }
    else
        ret = Py_BuildValue("y#", img, height * width * 6);
    free(img);
    free(alphaimg);
    return ret;
}

/*
 *************************************************
 * 
 * PVRTC
 * 
 ************************************************
*/

#include "pvrtc_decoder/PVRTDecompress.h"

static PyObject *decompress_pvrtc(PyObject *self, PyObject *args)
{
    //works
    uint8_t *src;
    unsigned int src_size;
    uint32_t width, height, do2bit_mode;

    if (!PyArg_ParseTuple(args, "y#iib", &src, &src_size, &width, &height, &do2bit_mode))
    {
        return NULL;
    }

    uint32_t dst_size = sizeof(uint8_t) * width * height * 4;
    uint8_t *dst = (uint8_t *)malloc(dst_size);
    pvr::PVRTDecompressPVRTC(src, do2bit_mode, width, height, dst);

    PyObject *ret = Py_BuildValue("y#", dst, dst_size);
    free(dst);

    return ret;
}
/*
wrong decompression - no node documentation
static PyObject * decompress_etc(PyObject * self, PyObject * args) {
    PyObject* ret;
    uint8_t* src;
    unsigned int src_size;
    uint32_t width, height, mode;

    if (!PyArg_ParseTuple(args, "y#iib", &src, &src_size, &width, &height, &mode)) {
        return NULL;
    }

    uint32_t dst_size = sizeof(uint8_t) * width * height * 4;
	uint8_t* dst = (uint8_t*) malloc(dst_size);
	pvr::PVRTDecompressETC(src, width, height, dst, mode);

    ret = Py_BuildValue("y#", dst, dst_size);
    free(dst);
    
    return ret;
}
*/

/*
 *************************************************
 * 
 * Crunch
 * 
 ************************************************
*/
#define _CRT_SECURE_NO_WARNINGS

#include <string.h>
#include <algorithm>

#if defined(__APPLE__)
#include <malloc/malloc.h>
#define malloc_usable_size malloc_size
#else
#if defined(__FreeBSD__)
#include <stdlib.h>
#else
#include <malloc.h>

#ifdef _WIN32
#define malloc_usable_size _msize
#endif
#endif
#endif

#include "crunch/crn_decomp.h"

// texture info
static PyObject *crunch_get_texture_info(PyObject *self, PyObject *args)
{
    unsigned char *buf;
    crnd::uint32 buf_length;
    if (!PyArg_ParseTuple(args, "y#", &buf, &buf_length))
    {
        return NULL;
    }
    crnd::crn_texture_info texture_info;
    texture_info.m_struct_size = sizeof(crnd::crn_texture_info);
    if (!crnd::crnd_get_texture_info(buf, buf_length, &texture_info))
    {
        PyErr_Format(PyExc_ZeroDivisionError, "Failed retrieving CRN texture info!");
        return NULL;
    }
    return Py_BuildValue("{s:I,s:I,s:I,s:I,s:I,s:I,s:I,s:I}",
                         "width", texture_info.m_width,
                         "height", texture_info.m_height,
                         "levels", texture_info.m_levels,
                         "faces", texture_info.m_faces,
                         "bytes_per_block", texture_info.m_bytes_per_block,
                         "userdata0", texture_info.m_userdata0,
                         "userdata1", texture_info.m_userdata1,
                         "format", texture_info.m_format);
}

// level info
static PyObject *crunch_get_level_info(PyObject *self, PyObject *args)
{
    unsigned char *buf;
    crnd::uint32 buf_length;
    crnd::uint32 level_index;
    if (!PyArg_ParseTuple(args, "y#I", &buf, &buf_length, &level_index))
    {
        return NULL;
    }

    crnd::crn_level_info level_info;
    level_info.m_struct_size = sizeof(crnd::crn_level_info);
    if (crnd::crnd_get_level_info(buf, buf_length, level_index, &level_info) == 0)
    {
        PyErr_Format(PyExc_ZeroDivisionError, "Dividing %d by zero!", level_index);
        return NULL;
    }

    return Py_BuildValue("{s:I,s:I,s:I,s:I,s:I,s:I,s:I}",
                         "width", level_info.m_width,
                         "height", level_info.m_height,
                         "faces", level_info.m_faces,
                         "blocks_x", level_info.m_blocks_x,
                         "blocks_y", level_info.m_blocks_y,
                         "bytes_per_block", level_info.m_bytes_per_block,
                         "format", level_info.m_format);
}

static PyObject *crunch_unpack_level(PyObject *self, PyObject *args)
{
    const void *pData;
    crnd::uint32 data_size;
    crnd::uint32 level;
    if (!PyArg_ParseTuple(args, "y#I", &pData, &data_size, &level))
    {
        return NULL;
    }

    if ((!pData) || (data_size < 1))
        return NULL;

    crnd::crn_texture_info tex_info;
    tex_info.m_struct_size = sizeof(crnd::crn_texture_info);
    if (!crnd_get_texture_info(pData, data_size, &tex_info))
    {
        fprintf(stdout, "crnd_get_texture_info() failed");
        return NULL;
    }

    crnd::uint32 tex_num_blocks_x = (tex_info.m_width + 3) >> 2;
    crnd::uint32 tex_num_blocks_y = (tex_info.m_height + 3) >> 2;

    size_t dxt_data_size = tex_info.m_bytes_per_block * tex_num_blocks_x * tex_num_blocks_y * tex_info.m_faces;
    crnd::uint8 *dxt_data = (crnd::uint8 *)malloc(dxt_data_size);

    // Create temp buffer big enough to hold the largest mip level, and all faces if it's a cubemap.
    // dxt_data.resize(tex_info.m_bytes_per_block * tex_num_blocks_x * tex_num_blocks_y * tex_info.m_faces);

    crnd::crnd_unpack_context pContext = crnd::crnd_unpack_begin(pData, data_size);

    if (!pContext)
    {
        fprintf(stdout, "crnd_unpack_begin() failed");
        return NULL;
    }

    void *pFaces[cCRNMaxFaces];

    crnd::uint32 f;
    for (f = tex_info.m_faces; f < cCRNMaxFaces; f++)
        pFaces[f] = NULL;

    crnd::uint32 level_width = std::max(1U, tex_info.m_width >> level);
    crnd::uint32 level_height = std::max(1U, tex_info.m_height >> level);
    crnd::uint32 num_blocks_x = (level_width + 3U) >> 2U;
    crnd::uint32 num_blocks_y = (level_height + 3U) >> 2U;

    crnd::uint32 row_pitch = num_blocks_x * tex_info.m_bytes_per_block;
    crnd::uint32 size_of_face = num_blocks_y * row_pitch;

    for (f = 0; f < tex_info.m_faces; f++)
        pFaces[f] = &dxt_data[f * size_of_face];

    if (!crnd::crnd_unpack_level(pContext, pFaces, dxt_data_size, row_pitch, level))
    {
        crnd::crnd_unpack_end(pContext);
        fprintf(stdout, "crnd_unpack_level() failed");
        return NULL;
    }

    crnd::crnd_unpack_end(pContext);
    return Py_BuildValue("y#", pFaces[0], size_of_face);
}

/*
 *************************************************
 * 
 * Python Connection
 * 
 ************************************************
*/

// Exported methods are collected in a table
static struct PyMethodDef method_table[] = {
    {
        "basisu_decompress",
        (PyCFunction)basisu_decompress,
        METH_VARARGS,
        "Decompresses data compressed via a block compression to RGBA via basisu's unpack_block function.\n\
        Formats:\n\
            0 - cETC1       - ETC1\n\
            1 - cETC1S      - ETC1(subset:diff colors only, no subblocks)\n\
            2 - cETC2_RGB   - ETC2 color block\n\
            3 - cETC2_RGBA  - ETC2 alpha block followed by ETC2 color block\n\
            4 - cETC2_ALPHA - ETC2 EAC alpha block\n\
            5 - cBC1 -DXT1\n\
            6 - cBC3 -DXT5(DXT5A block followed by a DXT1 block)\n\
            7 - cBC4 -DXT5A\n\
            8 - cBC5 -3DC / DXN(two DXT5A blocks)\n\
            9 - cBC7\n\
            10 - cASTC4x4\n\
            11 - cPVRTC1_4_RGB\n\
            12 - cPVRTC1_4_RGBA\n\
            13 - cATC_RGB\n\
            14 - cATC_RGBA_INTERPOLATED_ALPHA\n\
            15 - cFXT1_RGB\n\
            16 - cPVRTC2_4_RGBA\n\
            17 - cETC2_R11_EAC\n\
            18 - cETC2_RG11_EAC\n\
        \n\
        : param src : compressed data\n\
        : type src : bytes\n\
        : param width : image width\n\
        : type width : int\n\
        : param height : image height\n\
        : type height : int\n\
        : param format : see list above\n\
        : type format : int"
        },
        {"decompress_astc",
         (PyCFunction)decompress_astc,
         METH_VARARGS,
         "Decompresses raw astc-compressed data to RGBA\n\
        :param src: compressed data\n\
        :type src: bytes\n\
        :param width: image width\n\
        :type width: int\n\
        :param height: image height\n\
        :type height: int\n\
        :param block_width: width of a block\n\
        :type block_width: int\n\
        :param block_height: height of a blocks\n\
        :type block_height: int\n\
        :param isSRGB: False\n\
        :type isSRGB: bool"},
        {"decompress_atc",
         (PyCFunction)decompress_atc,
         METH_VARARGS,
         "Decompresses raw atc-compressed data to RGBA\n\
        :param src: compressed data\n\
        :type src: bytes\n\
        :param width: image width\n\
        :type width: int\n\
        :param height: image height\n\
        :type height: int\n\
        :param alpha: True if ATC RGBA else False\n\
        :type alpha: bool"},
        {"decompress_pvrtc",
         (PyCFunction)decompress_pvrtc,
         METH_VARARGS,
         "Decompresses raw pvrtc-compressed data to RGBA\n\
        :param src: compressed data\n\
        :type src: bytes\n\
        :param width: image width\n\
        :type width: int\n\
        :param height: image height\n\
        :type height: int\n\
        :param do2bit_mode: 0\n\
        :type do2bit_mode: int"},
        {"decompress_etc",
         (PyCFunction)decompress_etc,
         METH_VARARGS,
         "Decompresses raw etc-compressed data to RGB(A)\n\
        This function has a memory leak, so it might crash\
        if you use it on too many images at once (~500+)\n\
        The basisu_decompress function also supports most ETC,\
        so it is a good alternative.\n\
        \n\
        Formats:\n\
             0 = ETC1_RGB_NO_MIPMAPS\n\
             1 = ETC2PACKAGE_RGB_NO_MIPMAPS\n\
             2 = ETC2PACKAGE_RGBA_NO_MIPMAPS_OLD\n\
             3 = ETC2PACKAGE_RGBA_NO_MIPMAPS\n\
             4 = ETC2PACKAGE_RGBA1_NO_MIPMAPS\n\
             5 = ETC2PACKAGE_R_NO_MIPMAPS\n\
             6 = ETC2PACKAGE_RG_NO_MIPMAPS\n\
             7 = ETC2PACKAGE_R_SIGNED_NO_MIPMAPS\n\
             8 = ETC2PACKAGE_RG_SIGNED_NO_MIPMAPS\n\
             9 = ETC2PACKAGE_sRGB_NO_MIPMAPS\n\
            10 = ETC2PACKAGE_sRGBA_NO_MIPMAPS\n\
            11 = ETC2PACKAGE_sRGBA1_NO_MIPMAPS\n\
        \n\
        :param src: compressed data\n\
        :type src: bytes\n\
        :param width: image width\n\
        :type width: int\n\
        :param height: image height\n\
        :type height: int\n\
        :param format: see list above\n\
        :type format: int"},
        {"crunch_get_texture_info",
         (PyCFunction)crunch_get_texture_info,
         METH_VARARGS,
         "Retrieves texture information from the CRN file.\n\
        :param data: byte data of the file\n\
        :type data: bytes\n\
        :returns: dict"},
        {"crunch_get_level_info",
         (PyCFunction)crunch_get_level_info,
         METH_VARARGS,
         "Retrieves mipmap level specific information from the CRN file.\n\
        :param data: byte data of the file\n\
        :type data: bytes\n\
        :param level: mipmap level\n\
        :type level: int\n\
        :returns: dict"},
        {"crunch_unpack_level",
         (PyCFunction)crunch_unpack_level,
         METH_VARARGS,
         "Transcodes the specified mipmap level to a destination buffer.\n\
        :param data: byte data of the file\n\
        :type data: bytes\n\
        :param level: mipmap level\n\
        :type level: int\n\
        :returns: bytes"},
        {NULL,
         NULL,
         0,
         NULL} // Sentinel value ending the table
    };

// A struct contains the definition of a module
static PyModuleDef tex2img_module = {
    PyModuleDef_HEAD_INIT,
    "tex2img", // Module name
    "a texture decompression C++-extension for Python",
    -1, // Optional size of the module state memory
    method_table,
    NULL, // Optional slot definitions
    NULL, // Optional traversal function
    NULL, // Optional clear function
    NULL  // Optional module deallocation function
};

// The module init function
PyMODINIT_FUNC PyInit_tex2img(void)
{
    return PyModule_Create(&tex2img_module);
}