#include <gtest/gtest.h>
#include "../AIL.h"

#include <cmath>

#include "testCommon.h"

#ifdef HAVE_TIFF

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

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &pngData[0], (int32_t)pngData.size());

    AImgHandle img = NULL;
    int32_t error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    if (error)
        return false;

    int32_t pngWidth;
    int32_t pngHeight;
    int32_t pngNumChannels;
    int32_t pngBytesPerChannel;
    int32_t pngFloatOrInt;
    int32_t pngImgFmt;
    error = AImgGetInfo(img, &pngWidth, &pngHeight, &pngNumChannels, &pngBytesPerChannel, &pngFloatOrInt, &pngImgFmt, NULL);
    if (error)
        return false;

    std::vector<uint8_t> pngImgData(pngWidth*pngHeight * 4, 78);

    error = AImgDecodeImage(img, &pngImgData[0], AImgFormat::RGBA8U);
    if (error)
        return false;

    AImgClose(img);
    img = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    /////////////////

    auto tiffData = readFile<uint8_t>(getImagesDir() + "/tiff/" + name);

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &tiffData[0], (int32_t)tiffData.size());

    error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    if (error)
        return false;

    int32_t tiffWidth;
    int32_t tiffHeight;
    int32_t tiffNumChannels;
    int32_t tiffBytesPerChannel;
    int32_t tiffFloatOrInt;
    int32_t tiffImgFmt;

    error = AImgGetInfo(img, &tiffWidth, &tiffHeight, &tiffNumChannels, &tiffBytesPerChannel, &tiffFloatOrInt, &tiffImgFmt, NULL);
    if (error)
        return false;

    if (tiffWidth != pngWidth || tiffHeight != pngHeight)
        return false;

    std::vector<uint8_t> tiffImgData(tiffWidth*tiffHeight * 4, 78);

    error = AImgDecodeImage(img, &tiffImgData[0], AImgFormat::RGBA8U);
    if (error)
        return false;

    AImgClose(img);
    img = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    for (int32_t y = 0; y < pngHeight; y++)
    {
        for (int32_t x = 0; x < pngWidth; x++)
        {
            uint8_t pngR = pngImgData[(x + y*pngWidth) * 4 + 0];
            uint8_t pngG = pngImgData[(x + y*pngWidth) * 4 + 1];
            uint8_t pngB = pngImgData[(x + y*pngWidth) * 4 + 2];
            uint8_t pngA = pngImgData[(x + y*pngWidth) * 4 + 3];

            // Test files were written with ps, the int files, and the baseline png that we compare to are written
            // in srgb. The float files, however, are written as linear colour. So, we need to convert one to the other before we compare.
            if (convertSrgb)
            {
                float f;
                f = pngR / 255.0f; pngR = (uint8_t)(std::pow(f, 2.2f) * 255.0f);
                f = pngG / 255.0f; pngG = (uint8_t)(std::pow(f, 2.2f) * 255.0f);
                f = pngB / 255.0f; pngB = (uint8_t)(std::pow(f, 2.2f) * 255.0f);
                f = pngA / 255.0f; pngA = (uint8_t)(std::pow(f, 2.2f) * 255.0f);
            }

            uint8_t tiffR = tiffImgData[(x + y*pngWidth) * 4 + 0];
            uint8_t tiffG = tiffImgData[(x + y*pngWidth) * 4 + 1];
            uint8_t tiffB = tiffImgData[(x + y*pngWidth) * 4 + 2];
            uint8_t tiffA = tiffImgData[(x + y*pngWidth) * 4 + 3];

            int32_t diffR = std::abs(((int32_t)pngR) - ((int32_t)tiffR));
            int32_t diffG = std::abs(((int32_t)pngG) - ((int32_t)tiffG));
            int32_t diffB = std::abs(((int32_t)pngB) - ((int32_t)tiffB));
            int32_t diffA = std::abs(((int32_t)pngA) - ((int32_t)tiffA));

            int32_t thresh = 3;
            if (diffR > thresh || diffG > thresh || diffB > thresh || diffA > thresh)
                return false;
        }
    }

    return true;
}

bool testTiffWrite(int32_t testFormat, int32_t outputFormat = -1)
{
    if (outputFormat < 0)
    {
        outputFormat = testFormat;
    }

    auto pngData = readFile<uint8_t>(getImagesDir() + "/tiff/8_bit_png.png");

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &pngData[0], (int32_t)pngData.size());

    AImgHandle img = NULL;
    int32_t error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    int32_t pngWidth;
    int32_t pngHeight;
    int32_t pngNumChannels;
    int32_t pngBytesPerChannel;
    int32_t pngFloatOrInt;
    int32_t pngImgFmt;

    error = AImgGetInfo(img, &pngWidth, &pngHeight, &pngNumChannels, &pngBytesPerChannel, &pngFloatOrInt, &pngImgFmt, NULL);
    if (error)
        return false;

    int32_t numChannels, bytesPerChannel, floatOrInt;
    AIGetFormatDetails(testFormat, &numChannels, &bytesPerChannel, &floatOrInt);

    std::vector<uint8_t> pngImgData(pngWidth*pngHeight*numChannels*bytesPerChannel, 78);

    error = AImgDecodeImage(img, &pngImgData[0], testFormat);
    if (error)
        return false;

    AImgClose(img);
    img = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    ///////////

    int32_t numChannels_out, bytesPerChannel_out, floatOrInt_out;
    AIGetFormatDetails(outputFormat, &numChannels_out, &bytesPerChannel_out, &floatOrInt_out);

    std::vector<uint8_t> fileData(pngWidth*pngHeight*bytesPerChannel_out*numChannels_out * 10, 79); // 10x the raw data size, should be well enough space
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], (int32_t)fileData.size());

    AImgHandle wImg = AImgGetAImg(AImgFileFormat::TIFF_IMAGE_FORMAT);

    error = AImgWriteImage(wImg, &pngImgData[0], pngWidth, pngHeight, testFormat, outputFormat, NULL, NULL, 0, writeCallback, tellCallback, seekCallback, callbackData, NULL);
    if (error)
        return false;

    seekCallback(callbackData, 0);

    error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);
    if (error)
        return false;

    int32_t tiffWidth, tiffHeight, tiffNumChannels, tiffBytesPerChannel, tiffFloatOrInt, tiffImgFmt;

    error = AImgGetInfo(img, &tiffWidth, &tiffHeight, &tiffNumChannels, &tiffBytesPerChannel, &tiffFloatOrInt, &tiffImgFmt, NULL);
    if (error)
        return false;

    if (tiffImgFmt != outputFormat)
        return false;

    if (tiffWidth != pngWidth || tiffHeight != pngHeight)
        return false;

    std::vector<uint8_t> tiffImgData(pngWidth*pngHeight*numChannels*bytesPerChannel, 78);

    error = AImgDecodeImage(img, &tiffImgData[0], testFormat);
    if (error)
        return false;

    for (size_t i = 0; i < tiffImgData.size(); i++)
    {
        if (tiffImgData[i] != pngImgData[i])
            return false;
    }

    return true;
}

bool testColourProfileFromPngToTiff()
{
    ////////////////////////////////////////////////////// Read png (data and colour profile)
    auto pngData = readFile<uint8_t>(getImagesDir() + "/png/ICC.png");

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &pngData[0], (int32_t)pngData.size());

    AImgHandle pngImg = NULL;
    int32_t error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &pngImg, NULL);
    if (error)
        return false;

    int32_t pngWidth;
    int32_t pngHeight;
    int32_t pngNumChannels;
    int32_t pngBytesPerChannel;
    int32_t pngFloatOrInt;
    int32_t pngImgFmt;
    uint32_t pngColourProfileLen;
    error = AImgGetInfo(pngImg, &pngWidth, &pngHeight, &pngNumChannels, &pngBytesPerChannel, &pngFloatOrInt, &pngImgFmt, &pngColourProfileLen);
    if (error)
        return false;

    char profileName[30];
    std::vector<uint8_t> pngColourProfile(pngColourProfileLen);
    AImgGetColourProfile(pngImg, profileName, pngColourProfile.data(), &pngColourProfileLen);

    std::vector<uint8_t> pngImgData(pngWidth*pngHeight * 4, 78);

    error = AImgDecodeImage(pngImg, &pngImgData[0], AImgFormat::RGBA8U);
    if (error)
        return false;

    AImgClose(pngImg);
    pngImg = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    ////////////////////////////////////////////////////// Read tiff (only data)
    auto tiffData = readFile<uint8_t>(getImagesDir() + "/tiff/ICC.tif");

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &tiffData[0], (int32_t)tiffData.size());

    AImgHandle tiffImg1 = NULL;
    error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &tiffImg1, NULL);
    if (error)
        return false;

    int32_t tiffWidth;
    int32_t tiffHeight;
    int32_t tiffNumChannels;
    int32_t tiffBytesPerChannel;
    int32_t tiffFloatOrInt;
    int32_t tiffImgFmt;

    error = AImgGetInfo(tiffImg1, &tiffWidth, &tiffHeight, &tiffNumChannels, &tiffBytesPerChannel, &tiffFloatOrInt, &tiffImgFmt, NULL);
    if (error)
        return false;

    if (tiffWidth != pngWidth || tiffHeight != pngHeight)
        return false;

    std::vector<uint8_t> tiffImgData(tiffWidth*tiffHeight * 4, 78);

    error = AImgDecodeImage(tiffImg1, &tiffImgData[0], AImgFormat::RGBA8U);
    if (error)
        return false;

    AImgClose(tiffImg1);
    tiffImg1 = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    ////////////////////////////////////////////////////// Write tiff (data plus png colour profile)
    std::vector<uint8_t> fileData(pngWidth*pngHeight*pngBytesPerChannel*pngNumChannels * 10, 79); // 10x the raw data size, should be well enough space
    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], (int32_t)fileData.size());

    AImgHandle wImg = AImgGetAImg(AImgFileFormat::TIFF_IMAGE_FORMAT);

    error = AImgWriteImage(wImg, &tiffImgData[0], pngWidth, pngHeight, AImgFormat::RGBA8U, AImgFormat::INVALID_FORMAT, "", pngColourProfile.data(), pngColourProfileLen, writeCallback, tellCallback, seekCallback, callbackData, NULL);
    if (error)
        return false;

    seekCallback(callbackData, 0);

    FILE* f = fopen((getImagesDir() + "/tiff/ICC_png.tif").c_str(), "wb");
    fwrite(&fileData[0], 1, fileData.size(), f);
    fclose(f);

    AImgClose(wImg);
    wImg = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    ////////////////////////////////////////////////////// Read written tiff (data and colour profile)
    tiffData = readFile<uint8_t>(getImagesDir() + "/tiff/ICC_png.tif");

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &tiffData[0], (int32_t)tiffData.size());

    AImgHandle tiffImg2 = NULL;
    error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &tiffImg2, NULL);
    if (error)
        return false;

    uint32_t tifColourProfileLen;
    error = AImgGetInfo(tiffImg2, &tiffWidth, &tiffHeight, &tiffNumChannels, &tiffBytesPerChannel, &tiffFloatOrInt, &tiffImgFmt, &tifColourProfileLen);
    if (error)
        return false;

    char tifProfileName[30];
    std::vector<uint8_t> tifColourProfile(tifColourProfileLen);
    AImgGetColourProfile(tiffImg2, tifProfileName, tifColourProfile.data(), &tifColourProfileLen);

    if (tiffWidth != pngWidth || tiffHeight != pngHeight)
        return false;

    std::vector<uint8_t> tiffImgData2(tiffWidth*tiffHeight * 4, 78);

    error = AImgDecodeImage(tiffImg2, &tiffImgData2[0], AImgFormat::RGBA8U);
    if (error)
        return false;

    AImgClose(tiffImg2);
    tiffImg2 = NULL;
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    ////////////////////////////////////////////////////// Compare png and tiff colour profiles
    if (pngColourProfileLen != tifColourProfileLen)
        return false;
    for (uint32_t i = 0; i < pngColourProfileLen; i++)
    {
        if (pngColourProfile[i] != tifColourProfile[i])
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
    ASSERT_TRUE(testTiffWrite(AImgFormat::RGBA16U));
}

TEST(TIFF, TestWrite16F)
{
    ASSERT_TRUE(testTiffWrite(AImgFormat::RGBA16F));
}

TEST(TIFF, TestWrite32F)
{
    ASSERT_TRUE(testTiffWrite(AImgFormat::RGBA32F));
}


///
/// Read the png image in its current format,
/// save tiff image in the test format
///
TEST(TIFF, TestWrite8Bits)
{
    ASSERT_TRUE(testTiffWrite(AImgFormat::RGB8U, AImgFormat::RGB8U));
}

TEST(TIFF, TestWrite16Bits)
{
    ASSERT_TRUE(testTiffWrite(AImgFormat::RGB8U, AImgFormat::RGB16U));
}

TEST(TIFF, TestWrite32bits)
{
    ASSERT_TRUE(testTiffWrite(AImgFormat::RGB8U, AImgFormat::RGB32F));
}

TEST(TIFF, TestColourProfileFromPngToTiff)
{
    ASSERT_TRUE(testColourProfileFromPngToTiff());
}

TEST(TIFF, TestReadWriteICCProfile)
{
    // Read tif colour profile
    char profileName[30];
    uint8_t * tifColourProfile = NULL;
    uint32_t tifColourProfileLen = 0;
    readWriteIcc("/tiff/ICC.tif", "/tiff/ICC_out.tif", profileName, &tifColourProfile, &tifColourProfileLen);
    ASSERT_EQ(tifColourProfileLen, 560u);
    ASSERT_NE(tifColourProfile, (void*)NULL);
}

TEST(TIFF, TestCompareWithPngICCProfile)
{
    ASSERT_TRUE(compareIccProfiles("/png/ICC.png", "/tiff/ICC.tif"));
}

TEST(TIFF, TestSupportedFormat)
{
    ASSERT_TRUE(AImgIsFormatSupported(AImgFileFormat::TIFF_IMAGE_FORMAT, AImgFormat::_8BITS));
    ASSERT_TRUE(AImgIsFormatSupported(AImgFileFormat::TIFF_IMAGE_FORMAT, AImgFormat::_16BITS));
    ASSERT_TRUE(AImgIsFormatSupported(AImgFileFormat::TIFF_IMAGE_FORMAT, AImgFormat::_32BITS));
}

#endif // HAVE_TIFF

int main(int argc, char **argv)
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}