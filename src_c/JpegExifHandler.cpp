#include "JpegExifHandler.hpp"
#include <cstring>
#include <cstdint>

namespace AImg
{
    // Returns orientation code from the specific EXIF tag 0x112
// See https://www.media.mit.edu/pia/Research/deepview/exif.html
    uint16_t JpegExifHandler::GetOrientationField(int16_t * error) const noexcept
    {
        auto marker = this->GetEXIFSegment();
        if (marker == nullptr)
        {
            if (error != nullptr)
            {
                *error = AIMG_EXIF_DATA_NOT_FOUND;
            }

            return 0;
        }
        // Adding 6 to this ptr as the EXIF magic is 6 bytes long
        const uint8_t * tiffHeader = marker->data + 6;

        const auto intelSig = "II";
        auto littleEndian = memcmp(tiffHeader, intelSig, 2) == 0;

        const uint16_t orientationTagId = 0x112;

        uint32_t offset = *((uint32_t*)(tiffHeader + 4));

        if (!littleEndian)
        {
            offset = SwapBytes32(offset);
        }

        const uint8_t * IFD0 = tiffHeader + offset;

        uint16_t tagCount = *((uint16_t*)IFD0);

        if (!littleEndian)
        {
            tagCount = SwapBytes16(tagCount);
        }

        TiffTag_t * tags = (TiffTag_t *)(IFD0 + sizeof(uint16_t));

        TiffTag_t orientationTag;
        bool found = false;

        for (int tagIndex = 0; tagIndex < tagCount; tagIndex++)
        {
            TiffTag_t currentTag = tags[tagIndex];

            if (!littleEndian)
            {
                currentTag = this->SwapTiffTagBytes(currentTag);
            }

            if (currentTag.Id == orientationTagId)
            {
                orientationTag = currentTag;
                found = true;
                break;
            }
        }

        if (found)
        {
            uint16_t orientation = littleEndian
                ? orientationTag.Offset.Partial.Hi
                : orientationTag.Offset.Partial.Lo;

            if (orientation <= 8)
            {
                if (error != nullptr)
                {
                    *error = AIMG_SUCCESS;
                }

                return orientation;
            }
            else
            {
                if (error != nullptr)
                {
                    *error = AIMG_EXIF_INVALID_DATA;
                }

                return 0;
            }
        }
        else
        {
            if (error != nullptr)
            {
                *error = AIMG_EXIF_DATA_NOT_FOUND;
            }

            return 0;
        }
    }

    TiffTag_t JpegExifHandler::SwapTiffTagBytes(TiffTag_t tag)
    {
        tag.Id = SwapBytes16(tag.Id);

        tag.Type = SwapBytes16(tag.Type);

        tag.Count = SwapBytes32(tag.Count);

        tag.Offset.Full = SwapBytes32(tag.Offset.Full);

        return tag;
    }

    // Returns APP1 marker if found, nullptr otherwise
    // See https://www.media.mit.edu/pia/Research/deepview/exif.html
    jpeg_saved_marker_ptr JpegExifHandler::GetEXIFSegment() const noexcept
    {
        auto marker = this->cinfo->marker_list;

        constexpr auto App1Marker = 0xE1;

        while (marker != nullptr)
        {
            if (marker->marker == App1Marker)
            {
                const auto exifMagic = "Exif\0\0";

                if (!memcmp(exifMagic, marker->data, 6))
                    return marker;
                else
                    return nullptr;
            }

            marker = marker->next;
        }

        return nullptr;
    }

    bool JpegExifHandler::SupportsExif() const noexcept
    {
        auto marker = this->GetEXIFSegment();

        return marker != nullptr;
    }
}