/*
 * Copyright 2016-2019 Artomatix LTD
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#ifndef ARTOMATIX_AIL_INTERNAL_H
#define ARTOMATIX_AIL_INTERNAL_H

#include "AIL.h"

#include <stdlib.h> // Required for _byteswap_ushort
#ifdef _MSC_VER
#define SwapBytes16 _byteswap_ushort
#define SwapBytes32 _byteswap_ulong
#elif __GNUC__ || __clang__
#define SwapBytes16 __builtin_bswap16
#define SwapBytes32 __builtin_bswap32
#endif

#define AIL_UNUSED_PARAM(name) (void)(name)
bool IsMachineBigEndian();

typedef struct CallbackData
{
    ReadCallback readCallback;
    TellCallback tellCallback;
    SeekCallback seekCallback;
    WriteCallback writeCallback;
    void * callbackData;
} CallbackData;

#endif // ARTOMATIX_AIL_INTERNAL_H
