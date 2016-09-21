#include "AIL.h"
#include "png.h"
#include "AIL_internal.h"
#include <vector>
#include <png.h>

namespace AImg
{

    typedef struct CallbackData
    {
            ReadCallback readCallback;
            TellCallback tellCallback;
            SeekCallback seekCallback;
            WriteCallback writeCallback;
            void * callbackData;

    } CallbackData;

    int32_t PNGImageLoader::initialise()
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }

    bool PNGImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        int32_t startingPosition = tellCallback(callbackData);
        std::vector<uint8_t> header(8);
        readCallback(callbackData, &header[0], 8);

        seekCallback(callbackData, startingPosition);

        uint8_t png_signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};

        return ((int32_t)(memcmp(&header[0], &png_signature[0], 8))) == 0;

    }


    void png_custom_read_data(png_struct* png_ptr, png_byte* data, png_size_t length)
    {
        CallbackData callbackData = *((CallbackData *)png_get_io_ptr(png_ptr));

        callbackData.readCallback(callbackData.callbackData, data, length);

    }

    void png_custom_write_data(png_struct* png_ptr, png_byte* data, png_size_t length)
    {
        CallbackData callbackData = *((CallbackData *)png_get_io_ptr(png_ptr));

        callbackData.writeCallback(callbackData.callbackData, data, length);


    }

    std::string PNGImageLoader::getFileExtension()
    {
        return "PNG";
    }


    int32_t PNGImageLoader::getAImgFileFormatValue()
    {
        return PNG_IMAGE_FORMAT;
    }

    void user_flush_data(png_struct* png_ptr)
    {
        AIL_UNUSED_PARAM(png_ptr);
    }


    class PNGFile : public AImgBase
    {
        public:
            CallbackData * data;
            png_info * png_info_ptr;
            png_struct * png_read_ptr;
            png_struct * png_write_ptr;
            uint32_t width;
            uint32_t height;
            uint8_t colour_type;
            uint8_t bit_depth;
            uint8_t numChannels;


            PNGFile()
            {
                data = new CallbackData();
            }

            virtual ~PNGFile()
            {
                delete data;
            }

            virtual int32_t getImageInfo(int32_t *width, int32_t *height, int32_t *numChannels, int32_t *bytesPerChannel, int32_t *floatOrInt, int32_t *decodedImgFormat)
            {
                *width = this->width;
                *height = this->height;
                *numChannels = this->numChannels;

                if (bit_depth / 8 == 0)
                    *bytesPerChannel = -1;
                else
                    *bytesPerChannel = bit_depth/8;

                *floatOrInt = AImgFloatOrIntType::FITYPE_INT;

                *decodedImgFormat = getDecodeFormat();

                return 0;
            }

            int32_t getDecodeFormat()
            {
                if (bit_depth == 8)
                {
                    if (numChannels == 1)
                        return AImgFormat::R8U;
                    else if (numChannels == 2)
                        return AImgFormat::RG8U;
                    else if (numChannels == 3)
                        return AImgFormat::RGB8U;
                    else if (numChannels == 4)
                        return AImgFormat::RGBA8U;
                }

                else if (bit_depth == 16)
                {
                    if (numChannels == 1)
                        return AImgFormat::R16U;
                    else if (numChannels == 2)
                        return AImgFormat::RG16U;
                    else if (numChannels == 3)
                        return AImgFormat::RGB16U;
                    else if (numChannels == 4)
                        return AImgFormat::RGBA16U;
                }

                return AImgFormat::INVALID_FORMAT;

            }

            virtual int32_t decodeImage(void *destBuffer, int32_t forceImageFormat)
            {
                return 0;
            }

    };



    AImgBase* PNGImageLoader::openImage(ReadCallback readCallback, WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        try
        {
            PNGFile* png = new PNGFile();
            png->data->readCallback = readCallback;
            png->data->tellCallback = tellCallback;
            png->data->seekCallback = seekCallback;
            png->data->callbackData = callbackData;
            png->data->writeCallback = writeCallback;

            png->png_read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
            png->png_write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
            png->png_info_ptr = png_create_info_struct(png->png_read_ptr);

            png->width = png_get_image_width(png->png_read_ptr, png->png_info_ptr);
            png->height = png_get_image_height(png->png_read_ptr, png->png_info_ptr);
            png->bit_depth = png_get_bit_depth(png->png_read_ptr, png->png_info_ptr);
            png->numChannels = png_get_channels(png->png_read_ptr, png->png_info_ptr);
            png->colour_type = png_get_color_type(png->png_read_ptr, png->png_info_ptr);
            png_set_read_fn(png->png_read_ptr, (void *)(png->data), png_custom_read_data);
            png_set_write_fn(png->png_write_ptr, (void *)(png->data), png_custom_write_data, user_flush_data);
            return png;

        }
        catch (const std::exception &e)
        {
            AISetLastErrorDetails((e.what()));
            return NULL;
        }

    }

    AImgFormat PNGImageLoader::getWhatFormatWillBeWrittenForData(int32_t inputFormat)
    {
        return AImgFormat::INVALID_FORMAT;
    }

    int32_t PNGImageLoader::writeImage(void *data, int32_t width, int32_t height, int32_t inputFormat, WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        return 0;
    }
}
