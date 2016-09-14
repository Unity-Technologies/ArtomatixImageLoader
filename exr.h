#ifndef ARTOMATIX_EXR_H
#define ARTOMATIX_EXR_H

#include "ImageLoaderBase.h"

namespace AImg
{
    class ExrImageLoader : public ImageLoaderBase
    {
        public:

        virtual bool canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData);
        virtual std::string getFileExtension();
        virtual int32_t getAImgFileFormatValue();
        virtual AImgBase* openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData);
    };
}

#endif //ARTOMATIX_EXR_H 
