#include <gtest/gtest.h>

#include <stdio.h>
#include <vector>
#include <string>
#include <png.h>
#include <setjmp.h>
#include <stdint.h>
#include "testCommon.h"
#include "../AIL.h"

std::vector<uint8_t> decodePNGFile(const std::string& path)
{
    FILE * file = fopen(path.c_str(), "rb");
    if (file == NULL)
        std::cout << "Could not open file: " << path << std::endl;

    char header[8];

    fread(header, 1, 8, file);

    if (png_sig_cmp((png_bytep)header, 0, 8))
        std::cout << "Bad png: " << path << std::endl;

    png_struct * png_read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    png_info * png_info_ptr = png_create_info_struct(png_read_ptr);


    if (setjmp(png_jmpbuf(png_read_ptr)))
        std::cout << "Reading info header failed" << std::endl;

    png_init_io(png_read_ptr, file);
    png_set_sig_bytes(png_read_ptr, 8);
    png_read_info(png_read_ptr, png_info_ptr);

    int32_t width = png_get_image_width(png_read_ptr, png_info_ptr);
    int32_t height = png_get_image_height(png_read_ptr, png_info_ptr);
    int32_t bit_depth = png_get_bit_depth(png_read_ptr, png_info_ptr);
    int32_t numChannels = png_get_channels(png_read_ptr, png_info_ptr);

    void ** row_pointers = (void **)malloc(sizeof(png_bytep) * height);

    std::vector<uint8_t> buffer(width * height * numChannels * bit_depth/8);
    for (int y = 0; y < height; y++)
        row_pointers[y] = (void *)((size_t)&buffer[0] + (y*width * (bit_depth/8) * numChannels));

    png_read_image(png_read_ptr, (png_bytepp)row_pointers);

    free(row_pointers);

    png_destroy_read_struct(&png_read_ptr, &png_info_ptr, (png_infopp)NULL);
    png_destroy_info_struct(png_read_ptr, &png_info_ptr);
    fclose(file);
    return buffer;
}

bool validateReadPNGFile(const std::string& path)
{
    auto data = readFile<uint8_t>(getImagesDir() + path);

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);


    int32_t width;
    int32_t height;
    int32_t numChannels;
    int32_t bytesPerChannel;
    int32_t floatOrInt;
    int32_t imgFmt;
    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &imgFmt);


    std::vector<uint8_t> imgData(width*height*numChannels*bytesPerChannel, 78);

    int32_t error = AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);
    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        std::cout << AIGetLastErrorDetails() << std::endl;
    }

    auto knownData = decodePNGFile(getImagesDir() + path);

    bool isEq = true;

    for(int32_t y = 0; y < height; y++)
    {
        for(int32_t x = 0; x < width; x++)
        {
            isEq = isEq && knownData[x + width*y] == imgData[(x + width*y)];

            if (!isEq)
                goto failed;
        }

    }

failed:
    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    return isEq;
}

bool validateWritePNGFile(const std::string& path)
{
    auto data = readFile<uint8_t>(getImagesDir() + path);

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    int32_t width;
    int32_t height;
    int32_t numChannels;
    int32_t bytesPerChannel;
    int32_t floatOrInt;
    int32_t fmt;

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &fmt);

    std::vector<uint8_t> imgData(width*height*numChannels * bytesPerChannel, 78);

    AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    std::vector<char> fileData(width * height * numChannels * bytesPerChannel * 5);
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());
    AImgWriteImage(AImgFileFormat::PNG_IMAGE_FORMAT, &imgData[0], width, height, fmt, writeCallback, tellCallback, seekCallback, callbackData);

    seekCallback(callbackData, 0);

    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    std::vector<uint8_t> imgData2(width*height*numChannels*bytesPerChannel, 0);
    AImgDecodeImage(img, &imgData2[0], AImgFormat::INVALID_FORMAT);
    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    bool passing = true;
    for(uint32_t i = 0; i < imgData2.size(); i++)
    {
            passing = passing && (imgData[i] == imgData2[i]);
            if (!passing)
                goto failed;
    }
failed:
    return passing;
}

TEST(PNG, TestDetect8BitPNG)
{
    ASSERT_TRUE(detectImage("/png/8-bit.png", PNG_IMAGE_FORMAT));
}

TEST(PNG, TestDetect16BitPNG)
{
    ASSERT_TRUE(detectImage("/png/16-bit.png", PNG_IMAGE_FORMAT));
}

TEST(PNG, TestRead8BitPNGAttrs)
{
    ASSERT_TRUE(validateImageHeaders("/png/8-bit.png", 640, 400, 3, 1, AImgFloatOrIntType::FITYPE_INT, AImgFormat::RGB8U));
}

TEST(PNG, TestRead16BitPNGAttrs)
{
    ASSERT_TRUE(validateImageHeaders("/png/16-bit.png", 640, 400, 3, 2, AImgFloatOrIntType::FITYPE_INT, AImgFormat::RGB16U));
}

TEST(PNG, TestRead8BitPNG)
{
    ASSERT_TRUE(validateReadPNGFile("/png/8-bit.png"));
}

TEST(PNG, TestRead16BitPNG)
{
    ASSERT_TRUE(validateReadPNGFile("/png/16-bit.png"));
}

TEST(PNG, TesWrite8BitPNG8)
{
    ASSERT_TRUE(validateWritePNGFile("/png/8-bit.png"));
}

TEST(PNG, TesWrite16BitPNG8)
{
    ASSERT_TRUE(validateWritePNGFile("/png/16-bit.png"));
}

int main(int argc, char **argv)
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}