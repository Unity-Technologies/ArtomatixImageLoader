import ctypes
import enums

class PngEncodingOptions(ctypes.Structure):
    PNG_NO_FILTERS   = 0x00
    PNG_FILTER_NONE  = 0x08
    PNG_FILTER_SUB   = 0x10
    PNG_FILTER_UP    = 0x20
    PNG_FILTER_AVG   = 0x40
    PNG_FILTER_PAETH = 0x80
    PNG_ALL_FILTERS  = (PNG_FILTER_NONE | PNG_FILTER_SUB | PNG_FILTER_UP | PNG_FILTER_AVG | PNG_FILTER_PAETH)

    _fields_ = [
        ('type', ctypes.c_int),
        ('compressionLevel', ctypes.c_int),
        ('filter', ctypes.c_int)
    ]

    def __init__(self, compressionLevel, filter):
        self.type = enums.AImgFileFormats['PNG_IMAGE_FORMAT'].val
        self.compressionLevel = compressionLevel
        self.filter = filter
        
