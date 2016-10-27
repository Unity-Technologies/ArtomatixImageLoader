#include <gtest/gtest.h>
#include "../AIL.h"

#include <cmath>

#include "testCommon.h"

bool compareTiffToPng(const std::string& name, bool convertSrgb = false)
{
    #ifndef HAVE_PNG
        #error "this test needs png loading enabled"
    #endif

    auto pngData = readFile<uint8_t>(getImagesDir() + "/tiff/8_bit_png.png");

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &pngData[0], pngData.size());

    AImgHandle img = NULL;
    int32_t error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    if(error)
        return false;

    int32_t pngWidth;
    int32_t pngHeight;
    int32_t pngNumChannels;
    int32_t pngBytesPerChannel;
    int32_t pngFloatOrInt;
    int32_t pngImgFmt;
    error = AImgGetInfo(img, &pngWidth, &pngHeight, &pngNumChannels, &pngBytesPerChannel, &pngFloatOrInt, &pngImgFmt);
    if(error)
        return false;

    std::vector<uint8_t> pngImgData(pngWidth*pngHeight*4, 78);

    error = AImgDecodeImage(img, &pngImgData[0], AImgFormat::RGBA8U);
    if(error)
        return false;

    AImgClose(img);
    img = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    /////////////////


    auto tiffData = readFile<uint8_t>(getImagesDir() + "/tiff/" + name);

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &tiffData[0], tiffData.size());

    error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    if(error)
        return false;

    int32_t tiffWidth;
    int32_t tiffHeight;
    int32_t tiffNumChannels;
    int32_t tiffBytesPerChannel;
    int32_t tiffFloatOrInt;
    int32_t tiffImgFmt;
    error = AImgGetInfo(img, &tiffWidth, &tiffHeight, &tiffNumChannels, &tiffBytesPerChannel, &tiffFloatOrInt, &tiffImgFmt);
    if(error)
        return false;

    if(tiffWidth != pngWidth || tiffHeight != pngHeight)
        return false;

    std::vector<uint8_t> tiffImgData(tiffWidth*tiffHeight*4, 78);

    error = AImgDecodeImage(img, &tiffImgData[0], AImgFormat::RGBA8U);
    if(error)
        return false;

    AImgClose(img);
    img = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    for(int32_t y = 0; y < pngHeight; y++)
    {
        for(int32_t x = 0; x < pngWidth; x++)
        {
            uint8_t pngR = pngImgData[(x + y*pngWidth)*4 + 0];
            uint8_t pngG = pngImgData[(x + y*pngWidth)*4 + 1];
            uint8_t pngB = pngImgData[(x + y*pngWidth)*4 + 2];
            uint8_t pngA = pngImgData[(x + y*pngWidth)*4 + 3];

            // Test files were written with ps, the int files, and the baseline png that we compare to are written
            // in srgb. The float files, however, are written as linear colour. So, we need to convert one to the other before we compare.
            if(convertSrgb)
            {
                float f;
                f = pngR / 255.0f; pngR = std::pow(f, 2.2f) * 255.0f;
                f = pngG / 255.0f; pngG = std::pow(f, 2.2f) * 255.0f;
                f = pngB / 255.0f; pngB = std::pow(f, 2.2f) * 255.0f;
                f = pngA / 255.0f; pngA = std::pow(f, 2.2f) * 255.0f;
            }

            uint8_t tiffR = tiffImgData[(x + y*pngWidth)*4 + 0];
            uint8_t tiffG = tiffImgData[(x + y*pngWidth)*4 + 1];
            uint8_t tiffB = tiffImgData[(x + y*pngWidth)*4 + 2];
            uint8_t tiffA = tiffImgData[(x + y*pngWidth)*4 + 3];


            int32_t diffR = std::abs(((int32_t)pngR) - ((int32_t)tiffR));
            int32_t diffG = std::abs(((int32_t)pngG) - ((int32_t)tiffG));
            int32_t diffB = std::abs(((int32_t)pngB) - ((int32_t)tiffB));
            int32_t diffA = std::abs(((int32_t)pngA) - ((int32_t)tiffA));


            int32_t thresh = 3;
            if(diffR > thresh || diffG > thresh || diffB > thresh || diffA > thresh)
                return false;
        }

    }

    return true;
}

bool testTiffWrite(int32_t testFormat)
{
    auto pngData = readFile<uint8_t>(getImagesDir() + "/tiff/8_bit_png.png");

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &pngData[0], pngData.size());

    AImgHandle img = NULL;
    int32_t error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);


    int32_t pngWidth;
    int32_t pngHeight;
    int32_t pngNumChannels;
    int32_t pngBytesPerChannel;
    int32_t pngFloatOrInt;
    int32_t pngImgFmt;
    error = AImgGetInfo(img, &pngWidth, &pngHeight, &pngNumChannels, &pngBytesPerChannel, &pngFloatOrInt, &pngImgFmt);
    if(error)
        return false;

    int32_t numChannels, bytesPerChannel, floatOrInt;
    AIGetFormatDetails(testFormat, &numChannels, &bytesPerChannel, &floatOrInt);

    std::vector<uint8_t> pngImgData(pngWidth*pngHeight*numChannels*bytesPerChannel, 78);

    error = AImgDecodeImage(img, &pngImgData[0], testFormat);
    if(error)
        return false;

    AImgClose(img);
    img = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);


    ///////////


    std::vector<uint8_t> fileData(pngWidth*pngHeight*bytesPerChannel*numChannels*10, 79); // 10x the raw data size, should be well enough space
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());

    AImgHandle wImg = AImgGetAImg(AImgFileFormat::TIFF_IMAGE_FORMAT);
    error = AImgWriteImage(wImg, &pngImgData[0], pngWidth, pngHeight, testFormat, writeCallback, tellCallback, seekCallback, callbackData);
    if(error)
        return false;

    seekCallback(callbackData, 0);

    error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    if(error)
        return false;

    int32_t tiffWidth, tiffHeight, tiffNumChannels, tiffBytesPerChannel, tiffFloatOrInt, tiffImgFmt;
    error = AImgGetInfo(img, &tiffWidth, &tiffHeight, &tiffNumChannels, &tiffBytesPerChannel, &tiffFloatOrInt, &tiffImgFmt);
    if(error)
        return false;

    if(tiffImgFmt != testFormat)
        return false;

    if(tiffWidth != pngWidth || tiffHeight != pngHeight)
        return false;

    std::vector<uint8_t> tiffImgData(pngWidth*pngHeight*numChannels*bytesPerChannel, 78);

    error = AImgDecodeImage(img, &tiffImgData[0], AImgFormat::INVALID_FORMAT);
    if(error)
        return false;

    for(size_t i = 0; i < tiffImgData.size(); i++)
    {
        if(tiffImgData[i] != pngImgData[i])
            return false;
    }

    return true;
}

TEST(TIFF, TestDetectTIFF)
{
    ASSERT_TRUE(detectImage("/tiff/8_bit_int.tif", TIFF_IMAGE_FORMAT));
}

TEST(TIFF, TestReadGoodTIFFAttrs)
{
    ASSERT_TRUE(validateImageHeaders("/tiff/8_bit_int.tif", 64, 32, 3, 1, AImgFloatOrIntType::FITYPE_INT, AImgFormat::RGB8U));
}

TEST(TIFF, TestRead8BitInt)
{
    ASSERT_TRUE(compareTiffToPng("8_bit_int.tif"));
}

TEST(TIFF, TestRead16BitInt)
{
    ASSERT_TRUE(compareTiffToPng("16_bit_int.tif"));
}

TEST(TIFF, TestRead16BitFloat)
{
    ASSERT_TRUE(compareTiffToPng("16_bit_float.tif", true));
}

TEST(TIFF, TestRead24BitFloat)
{
    ASSERT_TRUE(compareTiffToPng("24_bit_float.tif", true));
}

TEST(TIFF, TestRead32BitFloat)
{
    ASSERT_TRUE(compareTiffToPng("32_bit_float.tif", true));
}

TEST(TIFF, TestRead8BitIntSeparate)
{
    ASSERT_TRUE(compareTiffToPng("8_bit_int_separate_chans.tif"));
}

TEST(TIFF, TestRead16BitIntSeparate)
{
    ASSERT_TRUE(compareTiffToPng("16_bit_int_separate_chans.tif"));
}

TEST(TIFF, TestRead16BitFloatSeparate)
{
    ASSERT_TRUE(compareTiffToPng("16_bit_float_separate_chans.tif", true));
}

TEST(TIFF, TestRead24BitFloatSeparate)
{
    ASSERT_TRUE(compareTiffToPng("24_bit_float_separate_chans.tif", true));
}

TEST(TIFF, TestRead32BitFloatSeparate)
{
    ASSERT_TRUE(compareTiffToPng("32_bit_float_separate_chans.tif", true));
}

// disabled for now, as hunter version of libtiff has jpg support disabled
//TEST(TIFF, TestReadJpegCompressed)
//{
//    ASSERT_TRUE(compareTiffToPng("jpeg_compressed.tif", true));
//}

TEST(TIFF, TestWrite8U)
{
    ASSERT_TRUE(testTiffWrite(AImgFormat::RGBA8U));
}

TEST(TIFF, TestWrite16U)
{
    testTiffWrite(AImgFormat::RGBA16U);
}

TEST(TIFF, TestWrite16F)
{
    testTiffWrite(AImgFormat::RGBA16F);
}

TEST(TIFF, TestWrite32F)
{
    testTiffWrite(AImgFormat::RGBA32F);
}

int main(int argc, char **argv)
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}
