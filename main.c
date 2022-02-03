#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define DCT_SIZE 8
#define HUFF_TREE_DC_LEN 16
#define HUFF_TREE_AC_LEN 256

FILE *fp_jpeg = NULL;

// λͼ�ļ�ͷ
typedef struct __attribute__((packed)) BMP_FILE_HEAD
{
    uint16_t bfType; // �ļ����ͣ�������0x424D��Ҳ�����ַ�BM
    uint32_t bfSize; // �ļ���С������ͷ
    uint16_t bfReserved1; // ������
    uint16_t bfReserved2; // ������
    uint32_t bfOffBits; // �ļ�ͷ��ʵ�ʵ�ͼ�����ݵ�ƫ���ֽ���
}BMP_FILE_HEAD;

// λͼ��Ϣͷ
typedef struct __attribute__((packed)) BMP_INFO_HEAD
{
    uint32_t biSize; // ����ṹ��ĳ��ȣ�Ϊ40�ֽ�
    int32_t biWidth; // ͼ��Ŀ��
    int32_t biHeight; // ͼ��ĳ���
    uint16_t biPlanes; // ������1
    uint16_t biBitCount; // ��ʾ��ɫʱҪ�õ���λ�������õ�ֵΪ 1���ڰ׶�ɫͼ��,4��16 ɫͼ��,8��256 ɫ��,24�����ɫͼ�����µ�.bmp ��ʽ֧�� 32 λɫ�����ﲻ�����ۣ�
    uint32_t biCompression; // ָ��λͼ�Ƿ�ѹ������Ч��ֵΪ BI_RGB��BI_RLE8��BI_RLE4��BI_BITFIELDS������һЩWindows����õĳ�������ʱֻ����BI_RGB��ѹ���������
    uint32_t biSizeImage; // ָ��ʵ�ʵ�λͼ����ռ�õ��ֽ���
    int32_t biXPelsPerMeter; // ָ��Ŀ���豸��ˮƽ�ֱ���
    int32_t biYPelsPerMeter; // ָ��Ŀ���豸�Ĵ�ֱ�ֱ���
    int32_t biClrUsed; // ָ����ͼ��ʵ���õ�����ɫ���������ֵΪ�㣬���õ�����ɫ��Ϊ 2 �� biBitCount �η�
    int32_t biClrImportant; // ָ����ͼ������Ҫ����ɫ���������ֵΪ�㣬����Ϊ���е���ɫ������Ҫ��
}BMP_INFO_HEAD;


const uint8_t default_luma_table[] =
{
    16, 11, 10, 16,  24,  40,  51,  61,
    12, 12, 14, 19,  26,  58,  60,  55,
    14, 13, 16, 24,  40,  57,  69,  56,
    14, 17, 22, 29,  51,  87,  80,  62,
    18, 22, 37, 56,  68, 109, 103,  77,
    24, 35, 55, 64,  81, 104, 113,  92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103,  99,
};

const uint8_t default_chroma_table[] =
{
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
};

const uint8_t zigzag_table[] =
{
    0,   1,  5,  6, 14, 15, 27, 28,
    2,   4,  7, 13, 16, 26, 29, 42,
    3,   8, 12, 17, 25, 30, 41, 43,
    9,  11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63,
};

const uint8_t default_ht_luma_dc_len[16] =
{
    0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0
};

const uint8_t default_ht_luma_dc[12] =
{
    0,1,2,3,4,5,6,7,8,9,10,11
};

const uint8_t default_ht_chroma_dc_len[16] =
{
    0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0
};

const uint8_t default_ht_chroma_dc[12] =
{
    0,1,2,3,4,5,6,7,8,9,10,11
};

const uint8_t default_ht_luma_ac_len[16] =
{
    0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d
};

const uint8_t default_ht_luma_ac[] =
{
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
    0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0,
    0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
    0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5,
    0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2,
    0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA
};

const uint8_t default_ht_chroma_ac_len[16] =
{
    0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77
};

const uint8_t default_ht_chroma_ac[] =
{
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
    0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xA1, 0xB1, 0xC1, 0x09, 0x23, 0x33, 0x52, 0xF0,
    0x15, 0x62, 0x72, 0xD1, 0x0A, 0x16, 0x24, 0x34, 0xE1, 0x25, 0xF1, 0x17, 0x18, 0x19, 0x1A, 0x26,
    0x27, 0x28, 0x29, 0x2A, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
    0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5,
    0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3,
    0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8,
    0xF9, 0xFA
};


typedef struct
{
    uint8_t raw;
    uint8_t code_length;
    uint16_t code_word;
} coefficients;

typedef struct __attribute__((packed))
{
    uint16_t len;
    uint8_t  identifier[5];
    uint16_t version;
    uint8_t  units;
    uint16_t Hdensity;
    uint16_t Vdensity;
    uint8_t  HthumbnailA;
    uint8_t  VthumbnailA;
} JFIF_Header;


int32_t read_bmp_to_rgb888(char *path, uint8_t **data, int32_t *width, int32_t *height);
void rgb888_dump_bmp(char *path, uint8_t *data, int32_t width, int32_t height);
void rgb888_dump_ppm(char *path, uint8_t *data, int32_t width, int32_t height);

int32_t read_block(const uint8_t *rgb, uint8_t *block, int32_t w, int32_t h, int32_t x, int32_t y);
int32_t block_rgb_to_yuv444(const uint8_t *block, uint8_t *y, uint8_t *u, uint8_t *v);
int32_t block_dct(const uint8_t *in, double *out);
int32_t block_quantization(const uint8_t *table, const double *in, int32_t *out);
int32_t block_zigzag(int32_t *in, int32_t *out);
int32_t huffman_encode();
int32_t make_qt_table(int32_t qt, const uint8_t *in, uint8_t *out);
int32_t make_huffman_tree(uint8_t *code_length, uint8_t *raw_val, int32_t out_len, coefficients *out);
int32_t jpeg_write_head();
int32_t jpeg_write_data();
int32_t jpeg_write_end();

int32_t jpeg_write_file(uint8_t *data, int32_t len);
int32_t jpeg_write_bits(uint32_t data, int32_t len);
int32_t jpeg_write_u8(uint8_t data);
int32_t jpeg_write_u16(uint16_t data);
int32_t jpeg_write_u32(uint32_t data);

int32_t jpeg_write_file(uint8_t *data, int32_t len)
{
    return fwrite(data, 1, len, fp_jpeg);
}

int32_t jpeg_write_bits(uint32_t data, int32_t len)
{
    // TODO
    return 0;
}

int32_t jpeg_write_u8(uint8_t data)
{
    uint8_t wd = data;
    return fwrite(&wd, 1, sizeof(uint8_t), fp_jpeg);
}

int32_t jpeg_write_u16(uint16_t data)
{
    uint16_t wd = data;
    return fwrite(&wd, 1, sizeof(uint16_t), fp_jpeg);
}

int32_t jpeg_write_u32(uint32_t data)
{
    uint32_t wd = data;
    return fwrite(&wd, 1, sizeof(uint32_t), fp_jpeg);
}

uint16_t sw16(uint16_t dat)
{
    uint16_t lo = (dat & 0x00ff);
    uint16_t hi = ((dat & 0xff00) >> 8);
    return (uint16_t)((lo << 8) | hi);
}

double ck(int32_t k)
{
    if (k == 0)
    {
        return sqrt(1.0 / DCT_SIZE);
    }
    else
    {
        return sqrt(2.0 / DCT_SIZE);
    }
}

int32_t block_dct(const uint8_t *in, double *out)
{
    int32_t i, j, n, m;
    double sum = 0.0;

    for (m = 0; m < DCT_SIZE; m++)
    {
        for (n = 0; n < DCT_SIZE; n++)
        {
            for (i = 0; i < DCT_SIZE; i++)
            {
                for (j = 0; j < DCT_SIZE; j++)
                {
                    // sum += in[i][j] * cos((2 * j + 1) * n * M_PI / (2 * DCT_SIZE)) * cos((2 * i + 1) * m * M_PI / (2 * DCT_SIZE));
                    sum += in[i * DCT_SIZE + j] * cos((2 * j + 1) * n * M_PI / (2 * DCT_SIZE)) * cos((2 * i + 1) * m * M_PI / (2 * DCT_SIZE));
                }
            }
            // out[m][n] = sum * ck(m) * ck(n);
            out[m * DCT_SIZE + n] =  sum * ck(m) * ck(n);
            sum = 0.0;
        }
    }

    return 0;
}

int32_t read_block(const uint8_t *rgb, uint8_t *block, int32_t w, int32_t h, int32_t x, int32_t y)
{
    // ������8������䣬Ҳ���Ը��Ʊ�Ե����
    int32_t dx, dy;
    int32_t rgb_offset, block_offset;

    for (dy = 0; dy < 8; dy++)
    {
        for (dx = 0; dx < 8; dx++)
        {
            rgb_offset = (((y + dy) * w) + (x + dx)) * 3;
            block_offset = (dy * DCT_SIZE + dx) * 3;
            // printf("b %d \n", block_offset);

            if (x + dx >= w)
            {
                block[block_offset + 0] = block[block_offset - 3 + 0];
                block[block_offset + 1] = block[block_offset - 3 + 1];
                block[block_offset + 2] = block[block_offset - 3 + 2];
                continue;
            }
            if (y + dy >= h)
            {
                block[block_offset + 0] = block[block_offset - 3 * DCT_SIZE + 0];
                block[block_offset + 1] = block[block_offset - 3 * DCT_SIZE + 1];
                block[block_offset + 2] = block[block_offset - 3 * DCT_SIZE + 2];
                continue;
            }
            block[block_offset + 0] = rgb[rgb_offset + 0];
            block[block_offset + 1] = rgb[rgb_offset + 1];
            block[block_offset + 2] = rgb[rgb_offset + 2];
        }
    }

    return 0;
}

// YUV��BT601��BT709��BT2020
// jpeg ʹ�õ�yuv��ʽ https://en.wikipedia.org/wiki/YCbCr
// �ο�JFIF��׼
int32_t block_rgb_to_yuv444(const uint8_t *block, uint8_t *y, uint8_t *u, uint8_t *v)
{
    uint8_t r, g, b;
    int32_t dx, dy;
    int32_t rgb_offset, block_offset;
    float luma, cb, cr;

    for (dy = 0; dy < DCT_SIZE; dy++)
    {
        for (dx = 0; dx < DCT_SIZE; dx++)
        {
            block_offset = (dy * DCT_SIZE + dx);
            r = block[block_offset * 3 + 0];
            g = block[block_offset * 3 + 1];
            b = block[block_offset * 3 + 2];
            luma = 0.299f   * r + 0.587f  * g + 0.114f  * b;
            cb   = -0.1687f * r - 0.3313f * g + 0.5f    * b + 128.0f;
            cr   = 0.5f     * r - 0.4187f * g - 0.0813f * b + 128.0f;
            y[block_offset] = (uint8_t)luma;
            u[block_offset] = (uint8_t)cb;
            v[block_offset] = (uint8_t)cr;
        }
    }
    return 0;
}

int32_t make_qt_table(int32_t qt, const uint8_t *in, uint8_t *out)
{
    int32_t dx, dy;
    int32_t offset;
    float alpha;
    float tmp;

    if (qt < 50)
    {
        alpha = 50.0f / qt;
    }
    else
    {
        alpha = 2.0f - qt / 50.0f;
    }

    for (dy = 0; dy < DCT_SIZE; dy++)
    {
        for (dx = 0; dx < DCT_SIZE; dx++)
        {
            offset = dy * DCT_SIZE + dx;
            tmp = in[offset] * alpha;
            tmp = tmp < 1 ? 1 : tmp;
            tmp = tmp > 255 ? 255 : tmp;
            out[offset] = (uint8_t)tmp;
        }
    }
    return 0;
}

int32_t make_huffman_tree(uint8_t *code_length, uint8_t *raw_val, int32_t out_len, coefficients *out)
{
    int32_t i, j;
    uint32_t code = 0;
    int32_t code_bits = 1;
    int32_t count = 0;

    for(i = 0; i < 16; i++)
    {
        for (j = 0; j < code_length[i]; j++)
        {
            // ����
            // out[count].code_word = code;
            // out[count].code_length = code_bits;
            // out[count].raw = raw_val[count];

            // ���ɲ�����
            out[raw_val[count]].raw = raw_val[count];
            out[raw_val[count]].code_word = code;
            out[raw_val[count]].code_length = code_bits;
            // printf("num %d, raw %#02x, code %#02x, len %d \n", count, raw_val[count], code, code_bits);

            count += 1;
            code += 1;
        }
        code_bits += 1;
        code <<= 1;
    }

    // for (i = 0; i < out_len; i++)
    // {
    // 	printf("num %d, raw %#02x, code %#02x, len %d \n", i, out[i].raw, out[i].code_word, out[i].code_length);
    // }

    return 0;
}

int32_t block_quantization(const uint8_t *table, const double *in, int32_t *out)
{
    int32_t dx, dy;
    int32_t offset;

    for (dy = 0; dy < DCT_SIZE; dy++)
    {
        for (dx = 0; dx < DCT_SIZE; dx++)
        {
            offset = dy * DCT_SIZE + dx;
            out[offset] = round(in[offset] / table[offset]);
        }
    }
    return 0;
}

int32_t block_zigzag(int32_t *in, int32_t *out)
{
    int32_t i;
    for (i = 0; i < DCT_SIZE * DCT_SIZE; i++)
    {
        // out[i] = in[zigzag_table[i]];
        out[zigzag_table[i]] = in[i];
    }
}

int32_t get_val_bit_and_code(int32_t value, uint16_t *bit_num, uint16_t *code)
{
    // ���㷽�����������䣬���������������ֵ�ķ���
    // λ��Ϊ������ľ���ֵ��λ��

    // �Ż����������ò�������
    int32_t abs_val = value;
    if ( value < 0 )
    {
        abs_val = -abs_val;
        value -= 1;
    }
    *bit_num = 1;
    while (abs_val >>= 1)
    {
        *bit_num += 1;
    }
    *code = (uint16_t)(value & ((1 << *bit_num) - 1));

#if 0
    // ԭʼ����
    int32_t tmp_val = value;
    int32_t abs_val = value;
    if (value < 0)
    {
        abs_val = -value;
        tmp_val = ~abs_val;
    }
    *bit_num = 1;
    while (abs_val >>= 1)
    {
        *bit_num += 1;
    }
    *code = (uint16_t)(tmp_val & ((1 << *bit_num) - 1));
#endif
}

int32_t read_bmp_to_rgb888(char *path, uint8_t **data, int32_t *width, int32_t *height)
{
    FILE *img_fp = NULL;
    BMP_FILE_HEAD BFH;
    BMP_INFO_HEAD BIH;
    int32_t file_size = 0;
    int32_t ret = 0;
    int32_t i = 0, j = 0;
    int32_t h = 0, v = 0;
    uint32_t offset = 0;
    uint32_t line_size = 0;
    uint8_t *bmp_data = NULL;
    uint8_t *rgb888_data = NULL;

    img_fp = fopen(path, "rb");
    if (img_fp == NULL)
    {
        printf("can not open %s\n", path);
        *data = NULL;
        *width = 0;
        *height = 0;
        return 1;
    }

    fseek(img_fp, 0, SEEK_END);
    file_size = ftell(img_fp);
    fseek(img_fp, 0, SEEK_SET);

    if (file_size < 54)
    {
        printf("file %s size error\n", path);
        *data = NULL;
        *width = 0;
        *height = 0;
        goto end;
    }

    fseek(img_fp, 0, SEEK_SET);
    fread(&BFH, sizeof(BFH), 1, img_fp); // ��ȡBMP�ļ�ͷ
    fread(&BIH, sizeof(BIH), 1, img_fp); // ��ȡBMP��Ϣͷ��40�ֽڣ�ֱ���ýṹ���

    printf("\nBMP file head\n");
    printf("bfType = %x\n", BFH.bfType);
    printf("bfSize = %d\n", BFH.bfSize);
    printf("bfReserved1 = %d\n", BFH.bfReserved1);
    printf("bfReserved2 = %d\n", BFH.bfReserved2);
    printf("bfOffBits = %d\n", BFH.bfOffBits);

    printf("\nBMP info head\n");
    printf("biSize = %d\n", BIH.biSize);
    printf("biWidth = %d\n", BIH.biWidth);
    printf("biHeight = %d\n", BIH.biHeight);
    printf("biPlanes = %d\n", BIH.biPlanes);
    printf("biBitCount = %d\n", BIH.biBitCount);
    printf("biCompression = %d\n", BIH.biCompression);
    printf("biSizeImage = %d\n", BIH.biSizeImage);
    printf("biXPelsPerMeter = %d\n", BIH.biXPelsPerMeter);
    printf("biYPelsPerMeter = %d\n", BIH.biYPelsPerMeter);
    printf("biClrUsed = %d\n", BIH.biClrUsed);
    printf("biClrImportant = %d\n", BIH.biClrImportant);
    
    // if((BFH.bfType != 0x424D) || (BIH.biClrImportant != 0))
    if((BFH.bfType != 0x4D42))
    {
        printf("\nnot bmp file\n");
        goto end;
    }

    if (BIH.biBitCount != 24 || ((BIH.biClrImportant != 0) && (BIH.biClrImportant != 16777216)))
    {
        printf("\nnot 24 bit bmp file\n");
        goto end;
    }

    bmp_data = (unsigned char *)malloc(BIH.biSizeImage);
    if (bmp_data == NULL)
    {
        printf("malloc bmp_buf error\n");
        *data = NULL;
        *width = 0;
        *height = 0;
        goto end;
    }
    fseek(img_fp, BFH.bfOffBits, SEEK_SET);
    // ret = fread(bmp_data, BIH.biSizeImage, 1, img_fp);
    // if (ret != 1) {
    ret = fread(bmp_data, 1, BIH.biSizeImage, img_fp);
    if (ret != BIH.biSizeImage)
    {
        printf("read bmp file error\n");
        *data = NULL;
        *width = 0;
        *height = 0;
        goto end;
    }
    fclose(img_fp);

    rgb888_data = (unsigned char *)malloc(BIH.biWidth * BIH.biHeight * 3);
    if (rgb888_data == NULL)
    {
        printf("malloc rgb_buf error\n");
        *data = NULL;
        *width = 0;
        *height = 0;
        return 1;
    }

    h = BIH.biWidth;
    v = BIH.biHeight;
    line_size = ((h * 3 + 3) >> 2) << 2; // ��4�ֽڶ���

    for (i = 0; i < v; i++)
    {
        for (j = 0; j < h; j++)
        {
            offset = (v - i - 1) * line_size + j * 3;

            rgb888_data[i * h * 3 + j * 3]     = bmp_data[offset + 2];
            rgb888_data[i * h * 3 + j * 3 + 1] = bmp_data[offset + 1];
            rgb888_data[i * h * 3 + j * 3 + 2] = bmp_data[offset];
        }
    }

    *width = BIH.biWidth;
    *height = BIH.biHeight;
    *data = rgb888_data;
    free(bmp_data);

end:
    fclose(img_fp);
    return 1;
}

void rgb888_dump_bmp(char *path, uint8_t *data, int32_t width, int32_t height)
{
    BMP_FILE_HEAD bfh;
    BMP_INFO_HEAD bih;
    FILE *fp;
    uint8_t line_buf[2048 * 3];
    int32_t line_size;
    int32_t i, j;

    if (width > 2048)
    {
        printf("width larger than 2048\n");
        return;
    }

    fp = fopen(path, "wb");
    if(fp == NULL)
    {
        printf("dump file %s error \n", path);
        return;
    }

    memset(&bfh, 0, sizeof(BMP_FILE_HEAD));
    memset(&bih, 0, sizeof(BMP_INFO_HEAD));
    memset(line_buf, 0, sizeof(line_buf));

    line_size = ((width * 3 + 3) >> 2) << 2;

    bfh.bfType = 0x4D42;
    bfh.bfSize = 54 + line_size * height;
    bfh.bfOffBits = 54;

    bih.biSize = 40;
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = 0;
    bih.biSizeImage = line_size * height;
    bih.biXPelsPerMeter = 4724;
    bih.biYPelsPerMeter = 4724;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    fwrite(&bfh, sizeof(BMP_FILE_HEAD), 1, fp);
    fwrite(&bih, sizeof(BMP_INFO_HEAD), 1, fp);
    for(i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            line_buf[j * 3] = data[(height - i - 1) * width * 3 + j * 3 + 2];
            line_buf[j * 3 + 1] = data[(height - i - 1) * width * 3 + j * 3 + 1];
            line_buf[j * 3 + 2] = data[(height - i - 1) * width * 3 + j * 3];
        }
        fwrite(line_buf, 1, line_size, fp);
    }

    fclose(fp);
}

void rgb888_dump_ppm(char *path, uint8_t *data, int32_t width, int32_t height)
{
    FILE *fp;
    char ppm_head[128];
    int32_t ppm_head_len;

    fp = fopen(path, "wb");
    if(fp == NULL)
    {
        printf("dump file %s error \n", path);
        return;
    }

    memset(ppm_head, 0, sizeof(ppm_head));
    sprintf(ppm_head, "P6 %d %d 255 ", width, height);
    ppm_head_len = strlen(ppm_head);
    fwrite(ppm_head, 1, ppm_head_len, fp);

    fwrite(data, 1, height * width * 3, fp);

    fclose(fp);
}

void print_block_u8(uint8_t *in)
{
    int32_t i, j;
    printf("\n");
    for (i = 0; i < DCT_SIZE; i++)
    {
        for (j = 0; j < DCT_SIZE; j++)
        {
            printf("%3d ", in[i * DCT_SIZE + j]);
        }
        printf("\n");
    }
    printf("\n");
}

int main(int argc, const char **argv)
{
    JFIF_Header jfif;

    int32_t w, h;
    int32_t qt = 90;

    uint8_t *rgb_data;
    uint8_t rgb_block[DCT_SIZE * DCT_SIZE * 3];
    uint8_t y_block[DCT_SIZE * DCT_SIZE];
    uint8_t u_block[DCT_SIZE * DCT_SIZE];
    uint8_t v_block[DCT_SIZE * DCT_SIZE];
    double dct_block[DCT_SIZE * DCT_SIZE];
    int32_t qt_block[DCT_SIZE * DCT_SIZE];
    int32_t zigzag_block[DCT_SIZE * DCT_SIZE];
    uint8_t luma_table[DCT_SIZE * DCT_SIZE];
    uint8_t chroma_table[DCT_SIZE * DCT_SIZE];

    coefficients huff_tree_luma_dc[HUFF_TREE_DC_LEN];
    coefficients huff_tree_chroma_dc[HUFF_TREE_DC_LEN];
    coefficients huff_tree_luma_ac[HUFF_TREE_AC_LEN];
    coefficients huff_tree_chroma_ac[HUFF_TREE_AC_LEN];
    memset(huff_tree_luma_dc,   0, sizeof(huff_tree_luma_dc));
    memset(huff_tree_chroma_dc, 0, sizeof(huff_tree_chroma_dc));
    memset(huff_tree_luma_ac,   0, sizeof(huff_tree_luma_ac));
    memset(huff_tree_chroma_ac, 0, sizeof(huff_tree_chroma_ac));

    // ����huffman������
    make_huffman_tree(default_ht_luma_dc_len,   default_ht_luma_dc,   HUFF_TREE_DC_LEN, huff_tree_luma_dc);
    make_huffman_tree(default_ht_chroma_dc_len, default_ht_chroma_dc, HUFF_TREE_DC_LEN, huff_tree_chroma_dc);
    make_huffman_tree(default_ht_luma_ac_len,   default_ht_luma_ac,   HUFF_TREE_AC_LEN, huff_tree_luma_ac);
    make_huffman_tree(default_ht_chroma_ac_len, default_ht_chroma_ac, HUFF_TREE_AC_LEN, huff_tree_chroma_ac);

    // ��������������
    make_qt_table(qt, default_luma_table, luma_table);
    make_qt_table(qt, default_chroma_table, chroma_table);
    print_block_u8(luma_table);
    print_block_u8(chroma_table);

    read_bmp_to_rgb888("test.bmp", &rgb_data, &w, &h);
    // rgb888_dump_ppm("out.ppm", rgb_data, w, h);
    fp_jpeg = fopen("output.jpeg", "wb");

    // д�ļ�ͷ
    jpeg_write_u16(sw16(0xFFD8));

    // дJFIF
    jfif.len = sw16(sizeof(JFIF_Header));
    strcpy(jfif.identifier, "JFIF");
    jfif.version = sw16(0x0102);
    jfif.units = 0x01; // 0x00-unspecified 0x01-dots_per_inch 0x02-dots_per_cm
    jfif.Hdensity = sw16(96);
    jfif.Vdensity = sw16(96);
    jfif.HthumbnailA = sw16(0);
    jfif.VthumbnailA = sw16(0);
    jpeg_write_u16(sw16(0xFFE0));
    jpeg_write_file(&jfif, sizeof(JFIF_Header));

    // д������
    jpeg_write_u16(sw16(0xFFDB));
    jpeg_write_u16(sw16(2+1+64));
    jpeg_write_u8(0x00); // AAAABBBB(bin), AAAA = ����(0:8λ��1:16λ) BBBB = ������ID�����4����
    jpeg_write_file(luma_table, sizeof(luma_table)); // ����������

    jpeg_write_u16(sw16(0xFFDB));
    jpeg_write_u16(sw16(2+1+64));
    jpeg_write_u8(0x01); // AAAABBBB(bin), AAAA = ����(0:8λ��1:16λ) BBBB = ������ID�����4����
    jpeg_write_file(chroma_table, sizeof(chroma_table)); // ɫ��������

    // TODO дͼ��ʼ���


    // TODO дhuffman��


    // TODO jpeg����д��



    free(rgb_data);
    fclose(fp_jpeg);

    return 0;
}
