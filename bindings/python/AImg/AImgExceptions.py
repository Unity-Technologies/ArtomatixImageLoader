import ail_py_native as native

class AImgException(Exception):
    pass

class AImgUnsupportedFiletypeException(AImgException):
    pass
class AImgLoadFailedExternalException(AImgException):
    pass
class AImgLoadFailedInternalException(AImgException):
    pass
class AImgConversionFailedBadFormatException(AImgException):
    pass
class AImgWriteFailedExternalException(AImgException):
    pass
class AImgWriteFailedInternalException(AImgException):
    pass
class AImgLoadFailedUnsupportedTiffException(AImgException):
    pass
class AImgOpenFailedEmptyInputException(AImgException):
    pass
class AImgInvalidEncodeArgsException(AImgException):
    pass

def checkErrorCode(aImgCapsule, errCode):
    if errCode != 0:

        msg = ""

        if aImgCapsule != None:
            msg = native.getErrorDetails(aImgCapsule)

        if   errCode == -1:
            raise AImgUnsupportedFiletypeException(msg)
        elif errCode == -2:
            raise AImgLoadFailedExternalException(msg)
        elif errCode == -3:
            raise AImgLoadFailedInternalException(msg)
        elif errCode == -4:
            raise AImgConversionFailedBadFormatException(msg)
        elif errCode == -5:
            raise AImgWriteFailedExternalException(msg)
        elif errCode == -6:
            raise AImgWriteFailedInternalException(msg)
        elif errCode == -7:
            raise AImgLoadFailedUnsupportedTiffException(msg)
        elif errCode == -8:
            raise AImgOpenFailedEmptyInputException(msg)
        elif errCode == -9:
            raise AImgInvalidEncodeArgsException(msg)
        else:
            raise AImgException("Unknown error occurred, code: " + str(errCode) + " " + msg) 
