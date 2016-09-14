#ifndef ARTOMATIX_TIFF_H
#define ARTOMATIX_TIFF_H

#include "ImageLoaderBase.h"

namespace AImg
{
    class TIFFImageLoader : public ImageLoaderBase
    {
        public:

        virtual AImgBase* getAImg();

        virtual int32_t initialise();
        virtual bool canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData);
        virtual std::string getFileExtension();
        virtual int32_t getAImgFileFormatValue();

        virtual AImgFormat getWhatFormatWillBeWrittenForData(int32_t inputFormat);
    };
}

#endif //ARTOMATIX_TIFF_H
