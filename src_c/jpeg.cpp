#include "AIL.h"
#include "jpeg.h"
#include "AIL_internal.h"
#include <vector>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>

#ifdef HAVE_JPEG
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
        jpeg_destination_mgr pub;
        void *buffer;
        CallbackData callbackFunctionData;
    } ArtomatixJPEGDestinationMGR;

    typedef struct
    {
            jpeg_error_mgr pub;
            jmp_buf buf;
    } ArtomatixErrorStruct;

    namespace JPEGConsts
    {
        // Buffer size used in libjpeg
        const size_t BUFFER_SIZE = 4096;
        const int Quality = 99;

    }

    namespace JPEGCallbackFunctions
    {
        namespace WriteFunctions
        {
            void initDestination(j_compress_ptr cinfo)
            {
                AIL_UNUSED_PARAM(cinfo);
            }

            boolean emptyOutputBuffer(j_compress_ptr cinfo)
            {
                ArtomatixJPEGDestinationMGR * dst = (ArtomatixJPEGDestinationMGR *) cinfo->dest;

                dst->callbackFunctionData.writeCallback(dst->callbackFunctionData.callbackData, (uint8_t *)dst->buffer, JPEGConsts::BUFFER_SIZE);
                dst->pub.next_output_byte = (JOCTET *) dst->buffer;
                dst->pub.free_in_buffer = JPEGConsts::BUFFER_SIZE;
                return TRUE;
            }

            void termDestination(j_compress_ptr cinfo)
            {

                ArtomatixJPEGDestinationMGR * dst = (ArtomatixJPEGDestinationMGR *) cinfo->dest;
                size_t datacount = JPEGConsts::BUFFER_SIZE - dst->pub.free_in_buffer;
                if (datacount > 0)
                    dst->callbackFunctionData.writeCallback(dst->callbackFunctionData.callbackData, (uint8_t *)dst->buffer, datacount);
            }
        }

        namespace ReadFunctions
        {
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
                size_t bytesRead = src->callbackFunctionData.readCallback(src->callbackFunctionData.callbackData, (uint8_t *)src->data, JPEGConsts::BUFFER_SIZE);

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
            ((ArtomatixJPEGSourceMGR * )cinfo->src)->data = (void *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, JPEGConsts::BUFFER_SIZE);
            ((ArtomatixJPEGSourceMGR *)cinfo->src)->callbackFunctionData = callbackData;
        }

        ArtomatixJPEGSourceMGR * src  = (ArtomatixJPEGSourceMGR *)cinfo->src;
        src->pub.init_source = JPEGCallbackFunctions::ReadFunctions::initSource;
        src->pub.fill_input_buffer = JPEGCallbackFunctions::ReadFunctions::fillInputBuffer;
        src->pub.skip_input_data = JPEGCallbackFunctions::ReadFunctions::skipInputData;
        src->pub.term_source = JPEGCallbackFunctions::ReadFunctions::termSource;
        src->pub.resync_to_restart = jpeg_resync_to_restart; // Default function from libjpeg
        src->pub.next_input_byte = (JOCTET *)src->data;
        src->pub.bytes_in_buffer = 0;
    }


    void setArtomatixDestinationMGR(j_compress_ptr cinfo, CallbackData callbackData)
    {
        if (cinfo->dest == NULL)
        {
            cinfo->dest = (jpeg_destination_mgr *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(ArtomatixJPEGDestinationMGR));
            ((ArtomatixJPEGDestinationMGR *)cinfo->dest)->buffer = (void *)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, JPEGConsts::BUFFER_SIZE);
        }

        ArtomatixJPEGDestinationMGR * src = (ArtomatixJPEGDestinationMGR *)cinfo->dest;
        src->callbackFunctionData = callbackData;
        src->pub.init_destination = JPEGCallbackFunctions::WriteFunctions::initDestination;
        src->pub.empty_output_buffer = JPEGCallbackFunctions::WriteFunctions::emptyOutputBuffer;
        src->pub.term_destination = JPEGCallbackFunctions::WriteFunctions::termDestination;
        src->pub.next_output_byte = (JOCTET *) src->buffer;
        src->pub.free_in_buffer = JPEGConsts::BUFFER_SIZE;

    }

    int32_t JPEGImageLoader::initialise()
    {
        return AImgErrorCode::AIMG_SUCCESS;
    }

    bool JPEGImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
    {
        AIL_UNUSED_PARAM(tellCallback);
        uint8_t possible_magic1[] = {0xFF, 0xD8, 0xFF, 0xDB};
        uint8_t possible_magic2[] = {0xFF, 0xD8, 0xFF, 0xE0};
        uint8_t possible_magic3[] = {0xFF, 0xD8, 0xFF, 0xE1};
        uint8_t possible_magic2_end[] = {0x4A, 0x46, 0x49, 0x46, 0x00, 0x01};
        uint8_t possible_magic3_end[] = {0x45, 0x78, 0x69, 0x66, 0x00, 0x00};

        bool goodHeader = false;

        int startingPosition = tellCallback(callbackData);
        std::vector<uint8_t> header(4);        
        readCallback(callbackData, &header[0], 4);

        goodHeader = ((int32_t)memcmp(&header[0], possible_magic1, 4)) == 0;

        std::vector<uint8_t> header_end(6);        
        seekCallback(callbackData, startingPosition + 6);
        readCallback(callbackData, &header_end[0], 6);
        seekCallback(callbackData, startingPosition);


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
                jpeg_destroy_decompress(&jpeg_read_struct);
            }


            int32_t openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
            {
                CallbackData data;
                data.callbackData = callbackData;
                data.readCallback = readCallback;
                data.tellCallback = tellCallback;
                data.seekCallback = seekCallback;

                setArtomatixSourceMGR(&jpeg_read_struct, data);
                jpeg_read_struct.err = jpeg_std_error(&err_mgr.pub);
                jpeg_read_struct.err->emit_message = JPEGCallbackFunctions::lessAnnoyingEmitMessage;
                jpeg_read_struct.err->error_exit = JPEGCallbackFunctions::handleFatalError;
                jpeg_read_header(&jpeg_read_struct, TRUE);
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
                    mErrorDetails = "[AImg::JPEGImageLoader::JPEGFile::decodeImage] jpeg_start_decompress failed!";
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }

                jpeg_start_decompress(&jpeg_read_struct);

                int row_stride = jpeg_read_struct.output_components * jpeg_read_struct.output_width;

                JSAMPROW buffer[1];

                buffer[0] = (JSAMPROW) destBuffer;

                if (setjmp(err_ptr->buf))
                {
                    mErrorDetails = "[AImg::JPEGImageLoader::decodeImage] jpeg_read_scanlines failed!";
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
                    int32_t numChannels, bytesPerChannel, floatOrInt;
                    AIGetFormatDetails(forceImageFormat, &numChannels, &bytesPerChannel, &floatOrInt);

                    std::vector<uint8_t> convertBuffer(jpeg_read_struct.image_width * jpeg_read_struct.image_height * numChannels * bytesPerChannel);

                    int32_t convertError = AImgConvertFormat(destBuffer, &convertBuffer[0], jpeg_read_struct.image_width, jpeg_read_struct.image_height, AImgFormat::R8U, forceImageFormat);

                    if (convertError != AImgErrorCode::AIMG_SUCCESS)
                        return convertError;
                }

                return AImgErrorCode::AIMG_SUCCESS;
            }

            int32_t writeImage(void *data, int32_t width, int32_t height, int32_t inputFormat, WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void *callbackData)
            {
                std::vector<uint8_t> convertBuffer(0);
                if (inputFormat != AImgFormat::RGB8U)
                {
                    convertBuffer.resize(width * height * 3);

                    int32_t convertError = AImgConvertFormat(data, &convertBuffer[0], width, height, inputFormat, AImgFormat::RGB8U);

                    if (convertError != AImgErrorCode::AIMG_SUCCESS)
                        return convertError;
                    data = &convertBuffer[0];
                }

                jpeg_compress_struct cinfo;
                ArtomatixErrorStruct jerr;
                CallbackData dataStruct;
                dataStruct.writeCallback = writeCallback;
                dataStruct.tellCallback = tellCallback;
                dataStruct.seekCallback = seekCallback;
                dataStruct.callbackData = callbackData;

                cinfo.err = jpeg_std_error(&jerr.pub);
                cinfo.err->emit_message = JPEGCallbackFunctions::lessAnnoyingEmitMessage;
                cinfo.err->error_exit = JPEGCallbackFunctions::handleFatalError;
                jpeg_create_compress(&cinfo);

                setArtomatixDestinationMGR(&cinfo, dataStruct);

                cinfo.image_width = width;
                cinfo.image_height = height;
                cinfo.input_components = 3;
                cinfo.in_color_space = JCS_RGB;

                jpeg_set_defaults(&cinfo);

                jpeg_set_quality(&cinfo, JPEGConsts::Quality, TRUE);

                if (setjmp(jerr.buf))
                {
                    mErrorDetails = "[AImg::JPEGImageLoader::writeImage] jpeg_start_compress failed!";
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }
                jpeg_start_compress(&cinfo, TRUE);

                int row_stride = width * cinfo.input_components;

                JSAMPROW row_pointer[1];


                if (setjmp(jerr.buf))
                {
                    mErrorDetails = "[AImg::JPEGImageLoader::writeImage] jpeg_write_scanlines failed!";
                    return AImgErrorCode::AIMG_LOAD_FAILED_EXTERNAL;
                }

                while(cinfo.next_scanline < cinfo.image_height)
                {
                    row_pointer[0] = (uint8_t *)data + row_stride * cinfo.next_scanline;
                    jpeg_write_scanlines(&cinfo, row_pointer, 1);
                }

                jpeg_finish_compress(&cinfo);
                jpeg_destroy_compress(&cinfo);

                return AImgErrorCode::AIMG_SUCCESS;
            }
    };

    AImgFormat JPEGImageLoader::getWhatFormatWillBeWrittenForData(int32_t inputFormat)
    {
        AIL_UNUSED_PARAM(inputFormat);
        return AImgFormat::RGB8U;
    }

    AImgBase* JPEGImageLoader::getAImg()
    {
        return new JPEGFile();
    }
}

#endif // HAVE_JPEG
