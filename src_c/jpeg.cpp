#include "AIL.h"
#include "jpeg.h"
#include "AIL_internal.h"
#include <vector>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>


namespace AImg
{
    typedef struct
    {
        jpeg_source_mgr pub;
        void *data;
        CallbackData callbackFunctionData;

    } ArtomatixJPEGSourceMGR;

    typedef struct
    {
            jpeg_error_mgr pub;
            jmp_buf buf;
    }ArtomatixErrorStruct;

    namespace JPEGCallbackFunctions
    {
        // Buffer size used in libjpeg
        const size_t INPUT_BUFFER_SIZE = 4096;

        void initSource(j_decompress_ptr cinfo)
        {
            AIL_UNUSED_PARAM(cinfo);
        }

        void skipInputData(j_decompress_ptr cinfo, long num_bytes)
        {
            ArtomatixJPEGSourceMGR * src = (ArtomatixJPEGSourceMGR *)cinfo->src;
            if (num_bytes > 0)
            {
                while (num_bytes > (long)src->pub.bytes_in_buffer)
                {

                    num_bytes -= src->pub.bytes_in_buffer;
                    (*src->pub.fill_input_buffer)(cinfo);
                }
            }
        }

        boolean fillInputBuffer(j_decompress_ptr cinfo)
        {
            ArtomatixJPEGSourceMGR * src  = (ArtomatixJPEGSourceMGR *)cinfo->src;
            size_t bytesRead = src->callbackFunctionData.readCallback(src->callbackFunctionData.callbackData, (uint8_t *)src->data, INPUT_BUFFER_SIZE);

            if (bytesRead <= 0)
                return FALSE;

            src->pub.bytes_in_buffer = bytesRead;
            src->pub.next_input_byte = (JOCTET *)src->data;

            return TRUE;
        }

        void termSource(j_decompress_ptr cinfo)
        {
            AIL_UNUSED_PARAM(cinfo);
        }

        void lessAnnoyingEmitMessage(j_common_ptr cinfo, int msg_level)
        {
              if (msg_level == 0)
                (*cinfo->err->output_message) (cinfo);
        }

        void handleFatalError(j_common_ptr cinfo)
        {
            ArtomatixErrorStruct * err = (ArtomatixErrorStruct *) cinfo->err;
            longjmp(err->buf, 1);
        }
    }

    void setArtomatixSourceMGR(j_decompress_ptr cinfo, CallbackData callbackData)
    {
        if (cinfo->src == NULL)
        {
            cinfo->src = (jpeg_source_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(ArtomatixJPEGSourceMGR));
            ((ArtomatixJPEGSourceMGR * )cinfo->src)->data = malloc(JPEGCallbackFunctions::INPUT_BUFFER_SIZE);
            ((ArtomatixJPEGSourceMGR *)cinfo->src)->callbackFunctionData = callbackData;
        }

        ArtomatixJPEGSourceMGR * src  = (ArtomatixJPEGSourceMGR *)cinfo->src;
        src->pub.init_source = JPEGCallbackFunctions::initSource;
        src->pub.fill_input_buffer = JPEGCallbackFunctions::fillInputBuffer;
        src->pub.skip_input_data = JPEGCallbackFunctions::skipInputData;
        src->pub.term_source = JPEGCallbackFunctions::termSource;
        src->pub.resync_to_restart = jpeg_resync_to_restart;
        src->pub.next_input_byte = (JOCTET *)src->data;
        src->pub.bytes_in_buffer = 0;

    }

    int32_t JPEGImageLoader::initialise()
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }

    bool JPEGImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        uint8_t possible_magic1[] = {0xFF, 0xD8, 0xFF, 0xDB};
        uint8_t possible_magic2[] = {0xFF, 0xD8, 0xFF, 0xE0};
        uint8_t possible_magic3[] = {0xFF, 0xD8, 0xFF, 0xE1};
        uint8_t possible_magic2_end[] = {0x4A, 0x46, 0x49, 0x46, 0x00, 0x01};
        uint8_t possible_magic3_end[] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};

        bool goodHeader = false;

        std::vector<uint8_t> header(4);
        seekCallback(callbackData, 0);
        readCallback(callbackData, &header[0], 4);

        goodHeader = ((int32_t)memcmp(&header[0], possible_magic1, 4)) == 0;

        std::vector<uint8_t> header_end(6);
        seekCallback(callbackData, 6);
        readCallback(callbackData, &header_end[0], 6);
        seekCallback(callbackData, 0);


        if (goodHeader)
            return goodHeader;

        else if(((int32_t)memcmp(&header[0], possible_magic2, 4)) == 0)
        {
            if(((int32_t)memcmp(&header_end[0], possible_magic2_end, 6)) == 0)
                return true;
            else
                return false;
        }

        else if(((int32_t)memcmp(&header[0], possible_magic3, 4)) == 0)
        {
            if(((int32_t)memcmp(&header_end[0], possible_magic3_end, 6)) == 0)
                return true;
            else
                return false;
        }

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
            jpeg_decompress_struct jpeg_read_struct;
            ArtomatixErrorStruct err_mgr;


            JPEGFile()
            {
                jpeg_create_decompress(&jpeg_read_struct);
            }

            virtual ~JPEGFile()
            {
                free(((ArtomatixJPEGSourceMGR *)jpeg_read_struct.src)->data);
                jpeg_destroy_decompress(&jpeg_read_struct);
            }

            virtual int32_t getImageInfo(int32_t *width, int32_t *height, int32_t *numChannels, int32_t *bytesPerChannel, int32_t *floatOrInt, int32_t *decodedImgFormat)
            {
                *width = jpeg_read_struct.image_width;
                *height = jpeg_read_struct.image_height;
                *bytesPerChannel = 1;
                *numChannels = 3;
                *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
                *decodedImgFormat = AImgFormat::RGB8U;
                return AImgErrorCode::AIMG_SUCCESS;
            }

            virtual int32_t decodeImage(void *destBuffer, int32_t forceImageFormat)
            {
                ArtomatixErrorStruct * err_ptr = (ArtomatixErrorStruct *) jpeg_read_struct.err;

                if (setjmp(err_ptr->buf))
                {
                    AISetLastErrorDetails("[AImg::JPEGFile::decodeImage] jpeg_start_decompress failed!");
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }

                jpeg_start_decompress(&jpeg_read_struct);

                int row_stride = jpeg_read_struct.output_components * jpeg_read_struct.output_width;

                JSAMPARRAY buffer = (JSAMPARRAY) malloc(sizeof(JSAMPROW));
                buffer[0] = (JSAMPROW) destBuffer;

                if (setjmp(err_ptr->buf))
                {
                    AISetLastErrorDetails("[AImg::JPEGFile::decodeImage] jpeg_read_scanlines failed!");
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }
                while (jpeg_read_struct.output_scanline < jpeg_read_struct.output_height)
                {
                    jpeg_read_scanlines(&jpeg_read_struct, buffer, 1);
                    buffer[0] = (uint8_t * )buffer[0] + row_stride;
                }

                jpeg_finish_decompress(&jpeg_read_struct);

                if (forceImageFormat != AImgFormat::INVALID_FORMAT)
                {
                    int32_t numChannels, bytesPerChannel, floatOrInt, width, height;
                    AIGetFormatDetails(forceImageFormat, &numChannels, &bytesPerChannel, &floatOrInt);

                    std::vector<uint8_t> convertBuffer(width * height * numChannels * bytesPerChannel);

                    int32_t convertError = AImgConvertFormat(destBuffer, &convertBuffer[0], width, height, AImgFormat::R8U, forceImageFormat);

                    if (convertError != AImgErrorCode::AIMG_SUCCESS)
                        return convertError;
                }

                free(buffer);

                return AImgErrorCode::AIMG_SUCCESS;
            }
    };


    AImgBase* JPEGImageLoader::openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        JPEGFile* jpeg = new JPEGFile();

        CallbackData data;

        data.callbackData = callbackData;
        data.readCallback = readCallback;
        data.tellCallback = tellCallback;
        data.seekCallback = seekCallback;

        setArtomatixSourceMGR(&jpeg->jpeg_read_struct, data);
        jpeg->jpeg_read_struct.err = jpeg_std_error(&jpeg->err_mgr.pub);
        jpeg->jpeg_read_struct.err->emit_message = JPEGCallbackFunctions::lessAnnoyingEmitMessage;
        jpeg->jpeg_read_struct.err->error_exit = JPEGCallbackFunctions::handleFatalError;
        jpeg_read_header(&jpeg->jpeg_read_struct, TRUE);


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
