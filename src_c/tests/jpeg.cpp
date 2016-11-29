#include <gtest/gtest.h>
#include "testCommon.h"
#include "../AIL.h"
#ifdef HAVE_JPEG
#include <jpeglib.h>
#include <math.h>

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

    uint8_t * rowBuffer =  &Vbuffer[0];
    JSAMPARRAY buffer = (JSAMPARRAY)malloc(sizeof(JSAMPROW));
    buffer[0] = (JSAMPROW) malloc(row_stride * sizeof(JSAMPLE));
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
    auto data = readFile<uint8_t>(getImagesDir() + path);

    ReadCallback readCallback = NULL;
    WriteCallback writeCallback = NULL;
    TellCallback tellCallback = NULL;
    SeekCallback seekCallback = NULL;
    void* callbackData = NULL;

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &data[0], data.size());

    AImgHandle img = NULL;
    int32_t error = AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

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

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &imgFmt);

    std::vector<uint8_t> imgData(width*height*numChannels*bytesPerChannel, 78);

    error = AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);

    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        std::cout << AImgGetErrorDetails(img) << std::endl;
        return false;
    }

    auto knownData = decodeJPEGFile(getImagesDir() + path);

    for(int32_t y = 0; y < height; y++)
    {
        for(int32_t x = 0; x < width; x++)
        {
            if(knownData[x + width*y] != imgData[(x + width*y)])
                return false;
        }
    }

    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
    AImgClose(img);

    return true;
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


TEST(JPEG, TestWriteJPEG)
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

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &fmt);

    std::vector<uint8_t> imgData(width*height*numChannels * bytesPerChannel, 78);

    AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);

    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    std::vector<char> fileData(width * height * numChannels * bytesPerChannel * 5);

    AIGetSimpleMemoryBufferCallbacks(&readCallback, &writeCallback, &tellCallback, &seekCallback, &callbackData, &fileData[0], fileData.size());

    AImgHandle wImg = AImgGetAImg(AImgFileFormat::JPEG_IMAGE_FORMAT);
    AImgWriteImage(wImg, &imgData[0], width, height, fmt, writeCallback, tellCallback, seekCallback, callbackData);
    AImgClose(wImg);

    seekCallback(callbackData, 0);

    AImgOpen(readCallback, tellCallback, seekCallback, callbackData, &img, NULL);

    std::vector<uint8_t> imgData2(width*height*numChannels*bytesPerChannel, 0);
    AImgDecodeImage(img, &imgData2[0], AImgFormat::INVALID_FORMAT);
    AImgClose(img);
    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);

    for(uint32_t i = 0; i < imgData2.size(); i++)
    {
        uint8_t diff = abs(imgData[i] - imgData2[i]);

        double percent = diff / 255.0;
        ASSERT_LT(percent, 0.04);
    }
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
