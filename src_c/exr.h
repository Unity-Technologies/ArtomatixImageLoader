#ifndef ARTOMATIX_EXR_H
#define ARTOMATIX_EXR_H

#include "ImageLoaderBase.h"

namespace AImg
{
    class ExrImageLoader : public ImageLoaderBase
    {
        public:

        virtual AImgBase* getAImg();

        virtual int32_t initialise();
        virtual bool canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData);
        virtual std::string getFileExtension();
        virtual int32_t getAImgFileFormatValue();

        virtual bool isFormatSupported(int32_t format);

        virtual AImgFormat getWhatFormatWillBeWrittenForData(int32_t inputFormat, int32_t outputFormat);
    };
}

#endif //ARTOMATIX_EXR_H 
