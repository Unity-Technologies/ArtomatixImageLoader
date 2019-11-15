#pragma once

#ifndef JPEG_EXIF_HANDLER_HPP
#define JPEG_EXIF_HANDLER_HPP

#include "IExifHandler.hpp"
#include "AIL_internal.h"
#include <jpeglib.h>

namespace AImg
{
    // See https://www.exif.org/Exif2-2.PDF page 13
    typedef struct
    {
        uint16_t Id;
        uint16_t Type;
        uint32_t Count;
        union
        {
            uint32_t Full;

#pragma pack(push, r1, 2)
            struct
            {
                uint16_t Hi;
                uint16_t Lo;
            } Partial;
#pragma pack( pop, r1 )
        } Offset;
    } TiffTag_t;

    class JpegExifHandler : public virtual IExifHandler
    {
    private:

        j_decompress_ptr cinfo;

        jpeg_saved_marker_ptr GetEXIFSegment() const noexcept;
        static TiffTag_t SwapTiffTagBytes(TiffTag_t tag);

    public:

        virtual bool SupportsExif() const noexcept override;
        JpegExifHandler(j_decompress_ptr cinfo) : cinfo(cinfo) {}

        virtual uint16_t GetOrientationField(int16_t * error = nullptr) const noexcept override;
    };
}
#endif