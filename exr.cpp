#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImathBox.h>
#include <ImfIO.h>

#include <stdint.h>
#include <vector>
#include <exception>
#include <algorithm>

#include "AIL.h"
#include "exr.h"

namespace AImg
{
    class CallbackIStream : public Imf::IStream
    {
        public:
            CallbackIStream(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData) : IStream("")
            {
                mReadCallback = readCallback;
                mTellCallback = tellCallback;
                mSeekCallback = seekCallback;
                mCallbackData = callbackData;
            }

            virtual bool read(char c[], int n)
            {
                return mReadCallback(mCallbackData, (uint8_t*)c, n) == n;
            }

            virtual uint64_t tellg()
            {
                return mTellCallback(mCallbackData);
            }

            virtual void seekg(uint64_t pos)
            {
                mSeekCallback(mCallbackData, pos);
            }

            virtual void clear()
            {

            }

            ReadCallback mReadCallback;
            TellCallback mTellCallback;
            SeekCallback mSeekCallback;
            void* mCallbackData;
    };

    class CallbackOStream : public Imf::OStream
    {
        public:
            CallbackOStream(WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData) : OStream("")
            {
                mWriteCallback = writeCallback;
                mTellCallback = tellCallback;
                mSeekCallback = seekCallback;
                mCallbackData = callbackData;
            }

            virtual void write(const char c[], int n)
            {
                mWriteCallback(mCallbackData, (const uint8_t*)c, n);
            }

            virtual uint64_t tellp()
            {
                return mTellCallback(mCallbackData);
            }

            virtual void seekp(uint64_t pos)
            {
                mSeekCallback(mCallbackData, pos);
            }

            virtual void clear()
            {

            }

            WriteCallback mWriteCallback;
            TellCallback mTellCallback;
            SeekCallback mSeekCallback;
            void* mCallbackData;
    };

    int32_t ExrImageLoader::initialise()
    {
        try
        {
            Imf::staticInitialize();
        }
        catch (const std::exception &e)
        {
            AISetLastErrorDetails(e.what());
            return AImgErrorCode::AIMG_LOAD_FAILED_INTERNAL;
        }
    }
    
    bool ExrImageLoader::canLoadImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData)
    {
        int32_t startingPos = tellCallback(callbackData);

        std::vector<uint8_t> header(4);
        readCallback(callbackData, &header[0], 4);

        seekCallback(callbackData, startingPos);

        return header[0] == 0x76 && header[1] == 0x2f && header[2] == 0x31 && header[3] == 0x01;
    }
        
    std::string ExrImageLoader::getFileExtension()
    {
        return "EXR";
    }

    int32_t ExrImageLoader::getAImgFileFormatValue()
    {
        return EXR_IMAGE_FORMAT;
    }


    class ExrFile : public AImgBase
    {
        public:
            CallbackIStream* data;
            Imf::InputFile* file;
            Imath::Box2i dw;

            virtual ~ExrFile()
            {
                delete data;
                delete file;
            }

            int32_t getDecodeFormat()
            {
                // yes, I am actually pretty sure this is the only way to get a channel count...
                int32_t channelNum = 0;
                const Imf::ChannelList &channels = file->header().channels();
                for (Imf::ChannelList::ConstIterator it = channels.begin(); it != channels.end(); ++it)
                    channelNum++;

                if(channelNum > 3)
                    return AImgFormat::RGBA32F;
                if(channelNum == 3)
                    return AImgFormat::RGB32F;
                if(channelNum == 2)
                    return AImgFormat::RG32F;
                if(channelNum == 1)
                    return AImgFormat::R32F;

                return AImgFormat::INVALID_FORMAT;

            }

            virtual int32_t getImageInfo(int32_t* width, int32_t* height, int32_t* numChannels, int32_t* bytesPerChannel, int32_t* floatOrInt, int32_t* decodedImgFormat)
            {
                *width = dw.max.x - dw.min.x + 1;
                *height = dw.max.y - dw.min.y + 1;
                *decodedImgFormat = getDecodeFormat();

                *numChannels = 0;

                Imf::PixelType lastChannelType;
                bool allChannelsSame = true;

                bool isFirstChannel = true;

                const Imf::ChannelList &channels = file->header().channels();
                for (Imf::ChannelList::ConstIterator it = channels.begin(); it != channels.end(); ++it)
                {
                    (*numChannels)++;

                    if(isFirstChannel)
                    {
                        isFirstChannel = false;
                        lastChannelType = it.channel().type;
                    }

                    if(it.channel().type != lastChannelType)
                        allChannelsSame = false;
                }

                if(!allChannelsSame)
                {
                    *bytesPerChannel = -1;
                    *floatOrInt = AImgFloatOrIntType::FITYPE_UNKNOWN;
                }
                else
                {
                    if(lastChannelType == Imf::PixelType::UINT)
                    {
                        *bytesPerChannel = 4;
                        *floatOrInt = AImgFloatOrIntType::FITYPE_INT;
                    }
                    if(lastChannelType == Imf::PixelType::FLOAT)
                    {
                        *bytesPerChannel = 4;
                        *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
                    }
                    else if(lastChannelType == Imf::PixelType::HALF)
                    {
                        *bytesPerChannel = 2;
                        *floatOrInt = AImgFloatOrIntType::FITYPE_FLOAT;
                    }
                    else
                    {
                        AISetLastErrorDetails("Invalid channel type in exr file");
                        return AImgErrorCode::AIMG_LOAD_FAILED_INTERNAL;
                    }
                }

                return AImgErrorCode::AIMG_SUCCESS;
            }

            virtual int32_t decodeImage(void* destBuffer, int32_t forceImageFormat)
            {
                try
                {
                    int32_t width = dw.max.x - dw.min.x + 1;

                    std::vector<std::string> allChannelNames;
                    bool isRgba = true;

                    const Imf::ChannelList &channels = file->header().channels();
                    for (Imf::ChannelList::ConstIterator it = channels.begin(); it != channels.end(); ++it)
                    {
                        std::string name = it.name();
                        allChannelNames.push_back(it.name());
                        if(name != "R" && name != "G" && name != "B" && name != "A")
                            isRgba = false;
                    }

                    std::vector<std::string> usedChannelNames;

                    // ensure RGBA byte order, when loading an rgba image
                    if(isRgba)
                    {
                        if(std::find(allChannelNames.begin(), allChannelNames.end(), "R") != allChannelNames.end())
                            usedChannelNames.push_back("R");
                        if(std::find(allChannelNames.begin(), allChannelNames.end(), "G") != allChannelNames.end())
                            usedChannelNames.push_back("G");
                        if(std::find(allChannelNames.begin(), allChannelNames.end(), "B") != allChannelNames.end())
                            usedChannelNames.push_back("B");
                        if(std::find(allChannelNames.begin(), allChannelNames.end(), "A") != allChannelNames.end())
                            usedChannelNames.push_back("A");
                    }
                    // otherwise just whack em in in order
                    else
                    {
                        for(uint32_t i = 0; i < allChannelNames.size(); i++)
                        {
                            if(usedChannelNames.size() >= 4)
                                break;

                            if(std::find(usedChannelNames.begin(), usedChannelNames.end(), allChannelNames[i]) == allChannelNames.end())
                                usedChannelNames.push_back(allChannelNames[i]);
                        }
                    }

                    Imf::FrameBuffer frameBuffer;

                    for (uint32_t i = 0; i < usedChannelNames.size(); i++)
                    {
                        frameBuffer.insert(usedChannelNames[i],
                            Imf::Slice(Imf::FLOAT,
                                ((char*)destBuffer) + i*sizeof(float),
                                sizeof(float) * usedChannelNames.size(),
                                sizeof(float) * width * usedChannelNames.size(),
                                1, 1,
                                0.0
                            )
                        );
                    }

                    file->setFrameBuffer(frameBuffer);
                    file->readPixels(dw.min.y, dw.max.y);

                    return AImgErrorCode::AIMG_SUCCESS;
                }
                catch (const std::exception &e)
                {
                    AISetLastErrorDetails(e.what());
                    return AImgErrorCode::AIMG_LOAD_FAILED_INTERNAL;
                }
            }

    };


    AImgBase* ExrImageLoader::openImage(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData)
    {
        try
        {
            ExrFile* exr = new ExrFile();
            exr->data = new CallbackIStream(readCallback, tellCallback, seekCallback, callbackData);
            exr->file = new Imf::InputFile(*exr->data);
            exr->dw = exr->file->header().dataWindow();

            return exr;
        }
        catch (const std::exception &e)
        {
            AISetLastErrorDetails(e.what());
            return NULL;
        }
    }




/*
    void exrSave(WriteCallback writeCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData,  float* data, int width, int height)
    {
        CallbackOStream ostream(writeCallback, tellCallback, seekCallback, callbackData);

        Imf::Header header(width, height);
        header.channels().insert("R", Imf::Channel(Imf::FLOAT));
        header.channels().insert("G", Imf::Channel(Imf::FLOAT));
        header.channels().insert("B", Imf::Channel(Imf::FLOAT));

        Imf::OutputFile file(ostream, header);

        Imf::FrameBuffer frameBuffer;

        frameBuffer.insert("R",
            Imf::Slice(Imf::FLOAT,
            (char*)&data[0],
            sizeof(float) * 3,
            sizeof(float) * width * 3,
            1, 1,
            0.0
            )
            );

        frameBuffer.insert("G",
            Imf::Slice(Imf::FLOAT,
            (char*)&data[1],
            sizeof(float) * 3,
            sizeof(float) * width * 3,
            1, 1,
            0.0
            )
            );

        frameBuffer.insert("B",
            Imf::Slice(Imf::FLOAT,
            (char*)&data[2],
            sizeof(float) * 3,
            sizeof(float) * width * 3,
            1, 1,
            0.0
            )
            );

        file.setFrameBuffer(frameBuffer);
        file.writePixels(height);
    }


    ExrFile* exrOpen(ReadCallback readCallback, TellCallback tellCallback, SeekCallback seekCallback, void* callbackData)
    {
        ExrFile* exr = new ExrFile();
        exr->data = new CallbackIStream(readCallback, tellCallback, seekCallback, callbackData);
        exr->file = new Imf::InputFile(*exr->data);
        exr->dw = exr->file->header().dataWindow();

        return exr;
    }

    void exrClose(ExrFile* exr)
    {
        delete exr->data;
        delete exr->file;
        delete exr;
    }

    int exrWidth(ExrFile* exr)
    {
        return exr->dw.max.x - exr->dw.min.x + 1;
    }

    int exrHeight(ExrFile* exr)
    {
        return exr->dw.max.y - exr->dw.min.y + 1;
    }

    int exrChannels(ExrFile* exr, char* buf, int len)
    {
        int pos = 0;

        const Imf::ChannelList &channels = exr->file->header().channels();
        for (Imf::ChannelList::ConstIterator it = channels.begin(); it != channels.end(); ++it)
        {
            const char* name = it.name();
            for (int i = 0; i < strlen(name) && pos < len-1; i++)
            {
                buf[pos] = name[i];
                pos++;
            }

            buf[pos] = ',';
            pos++;
        }

        return pos;
    }

    void exrRead(ExrFile* exr, float* dest, char* channelStr)
    {
        Imf::FrameBuffer frameBuffer;

        int width = exrWidth(exr);

        std::istringstream ss(channelStr);
        std::string token;

        std::vector<std::string> channels;

        while (std::getline(ss, token, ','))
        {
            channels.push_back(token);
        }

        for (int i = 0; i < channels.size(); i++)
        {
            frameBuffer.insert(channels[i],
                Imf::Slice(Imf::FLOAT,
                    (char*)&dest[i],
                    sizeof(float) * channels.size(),
                    sizeof(float) * width * channels.size(),
                    1, 1,
                    0.0
                )
            );
        }
        
        exr->file->setFrameBuffer(frameBuffer);

        exr->file->readPixels(exr->dw.min.y, exr->dw.max.y);
    }

    void exrInit()
    {
        Imf::staticInitialize();
    }*/
}
