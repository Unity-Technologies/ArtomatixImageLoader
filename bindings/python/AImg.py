import ail_py_native as native
import enums
import collections
import numpy as np


RawFileInfo = collections.namedtuple("RawFileInfo", "numChannels bytesPerChannel floatOrInt")

class AImg(object):
    
    def __init__(self, io_or_path):
        if type(io_or_path) == str:
            self.init_from_stream(open(io_or_path), True)
        else:
            self.init_from_stream(io_or_path, False)

    def init_from_stream(self, stream, close_after_decode):
        self._stream = stream
        self._close_after_decode = close_after_decode

        self._callbackData = native.getCallbackDataFromFileLikeObject(self._stream)
        
        self._imgCapsule, detectedFileFormat = native.open(self._callbackData)
        self.detectedFileFormat = enums.AImgFileFormats[detectedFileFormat]

        self.width, self.height, rawNumChannels, rawBytesPerChannel, rawFloatOrInt, decodedImgFormat = native.getInfo(self._imgCapsule)
        self.rawFileInfo = RawFileInfo(rawNumChannels, rawBytesPerChannel, rawFloatOrInt)
        self.decodedImgFormat = enums.AImgFormats[decodedImgFormat]
        
        self._decodeDone = False

    def decode(self, destBuffer=None):
        if self._decodeDone:
            raise IOError("instance has already been decoded")

        self._decodeDone = True

        formatInfo = enums.getFormatInfo(self.decodedImgFormat)

        if destBuffer == None:
            destBuffer = np.zeros(shape=(self.height, self.width, formatInfo.numChannels), dtype = formatInfo.npType, order="C")

        if type(destBuffer) != np.ndarray:
            raise ValueError("destBuffer must be a numpy.ndarray instance")

        if destBuffer.nbytes < self.width * self.height * formatInfo.bytesPerChannel * formatInfo.numChannels:
            raise ValueError("destBuffer is too small")

        if not (destBuffer.flags.c_contiguous and destBuffer.flags.writeable and destBuffer.flags.aligned):
            raise ValueError("destBuffer does not meet flags requirements (c_contiguous & writeable & aligned)")

        native.decode(self._imgCapsule, destBuffer, -1)

        if self._close_after_decode:
            self._stream.close()

        self._imgCapsule = None
        self._callbackData = None

        return destBuffer


def write(io_or_path, data, fileFormat):
    if type(io_or_path) == str:
        stream = open(io_or_path, "wb")
    else:
        stream = io_or_path

    callbackData = native.getCallbackDataFromFileLikeObject(stream)

    fmt = enums.getFormatFromNumpyArray(data)
    height, width = data.shape[0:2]

    native.write(fileFormat.val, data, stream, width, height, fmt.val)
