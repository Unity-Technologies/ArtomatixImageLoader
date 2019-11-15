#pragma once
#ifndef IEXIFHANDLER_HPP
#define IEXIFHANDLER_HPP

#include <cstdint>

namespace AImg
{
    class IExifHandler
    {
    public:

        virtual bool SupportsExif() const noexcept = 0;
        virtual uint16_t GetOrientationField(int16_t * error = nullptr) const noexcept = 0;
    };
}

#endif