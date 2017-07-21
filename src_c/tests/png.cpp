#include <gtest/gtest.h>
#include "../AIL.h"

#ifdef HAVE_PNG
#include <stdio.h>
#include <vector>
#include <string>
#include <png.h>
#include <setjmp.h>
#include <stdint.h>
#include <ctime>
#include "testCommon.h"

#include <half.h>

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

    #if AIL_BYTEORDER == AIL_LIL_ENDIAN
    if (bit_depth > 8)
       png_set_swap(png_read_ptr);
    #endif

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
        std::cout << AImgGetErrorDetails(img) << std::endl;
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

bool validateWritePNGFile(const std::string& path, void* encodeOptions, int32_t& err, std::vector<char>& fileData)
{
    err = AImgErrorCode::AIMG_SUCCESS;

    auto data = readFile<uint8_t>(getImagesDir() + path);

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    std::vector<uint8_t> imgData2;
    bool passing = true;
    AImgHandle wImg = NULL;
    std::vector<uint8_t> imgData;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    err = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    if(err != AImgErrorCode::AIMG_SUCCESS) goto failed;

    int32_t width;
    int32_t height;
    int32_t numChannels;
    int32_t bytesPerChannel;
    int32_t floatOrInt;
    int32_t fmt;

    err = AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &fmt);
    if(err != AImgErrorCode::AIMG_SUCCESS) goto failed;

    imgData.resize(width*height*numChannels * bytesPerChannel, 78);

    err = AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);
    if(err != AImgErrorCode::AIMG_SUCCESS) goto failed;

    AImgClose(img);
    img = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
    readCallback = NULL; writeCallback = NULL; tellCallback = NULL, seekCallback = NULL; callbackData = NULL;

    fileData.resize(width * height * numChannels * bytesPerChannel * 5);
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());

    wImg = AImgGetAImg(AImgFileFormat::PNG_IMAGE_FORMAT);
    err = AImgWriteImage(wImg, &imgData[0], width, height, fmt, writeCallback, tellCallback, seekCallback, callbackData, encodeOptions);
    if(err != AImgErrorCode::AIMG_SUCCESS) goto failed;

    fileData.resize(tellCallback(callbackData));

    // reset the callbacks, as the resize above could invalidate the &fileData[0] ptr
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());

    AImgClose(wImg);
    wImg = NULL;

    seekCallback(callbackData, 0);

    err = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    if(err != AImgErrorCode::AIMG_SUCCESS) goto failed;

    imgData2.resize(width*height*numChannels*bytesPerChannel, 0);
    err = AImgDecodeImage(img, &imgData2[0], AImgFormat::INVALID_FORMAT);
    if(err != AImgErrorCode::AIMG_SUCCESS) goto failed;

    AImgClose(img);
    img = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
    readCallback = NULL; writeCallback = NULL; tellCallback = NULL; seekCallback = NULL; callbackData = NULL;

    for(uint32_t i = 0; i < imgData2.size(); i++)
    {
        passing = passing && (imgData[i] == imgData2[i]);
        if (!passing)
            return false;
    }

    return true;

failed:

    if(img != NULL)
        AImgClose(img);
    if(wImg != NULL)
        AImgClose(NULL);
    if(callbackData != NULL)
        AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    return false;
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

TEST(PNG, TestReadIndexedPNGAttrs)
{
    ASSERT_TRUE(validateImageHeaders("/png/indextest_indexed.png", 256, 256, 3, 1, AImgFloatOrIntType::FITYPE_INT, AImgFormat::RGB8U));
}

TEST(PNG, TestReadIndexed)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/png/indextest_indexed.png");

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
    AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);



    data = readFile<uint8_t>(getImagesDir() + "/png/indextest_nonindexed.png");
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &imgFmt);


    std::vector<uint8_t> nonIdxData(width*height*numChannels*bytesPerChannel, 78);

    AImgDecodeImage(img, &nonIdxData[0], AImgFormat::INVALID_FORMAT);

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    ASSERT_EQ(imgData.size(), nonIdxData.size());
    for(size_t i = 0; i < imgData.size(); i++)
        ASSERT_EQ(imgData[i], nonIdxData[i]);
}

TEST(PNG, TestRead8BitPNG)
{
    ASSERT_TRUE(validateReadPNGFile("/png/8-bit.png"));
}

TEST(PNG, TestRead16BitPNG)
{
    ASSERT_TRUE(validateReadPNGFile("/png/16-bit.png"));
}

TEST(PNG, TestWrite8BitPNG8)
{
    int32_t err;
    std::vector<char> fileData;
    ASSERT_TRUE(validateWritePNGFile("/png/8-bit.png", NULL, err, fileData));
    ASSERT_EQ(err, AImgErrorCode::AIMG_SUCCESS);
}

TEST(PNG, TestWrite16BitPNG8)
{
    int32_t err;
    std::vector<char> fileData;
    ASSERT_TRUE(validateWritePNGFile("/png/16-bit.png", NULL, err, fileData));
    ASSERT_EQ(err, AImgErrorCode::AIMG_SUCCESS);
}

TEST(PNG, TestWriteInvalidOptions)
{
    int32_t options = AImgFileFormat::EXR_IMAGE_FORMAT;

    int32_t err;
    std::vector<char> fileData;
    ASSERT_FALSE(validateWritePNGFile("/png/8-bit.png", &options, err, fileData));
    ASSERT_EQ(err, AImgErrorCode::AIMG_INVALID_ENCODE_ARGS);
}

TEST(PNG, TestWriteNoCompressionOptions)
{
    PngEncodingOptions options;
    options.type = AImgFileFormat::PNG_IMAGE_FORMAT;
    options.compressionLevel = 0;
    options.filter = AIL_PNG_NO_FILTERS;

    clock_t uncompressedSaveTime = clock();

    int32_t err;
    std::vector<char> uncompressedFileData;
    ASSERT_TRUE(validateWritePNGFile("/png/8-bit.png", &options, err, uncompressedFileData));
    ASSERT_EQ(err, AImgErrorCode::AIMG_SUCCESS);

    uncompressedSaveTime = clock() - uncompressedSaveTime;

    clock_t compressedSaveTime = clock();

    std::vector<char> defaultCompressionFileData;
    ASSERT_TRUE(validateWritePNGFile("/png/8-bit.png", NULL, err, defaultCompressionFileData));
    ASSERT_EQ(err, AImgErrorCode::AIMG_SUCCESS);

    compressedSaveTime = clock() - compressedSaveTime;

    // assert that the compressed save is smaller, but took longer to perform
    ASSERT_GT(uncompressedFileData.size(), defaultCompressionFileData.size());
    ASSERT_GT(compressedSaveTime, uncompressedSaveTime);
}

TEST(PNG, TestCompareForceImageFormat1)
{
    ASSERT_TRUE(compareForceImageFormat("/png/8-bit.png"));
}

TEST(PNG, TestCompareForceImageFormat2)
{
    ASSERT_TRUE(compareForceImageFormat("/png/16-bit.png"));
}

TEST(PNG, TestCompareForceImageFormat3)
{
    ASSERT_TRUE(compareForceImageFormat("/png/alpha.png"));
}

TEST(PNG, TestForceImageFormat)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/png/8-bit.png");

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


    std::vector<float> imgData(width*height*3, 78);

    int32_t error = AImgDecodeImage(img, &imgData[0], AImgFormat::RGB32F);
    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        std::cout << AImgGetErrorDetails(img) << std::endl;
    }

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    auto knownData = decodePNGFile(getImagesDir() + "/png/8-bit.png");


    for(uint32_t i = 0; i < imgData.size(); i++)
        ASSERT_EQ(((float)knownData[i]) / 255.0f, imgData[i]);

}

TEST(Png, TestForceAwayAlpha)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/png/alpha.png");

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


    std::vector<uint8_t> imgData(width*height*3, 78);

    int32_t error = AImgDecodeImage(img, &imgData[0], AImgFormat::RGB8U); // image is actually RGBA8U, so force off the alpha channel
    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        std::cout << AImgGetErrorDetails(img) << std::endl;
    }

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    auto knownData = decodePNGFile(getImagesDir() + "/png/alpha.png");

    for(int32_t y = 0; y < height; y++)
    {
        for(int32_t x = 0; x < width; x++)
        {
            uint8_t realR = knownData[(x + y*width)*4 + 0]; 
            uint8_t realG = knownData[(x + y*width)*4 + 1]; 
            uint8_t realB = knownData[(x + y*width)*4 + 2]; 
            uint8_t realA = knownData[(x + y*width)*4 + 3]; 
            
            uint8_t readR = imgData[(x + y*width)*3 + 0]; 
            uint8_t readG = imgData[(x + y*width)*3 + 1]; 
            uint8_t readB = imgData[(x + y*width)*3 + 2]; 

            if(realA != 0)
            {
                ASSERT_EQ(realR, readR);
                ASSERT_EQ(realG, readG);
                ASSERT_EQ(realB, readB);
            }
        }
    }
}

TEST(Png, TestWriteFrom32Bit)
{
    int32_t width = 64;
    int32_t height = 32;
    int32_t numChannels = 4;

    std::vector<float> fData(width*height*numChannels, 0.0f);

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            fData[((x + y*width)*numChannels)+0] = 1.0f;
            fData[((x + y*width)*numChannels)+1] = 1.0f;
            fData[((x + y*width)*numChannels)+2] = 0.0f;
            fData[((x + y*width)*numChannels)+3] = 1.0f;

        }
    }

    std::vector<uint8_t> fileData(width*height*numChannels*50); // should be big enough

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());

    AImgHandle wImg = AImgGetAImg(AImgFileFormat::PNG_IMAGE_FORMAT);
    int32_t err = AImgWriteImage(wImg, &fData[0], width, height, AImgFormat::RGBA32F, writeCallback, tellCallback, seekCallback, callbackData, NULL);
    AImgClose(wImg);

    seekCallback(callbackData, 0);


    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    std::vector<uint16_t> data16(width*height*numChannels, 0);

    AImgDecodeImage(img, &data16[0], AImgFormat::INVALID_FORMAT);

    for(uint32_t i = 0; i < data16.size(); i++)
    {
        ASSERT_EQ(data16[i], ((uint32_t)fData[i])* std::numeric_limits<uint16_t>::max());
    }

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
}

TEST(Png, TestWriteFrom16BitFloat)
{
    int32_t width = 64;
    int32_t height = 32;
    int32_t numChannels = 4;

    std::vector<half> fData(width*height*numChannels, 0.0f);

    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            fData[((x + y*width)*numChannels)+0] = 1.0f;
            fData[((x + y*width)*numChannels)+1] = 1.0f;
            fData[((x + y*width)*numChannels)+2] = 0.0f;
            fData[((x + y*width)*numChannels)+3] = 1.0f;

        }
    }

    std::vector<uint8_t> fileData(width*height*numChannels*50); // should be big enough

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());

    AImgHandle wImg = AImgGetAImg(AImgFileFormat::PNG_IMAGE_FORMAT);
    int32_t err = AImgWriteImage(wImg, &fData[0], width, height, AImgFormat::RGBA16F, writeCallback, tellCallback, seekCallback, callbackData, NULL);
    AImgClose(wImg);

    seekCallback(callbackData, 0);


    AImgHandle img = NULL;
    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    std::vector<uint16_t> data16(width*height*numChannels, 0);

    AImgDecodeImage(img, &data16[0], AImgFormat::INVALID_FORMAT);

    for(uint32_t i = 0; i < data16.size(); i++)
    {
        ASSERT_EQ(data16[i], ((uint32_t)fData[i])* std::numeric_limits<uint16_t>::max());
    }

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
}

#endif // HAVE_PNG

int main(int argc, char **argv)
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}
