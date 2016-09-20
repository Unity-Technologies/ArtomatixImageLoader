#ifndef ARTOMATIX_AIL_H
#define ARTOMATIX_AIL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" 
{
#endif

#ifdef WIN32
    #define CALLCONV __stdcall
#else
    #define CALLCONV
#endif

// Callback typedefs
typedef int32_t CALLCONV (*ReadCallback)    (void* callbackData, uint8_t* dest, int32_t count);
typedef void    CALLCONV (*WriteCallback)   (void* callbackData, const uint8_t* src, int32_t count);
typedef int32_t CALLCONV (*TellCallback)    (void* callbackData);
typedef void    CALLCONV (*SeekCallback)    (void* callbackData, int32_t pos);

typedef void* AImgHandle;

// format is [channels][bits per channel][U/F]
// U means unsigned normalised, so eg 8U maps integer vals 0-255 to float range 0-1, F means an normal float value
enum AImgFormat
{
    INVALID_FORMAT = -1,

    R8U     = 0,
    RG8U    = 1,
    RGB8U   = 2,
    RGBA8U  = 3,
    
    R16U    = 4,
    RG16U   = 5,
    RGB16U  = 6,
    RGBA16U = 7,

    R16F    = 8,
    RG16F   = 9,
    RGB16F  = 10,
    RGBA16F = 11,

    R32F    = 12,
    RG32F   = 13,
    RGB32F  = 14,
    RGBA32F = 15
};

enum AImgErrorCode
{
    AIMG_SUCCESS = 0,
    AIMG_UNSUPPORTED_FILETYPE = -1,
    AIMG_LOAD_FAILED_EXTERNAL = -2, // load failed in an external library
    AIMG_LOAD_FAILED_INTERNAL = -3, // load failed inside ArtomatixImageLoader
    AIMG_CONVERSION_FAILED_BAD_FORMAT = -4,
    AIMG_WRITE_FAILED_EXTERNAL = -5
};

enum AImgFileFormat
{
    UNKNOWN_IMAGE_FORMAT = -1,
    EXR_IMAGE_FORMAT = 1
};

enum AImgFloatOrIntType
{
    FITYPE_UNKNOWN = -1,
    FITYPE_FLOAT = 0,
    FITYPE_INT = 1
};

void AISetLastErrorDetails(const char* err);
const char* GetLastErrorDetails();

// detectedFileFormat will be set to a member from AImgFileFormat if non-null, otherwise it is ignored.
int32_t AImgOpen(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData, AImgHandle* imgPtr, int32_t* detectedFileFormat);
void AImgClose(AImgHandle img);

int32_t AImgGetInfo(AImgHandle img, int32_t* width, int32_t* height, int32_t* numChannels, int32_t* bytesPerChannel, int32_t* floatOrInt, int32_t* decodedImgFormat);
int32_t AImgDecodeImage(AImgHandle img, void* destBuffer, int32_t forceImageFormat);
int32_t AImgInitialise();
void AImgCleanUp();

void AIGetFormatDetails(int32_t format, int32_t* numChannels, int32_t* bytesPerChannel, int32_t* floatOrInt);
int32_t AImgConvertFormat(void* src, void* dest, int32_t width, int32_t height, int32_t inFormat, int32_t outFormat);

int32_t AImgGetWhatFormatWillBeWrittenForData(int32_t fileFormat, int32_t inputFormat);
int32_t AImgWriteImage(int32_t fileFormat, void* data, int32_t width, int32_t height, int32_t inputFormat, WriteCallback writeCallback,
                   TellCallback tellCallback, SeekCallback seekCallback, void* callbackData);

void AIGetSimpleMemoryBufferCallbacks(ReadCallback* readCallback, WriteCallback* writeCallback, TellCallback* tellCallback, SeekCallback* seekCallback, void** callbackData, void* buffer, int32_t size);
void AIDestroySimpleMemoryBufferCallbacks(ReadCallback readCallback, WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData);


#ifdef __cplusplus
}
#endif

#endif //ARTOMATIX_AIL_H
