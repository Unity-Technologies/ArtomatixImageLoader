#include <gtest/gtest.h>
#include "testCommon.h"
#include "../AIL_internal.h"
#ifdef HAVE_JPEG
#include "../ImageLoaderBase.h"
#include <jpeglib.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../extern/stb_image.h"

std::vector<uint8_t> decodeJPEGFile(const std::string & path)
{
    FILE * file = fopen(path.c_str(), "rb");
    if (file == NULL)
        std::cout << "Could not open file: " + path << std::endl;

    jpeg_decompress_struct cinfo;
    jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    int row_stride;

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, file);

    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
    row_stride = cinfo.output_components * cinfo.output_width;

    std::vector<uint8_t> Vbuffer(row_stride*cinfo.output_height);

    uint8_t * rowBuffer = &Vbuffer[0];
    JSAMPARRAY buffer = (JSAMPARRAY)malloc(sizeof(JSAMPROW));
    buffer[0] = (JSAMPROW)malloc(row_stride * sizeof(JSAMPLE));
    while (cinfo.output_scanline < cinfo.output_height)
    {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(rowBuffer, buffer[0], row_stride);
        rowBuffer += row_stride;
    }

    jpeg_destroy_decompress(&cinfo);
    free(buffer[0]);
    free(buffer);
    fclose(file);

    return Vbuffer;
}

bool testReadJpegFile(const std::string& path)
{
    CallbackData callbacks;
    int16_t error;
    auto data = readFile<uint8_t>(getImagesDir() + path);
    auto img = GetJpegImageForReading(data, &callbacks, &error);

    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        std::cout << AImgGetErrorDetails(img) << std::endl;
        return false;
    }

    int32_t width;
    int32_t height;
    int32_t numChannels;
    int32_t bytesPerChannel;
    int32_t floatOrInt;
    int32_t imgFmt;

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &imgFmt, NULL);

    std::vector<uint8_t> imgData(width*height*numChannels*bytesPerChannel, 78);

    error = AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);

    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        std::cout << AImgGetErrorDetails(img) << std::endl;
        return false;
    }

    auto knownData = decodeJPEGFile(getImagesDir() + path);

    size_t length = knownData.size();
    if (length != imgData.size())
    {
        return false;
    }

    for (size_t i = 0; i < length; ++i)
    {
        if (knownData[i] != imgData[i])
        {
            return false;
        }
    }

    AIDestroySimpleMemoryBufferCallbacks(callbacks.readCallback, callbacks.writeCallback, callbacks.tellCallback, callbacks.seekCallback, callbacks.callbackData);
    AImgClose(img);

    return true;
}

AImgHandle GetJpegImageForReading(const std::vector<uint8_t>& data, CallbackData* callbacks, int16_t * error)
{
    AIGetSimpleMemoryBufferCallbacks(
        &callbacks->readCallback,
        &callbacks->writeCallback,
        &callbacks->tellCallback,
        &callbacks->seekCallback,
        &callbacks->callbackData,
        (void *)data.data(),
        data.size());

    AImgHandle img = nullptr;
    *error = AImgOpen(callbacks->readCallback, callbacks->tellCallback, callbacks->seekCallback, callbacks->callbackData, &img, nullptr);

    return img;
}

void TestWriteJpeg(AImgFormat decodeFormat, AImgFormat writeFormat, AImgFormat expectedWritten = AImgFormat::INVALID_FORMAT)
{
    auto data = readFile<uint8_t>(getImagesDir() + "/jpeg/test.jpeg");

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

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &fmt, NULL);

    if (decodeFormat < 0)
    {
        decodeFormat = (AImgFormat)fmt;
    }
    if (writeFormat < 0)
    {
        writeFormat = decodeFormat;
    }
    if (expectedWritten < 0)
    {
        expectedWritten = writeFormat;
    }

    AIGetFormatDetails(decodeFormat, &numChannels, &bytesPerChannel, &floatOrInt);

    std::vector<uint8_t> imgData(width*height*numChannels * bytesPerChannel, 78);

    AImgDecodeImage(img, &imgData[0], decodeFormat);

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    std::vector<uint8_t> fileData(width * height * numChannels * bytesPerChannel * 5);

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());

    AImgHandle wImg = AImgGetAImg(AImgFileFormat::JPEG_IMAGE_FORMAT);
    AImgWriteImage(wImg, &imgData[0], width, height, decodeFormat, writeFormat, NULL, NULL, 0, writeCallback, tellCallback, seekCallback, callbackData, NULL);
    AImgClose(wImg);

    seekCallback(callbackData, 0);

    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    std::vector<uint8_t> imgData2(width*height*numChannels*bytesPerChannel, 0);
    AImgDecodeImage(img, &imgData2[0], decodeFormat);

    int32_t _, writtenFormat;
    AImgGetInfo(img, &_, &_, &_, &_, &_, &writtenFormat, NULL);
    ASSERT_EQ(writtenFormat, expectedWritten);

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    for (uint32_t i = 0; i < imgData2.size(); i++)
    {
        uint8_t diff = abs(imgData[i] - imgData2[i]);

        double percent = diff / 255.0;
        ASSERT_LT(percent, 0.04);
    }
}

bool TestImageExifOrientation(const std::string& path, uint16_t expectedOrientation)
{
    int16_t error;
    CallbackData callbacks;

    auto data = readFile<uint8_t>(getImagesDir() + path);

    auto img = GetJpegImageForReading(data, &callbacks, &error);

    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        return false;
    }

    auto imgBase = (AImg::AImgBase *)img;

    if (!imgBase->SupportsExif())
        return false;

    auto exifData = imgBase->GetExifData();

    if (!exifData->SupportsExif())
        return false;

    error = AIMG_SUCCESS;
    auto orientation = exifData->GetOrientationField(&error);

    if (error != AIMG_SUCCESS)
    {
        return false;
    }

    AIDestroySimpleMemoryBufferCallbacks(
        callbacks.readCallback,
        callbacks.writeCallback,
        callbacks.tellCallback,
        callbacks.seekCallback,
        callbacks.callbackData);
    AImgClose(img);

    return orientation == expectedOrientation;
}

bool TestImageOrientationConversion(std::string prefix, int orientation)
{
    /* Load the source data to be transformed */
    int sourceWidth;
    int sourceHeight;
    int sourceChannels;
    std::string baseFilePath = getImagesDir() + "/orientation/" + prefix + "_base.png";
    uint8_t* baseData = stbi_load(baseFilePath.c_str(), &sourceWidth, &sourceHeight, &sourceChannels, STBI_rgb_alpha);

    /* Load the expected transformed data */
    int targetWidth;
    int targetHeight;
    int targetChannels;
    std::string expectedFilePath = getImagesDir() + "/orientation/" + prefix + "_" + std::to_string(orientation) + ".png";
    uint8_t* expectedData = stbi_load(expectedFilePath.c_str(), &targetWidth, &targetHeight, &targetChannels, STBI_rgb_alpha);

    /* Transform the source data */
    size_t dataSize = targetWidth * targetHeight * targetChannels;
    uint8_t* actualData = new uint8_t[dataSize];
    AImgConvertOrientation(baseData, actualData, sourceWidth, sourceHeight, AImgFormat::RGBA8U, AImgFormat::RGBA8U, orientation);

    /* Confirm expected and actual transformed data match */
    bool match = true;
    for (size_t i = 0; i < dataSize && match; ++i)
    {
        match = (expectedData[i] == actualData[i]);
    }

    /* Clean-up */
    delete[] baseData;
    delete[] expectedData;
    delete[] actualData;

    return match;
}

TEST(JPEG, TestExifSectionFound)
{
    int16_t error;
    CallbackData callbacks;
    auto path = getImagesDir() + "/jpeg/karl_comment.jpeg";

    auto data = readFile<uint8_t>(path);

    auto img = GetJpegImageForReading(data, &callbacks, &error);

    ASSERT_TRUE(error == AImgErrorCode::AIMG_SUCCESS);

    auto imgBase = (AImg::AImgBase *)img;

    ASSERT_TRUE(imgBase->SupportsExif());

    AIDestroySimpleMemoryBufferCallbacks(
        callbacks.readCallback,
        callbacks.writeCallback,
        callbacks.tellCallback,
        callbacks.seekCallback,
        callbacks.callbackData);
    AImgClose(img);
}

TEST(JPEG, TestDetectJPEG)
{
    ASSERT_TRUE(detectImage("/jpeg/test.jpeg", JPEG_IMAGE_FORMAT));
}

TEST(JPEG, TestReadJPEGAttrs)
{
    ASSERT_TRUE(validateImageHeaders("/jpeg/test.jpeg", 640, 400, 3, 1, AImgFloatOrIntType::FITYPE_INT, AImgFormat::RGB8U));
}

TEST(JPEG, TestReadJPEGAttrsGreyscale)
{
    ASSERT_TRUE(validateImageHeaders("/jpeg/greyscale.jpeg", 2048, 2048, 1, 1, AImgFloatOrIntType::FITYPE_INT, AImgFormat::R8U));
}

TEST(JPEG, TestCompareForceImageFormat1)
{
    ASSERT_TRUE(compareForceImageFormat("/jpeg/test.jpeg"));
}

TEST(JPEG, TestReadJPEGFile1)
{
    ASSERT_TRUE(testReadJpegFile("/jpeg/test.jpeg"));
}

TEST(JPEG, TestReadJPEGFile2)
{
    ASSERT_TRUE(testReadJpegFile("/jpeg/karl.jpeg"));
}

TEST(JPEG, TestReadJPEGFile3)
{
    ASSERT_TRUE(testReadJpegFile("/jpeg/karl_comment.jpeg"));
}

TEST(JPEG, TestReaExifOrientation1)
{
    ASSERT_TRUE(TestImageExifOrientation("/jpeg/ExifOrientation_1.jpg", 1));
}

TEST(JPEG, TestReaExifOrientation2)
{
    ASSERT_TRUE(TestImageExifOrientation("/jpeg/ExifOrientation_2.jpg", 2));
}

TEST(JPEG, TestReaExifOrientation3)
{
    ASSERT_TRUE(TestImageExifOrientation("/jpeg/ExifOrientation_3.jpg", 3));
}

TEST(JPEG, TestReaExifOrientation4)
{
    ASSERT_TRUE(TestImageExifOrientation("/jpeg/ExifOrientation_4.jpg", 4));
}

TEST(JPEG, TestReaExifOrientation5)
{
    ASSERT_TRUE(TestImageExifOrientation("/jpeg/ExifOrientation_5.jpg", 5));
}

TEST(JPEG, TestReaExifOrientation6)
{
    ASSERT_TRUE(TestImageExifOrientation("/jpeg/ExifOrientation_6.jpg", 6));
}

TEST(JPEG, TestReaExifOrientation7)
{
    ASSERT_TRUE(TestImageExifOrientation("/jpeg/ExifOrientation_7.jpg", 7));
}

TEST(JPEG, TestReaExifOrientation8)
{
    ASSERT_TRUE(TestImageExifOrientation("/jpeg/ExifOrientation_8.jpg", 8));
}

TEST(JPEG, TestSquareImageOrientationConversion2)
{
    ASSERT_TRUE(TestImageOrientationConversion("square", 2));
}

TEST(JPEG, TestSquareImageOrientationConversion3)
{
    ASSERT_TRUE(TestImageOrientationConversion("square", 3));
}

TEST(JPEG, TestSquareImageOrientationConversion4)
{
    ASSERT_TRUE(TestImageOrientationConversion("square", 4));
}

TEST(JPEG, TestSquareImageOrientationConversion5)
{
    ASSERT_TRUE(TestImageOrientationConversion("square", 5));
}

TEST(JPEG, TestSquareImageOrientationConversion6)
{
    ASSERT_TRUE(TestImageOrientationConversion("square", 6));
}

TEST(JPEG, TestSquareImageOrientationConversion7)
{
    ASSERT_TRUE(TestImageOrientationConversion("square", 7));
}

TEST(JPEG, TestSquareImageOrientationConversion8)
{
    ASSERT_TRUE(TestImageOrientationConversion("square", 8));
}

TEST(JPEG, TestNonSquareImageOrientationConversion2)
{
    ASSERT_TRUE(TestImageOrientationConversion("nonsquare", 2));
}

TEST(JPEG, TestNonSquareImageOrientationConversion3)
{
    ASSERT_TRUE(TestImageOrientationConversion("nonsquare", 3));
}

TEST(JPEG, TestNonSquareImageOrientationConversion4)
{
    ASSERT_TRUE(TestImageOrientationConversion("nonsquare", 4));
}

TEST(JPEG, TestNonSquareImageOrientationConversion5)
{
    ASSERT_TRUE(TestImageOrientationConversion("nonsquare", 5));
}

TEST(JPEG, TestNonSquareImageOrientationConversion6)
{
    ASSERT_TRUE(TestImageOrientationConversion("nonsquare", 6));
}

TEST(JPEG, TestNonSquareImageOrientationConversion7)
{
    ASSERT_TRUE(TestImageOrientationConversion("nonsquare", 7));
}

TEST(JPEG, TestNonSquareImageOrientationConversion8)
{
    ASSERT_TRUE(TestImageOrientationConversion("nonsquare", 8));
}

TEST(JPEG, TestWriteJPEG)
{
    TestWriteJpeg(AImgFormat::INVALID_FORMAT, AImgFormat::INVALID_FORMAT);
}

TEST(JPEG, TestWriteConvert)
{
    TestWriteJpeg(AImgFormat::RGB16U, AImgFormat::INVALID_FORMAT, AImgFormat::RGB8U);

    TestWriteJpeg(AImgFormat::RGB16U, AImgFormat::RGB8U);
}

TEST(JPEG, TestSupportedFormat)
{
    ASSERT_TRUE(AImgIsFormatSupported(AImgFileFormat::JPEG_IMAGE_FORMAT, AImgFormat::_8BITS | AImgFormat::RGB));
    ASSERT_FALSE(AImgIsFormatSupported(AImgFileFormat::JPEG_IMAGE_FORMAT, AImgFormat::_16BITS));
    ASSERT_FALSE(AImgIsFormatSupported(AImgFileFormat::JPEG_IMAGE_FORMAT, AImgFormat::_32BITS));
}

int main(int argc, char **argv)
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}

#endif