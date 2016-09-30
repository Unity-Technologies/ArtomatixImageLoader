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
        else:
            raise AImgException("Unknown error occurred, code: " + str(errCode) + " " + msg) 
