#include <gtest/gtest.h>
#include "testCommon.h"
#include "../AIL.h"
#include <jpeglib.h>


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


TEST(JPEG, TestDetectJPEG)
{
    ASSERT_TRUE(detectImage("/jpeg/test.jpeg", JPEG_IMAGE_FORMAT));
}

TEST(JPEG, TestReadJPEGAttrs)
{
    ASSERT_TRUE(validateImageHeaders("/jpeg/test.jpeg", 640, 400, 3, 1, AImgFloatOrIntType::FITYPE_INT, AImgFormat::RGB8U));
}

int main(int argc, char **argv)
{
    AImgInitialise();

    ::testing::InitGoogleTest(&argc, argv);
    int retval = RUN_ALL_TESTS();

    AImgCleanUp();

    return retval;
}

TEST(JPEG, TestReadJPEGFile)
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
    int32_t imgFmt;

    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &imgFmt);

    std::vector<uint8_t> imgData(width*height*numChannels*bytesPerChannel, 78);

    int32_t error = AImgDecodeImage(img, &imgData[0], AImgFormat::INVALID_FORMAT);

    if (error != AImgErrorCode::AIMG_SUCCESS)
    {
        std::cout << AIGetLastErrorDetails() << std::endl;
    }

    auto knownData = decodeJPEGFile(getImagesDir() + "/jpeg/test.jpeg");

    for(int32_t y = 0; y < height; y++)
    {
        for(int32_t x = 0; x < width; x++)
        {
            ASSERT_EQ(knownData[x + width*y], imgData[(x + width*y)]);
        }
    }

    AIDestroySimpleMemoryBufferCallbacks(readCallback, writeCallback, tellCallback, seekCallback, callbackData);
    AImgClose(img);
}
