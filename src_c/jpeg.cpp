#include "AIL.h"
#include "jpeg.h"
#include "AIL_internal.h"
#include <vector>
#include <jpeglib.h>

namespace AImg
{
    typedef struct
    {
        jpeg_source_mgr pub;
        const JOCTET *data;
        size_t len;

    } ArtomatixJPEGSourceMGR;

    namespace JPEGSourceManagerFunctions
    {
        const static JOCTET EOI_BUFFER[1] = { JPEG_EOI };
        void initSource(j_decompress_ptr cinfo)
        {
            AIL_UNUSED_PARAM(cinfo);
        }

        void skipInputData(j_decompress_ptr cinfo, long num_bytes)
        {
            ArtomatixJPEGSourceMGR * src = (ArtomatixJPEGSourceMGR *)cinfo->src;

            if (src->pub.bytes_in_buffer < num_bytes)
            {
                src->pub.next_input_byte = EOI_BUFFER;
                src->pub.bytes_in_buffer = 1;
            }

            else
            {
                src->pub.next_input_byte += num_bytes;
                src->pub.bytes_in_buffer -= num_bytes;
            }
        }

        boolean fillInputBuffer(j_decompress_ptr cinfo)
        {
            ArtomatixJPEGSourceMGR * src  = (ArtomatixJPEGSourceMGR *)cinfo->src;

            src->pub.next_input_byte = EOI_BUFFER;
            src->pub.bytes_in_buffer = 1;
            return TRUE;
        }

        void termSource(j_decompress_ptr cinfo)
        {
            AIL_UNUSED_PARAM(cinfo);
        }
    }

    void setSourceMGR(j_decompress_ptr cinfo, void * data, size_t len)
    {
        if (cinfo->src == NULL)
            cinfo->src = (jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(ArtomatixJPEGSourceMGR));

        ArtomatixJPEGSourceMGR * src  = (ArtomatixJPEGSourceMGR *)cinfo->src;
        src->pub.init_source = JPEGSourceManagerFunctions::initSource;
        src->pub.fill_input_buffer = JPEGSourceManagerFunctions::fillInputBuffer;
        src->pub.skip_input_data = JPEGSourceManagerFunctions::skipInputData;
        src->pub.term_source = JPEGSourceManagerFunctions::termSource;
        src->pub.resync_to_restart = jpeg_resync_to_restart;
        src->data = (const JOCTET *)data;
        src->len = len;
        src->pub.bytes_in_buffer = len;
        src->pub.next_input_byte = src->data;

    }

    int32_t JPEGImageLoader::initialise()
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }

    bool JPEGImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        return false;
    }

    std::string JPEGImageLoader::getFileExtension()
    {
        return "JPEG";
    }

    int32_t JPEGImageLoader::getAImgFileFormatValue()
    {
        return JPEG_IMAGE_FORMAT;
    }

    class JPEGFile : public AImgBase
    {
        public:

            virtual int32_t getImageInfo(int32_t *width, int32_t *height, int32_t *numChannels, int32_t *bytesPerChannel, int32_t *floatOrInt, int32_t *decodedImgFormat)
            {

                return AImgErrorCode::AIMG_SUCCESS;
            }

            int32_t getDecodeFormat()
            {

                return AImgFormat::INVALID_FORMAT;

            }

            virtual int32_t decodeImage(void *destBuffer, int32_t forceImageFormat)
            {
                return AImgErrorCode::AIMG_SUCCESS;
            }
    };


    AImgBase* JPEGImageLoader::openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        JPEGFile* jpeg = new JPEGFile();


        return jpeg;
    }

    AImgFormat JPEGImageLoader::getWhatFormatWillBeWrittenForData(int32_t inputFormat)
    {
        return AImgFormat::INVALID_FORMAT;
    }

    int32_t JPEGImageLoader::writeImage(void *data, int32_t width, int32_t height, int32_t inputFormat, WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }
}
