import collections
import numpy as np

class AILEnum(object):
    def __init__(self, entryType, dIn):
        self.d1 = {k: entryType(k, v) for k, v in dIn.iteritems()}
        self.d2 = {v: entryType(k, v) for k, v in dIn.iteritems()}
        
    def __getitem__(self, index):
        try:
            if type(index) == str:
                return self.d1[index]
            else:
                return self.d2[index] 
        except:
            pass

        raise Exception("invalid enum value")

AImgFormat = collections.namedtuple("AImgFormat", "name val")
AImgFormats = AILEnum(AImgFormat, {
    "INVALID_FORMAT" : -1,

    "R8U"     : 0,
    "RG8U"    : 1,
    "RGB8U"   : 2,
    "RGBA8U"  : 3,
    
    "R16U"    : 4,
    "RG16U"   : 5,
    "RGB16U"  : 6,
    "RGBA16U" : 7,

    "R16F"    : 8,
    "RG16F"   : 9,
    "RGB16F"  : 10,
    "RGBA16F" : 11,

    "R32F"    : 12,
    "RG32F"   : 13,
    "RGB32F"  : 14,
    "RGBA32F" : 15
})

AImgFileFormat = collections.namedtuple("AImgFileFormat", "name val")
AImgFileFormats = AILEnum(AImgFileFormat, {
    "UNKNOWN_IMAGE_FORMAT" : -1,
    "EXR_IMAGE_FORMAT" : 1
})

AImgFloatOrIntType = collections.namedtuple("AImgFloatOrIntType", "name val")
AImgFloatOrIntTypes = AILEnum(AImgFloatOrIntType, {
    "FITYPE_UNKNOWN" : -1,
    "FITYPE_FLOAT" : 0,
    "FITYPE_INT" : 1
})


AImgFormatInfo = collections.namedtuple("AImgFormatInfo", "numChannels bytesPerChannel floatOrInt npType")
def getFormatInfo(f):
    channels = 0

    channelSection = "RGBA"

    for i in range(len(channelSection)):
        if f.name[i] == channelSection[i]:
            channels += 1
        else:
            break

    bytesPerChannel = int(f.name[channels:-1])/8

    if f.name[-1] == "F":
        floatOrInt = AImgFloatOrIntTypes["FITYPE_FLOAT"]

        if bytesPerChannel == 4:
            npType = np.float32
        elif bytesPerChannel == 2:
            npType = np.float16
    else:
        floatOrInt = AImgFloatOrIntTypes["FITYPE_INT"]

        if bytesPerChannel == 2:
            npType = np.uint16
        elif bytesPerChannel == 1:
            npType = np.uint8

    return AImgFormatInfo(channels, bytesPerChannel, floatOrInt, npType)

def getFormatFromNumpyArray(data):
    if len(data.shape) < 3:
        numChannels = 1
    else:
        numChannels = data.shape[2]

    if numChannels > 4:
        raise ValueError("array has too many channels")
    
    formatStr = "RGBA"[0:numChannels]

    if data.dtype == np.uint8:
        formatStr += "8U"
    elif data.dtype == np.uint16:
        formatStr += "16U"
    elif data.dtype == np.float16:
        formatStr += "16F"
    elif data.dtype == np.float32:
        formatStr += "32F"
    else:
        raise ValueError("format " + str(data.dtype) + " is not supported")

    return AImgFormats[formatStr]
