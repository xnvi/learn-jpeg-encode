#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define DCT_SIZE 8

// 位图文件头
typedef struct __attribute__((packed)) BMP_FILE_HEAD
{
    unsigned short bfType; // 文件类型，必须是0x424D，也就是字符BM
    unsigned int bfSize; // 文件大小，包含头
    unsigned short bfReserved1; // 保留字
    unsigned short bfReserved2; // 保留字
    unsigned int bfOffBits; // 文件头到实际的图像数据的偏移字节数
}BMP_FILE_HEAD;

// 位图信息头
typedef struct __attribute__((packed)) BMP_INFO_HEAD
{
    unsigned int biSize; // 这个结构体的长度，为40字节
    int biWidth; // 图像的宽度
    int biHeight; // 图像的长度
    unsigned short biPlanes; // 必须是1
    unsigned short biBitCount; // 表示颜色时要用到的位数，常用的值为 1（黑白二色图）,4（16 色图）,8（256 色）,24（真彩色图）（新的.bmp 格式支持 32 位色，这里不做讨论）
    unsigned int biCompression; // 指定位图是否压缩，有效的值为 BI_RGB，BI_RLE8，BI_RLE4，BI_BITFIELDS（都是一些Windows定义好的常量，暂时只考虑BI_RGB不压缩的情况）
    unsigned int biSizeImage; // 指定实际的位图数据占用的字节数
    int biXPelsPerMeter; // 指定目标设备的水平分辨率
    int biYPelsPerMeter; // 指定目标设备的垂直分辨率
    unsigned int biClrUsed; // 指定本图象实际用到的颜色数，如果该值为零，则用到的颜色数为 2 的 biBitCount 次方
    unsigned int biClrImportant; // 指定本图象中重要的颜色数，如果该值为零，则认为所有的颜色都是重要的
}BMP_INFO_HEAD;

int read_bmp_to_rgb888(char *path, uint8_t **data, int32_t *width, int32_t *height);
void rgb888_dump_bmp(char *path, uint8_t *data, int32_t width, int32_t height);
void rgb888_dump_ppm(char *path, uint8_t *data, int32_t width, int32_t height);

int read_bmp_to_rgb888(char *path, uint8_t **data, int32_t *width, int32_t *height)
{
    FILE *img_fp = NULL;
    BMP_FILE_HEAD BFH;
    BMP_INFO_HEAD BIH;
    int file_size = 0;
    int ret = 0;
    int32_t i = 0, j = 0;
    int32_t h = 0, v = 0;
    unsigned int offset = 0;
    unsigned int line_size = 0;
    unsigned char *bmp_data = NULL;
    unsigned char *rgb888_data = NULL;

    img_fp = fopen(path, "rb");
    if (img_fp == NULL) {
        printf("can not open %s\n", path);
        *data = NULL;
        *width = 0;
        *height = 0;
        return 1;
    }

    fseek(img_fp, 0, SEEK_END);
    file_size = ftell(img_fp);
    fseek(img_fp, 0, SEEK_SET);

    if (file_size < 54) {
        printf("file %s size error\n", path);
        *data = NULL;
        *width = 0;
        *height = 0;
        goto end;
    }

    fseek(img_fp, 0, SEEK_SET);
    fread(&BFH, sizeof(BFH), 1, img_fp); // 读取BMP文件头
    fread(&BIH, sizeof(BIH), 1, img_fp); // 读取BMP信息头，40字节，直接用结构体读

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
    if (bmp_data == NULL) {
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
    if (ret != BIH.biSizeImage) {
        printf("read bmp file error\n");
        *data = NULL;
        *width = 0;
        *height = 0;
        goto end;
    }
    fclose(img_fp);

    rgb888_data = (unsigned char *)malloc(BIH.biWidth * BIH.biHeight * 3);
    if (rgb888_data == NULL) {
        printf("malloc rgb_buf error\n");
        *data = NULL;
        *width = 0;
        *height = 0;
        return 1;
    }

    h = BIH.biWidth;
    v = BIH.biHeight;
    line_size = ((h * 3 + 3) >> 2) << 2; // 行4字节对齐

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

void dbg_rgb888_dump_bmp(char *path, uint8_t *data, int32_t width, int32_t height)
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

void dbg_rgb888_dump_ppm(char *path, uint8_t *data, int32_t width, int32_t height)
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

int main(int argc, const char **argv)
{
    uint8_t *rgb_data;
    int32_t w, h;

    read_bmp_to_rgb888("test.bmp", &rgb_data, &w, &h);
    dbg_rgb888_dump_ppm("out.ppm", rgb_data, w, h);
    dbg_rgb888_dump_bmp("out.bmp", rgb_data, w, h);

    free(rgb_data);

    return 0;
}
