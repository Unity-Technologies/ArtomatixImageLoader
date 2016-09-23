#include <Python.h>
#include <numpy/arrayobject.h>

#include "AIL.h"

//static char module_docstring[] = "This module provides an interface for calculating chi-squared using C.";

typedef struct
{
    PyObject* fileLikeObj;

    PyObject* readMethod;
    PyObject* writeMethod;
    PyObject* tellMethod;
    PyObject* seekMethod;
} PyAIL_callback_data;



int32_t CALLCONV pyail_ReadCallback(void* callbackDataV, uint8_t* dest, int32_t count)
{
    PyAIL_callback_data* callbackData = callbackDataV;

    PyObject* read_args = Py_BuildValue("(i)", count);
    PyObject* data = PyObject_Call(callbackData->readMethod, read_args, NULL);
    Py_DECREF(read_args);

    char* dataRead;
    Py_ssize_t dataReadLen;
    PyBytes_AsStringAndSize(data, &dataRead, &dataReadLen);

    memcpy(dest, dataRead, dataReadLen);

    Py_DECREF(data);

    return dataReadLen;
}

void CALLCONV pyail_WriteCallback(void* callbackDataV, const uint8_t* src, int32_t count)
{
    PyAIL_callback_data* callbackData = callbackDataV;

    PyObject* writeArgs = Py_BuildValue("(s#)", src, count);
    PyObject_Call(callbackData->writeMethod, writeArgs, NULL);

    Py_DECREF(writeArgs);
}

int32_t CALLCONV pyail_TellCallback(void* callbackDataV)
{
    PyAIL_callback_data* callbackData = callbackDataV;

    PyObject* tellArgs = Py_BuildValue("()");
    PyObject* pyTellVal = PyObject_Call(callbackData->tellMethod, tellArgs, NULL);
    Py_ssize_t tellVal = PyInt_AsSsize_t(pyTellVal);
    Py_DECREF(pyTellVal);
    Py_DECREF(tellArgs);

    return tellVal;
}

void CALLCONV pyail_SeekCallback (void* callbackDataV, int32_t pos)
{
    PyAIL_callback_data* callbackData = callbackDataV;

    PyObject* seekArgs = Py_BuildValue("(i)", pos);
    PyObject_Call(callbackData->seekMethod, seekArgs, NULL);
    Py_DECREF(seekArgs);
}

void pyail_callbackDataDestructor(PyObject* callbackDataCapsule)
{
    printf("FREEING CALLBAC DATA\n");
    PyAIL_callback_data* callbackData = PyCapsule_GetPointer(callbackDataCapsule, NULL);
    Py_DECREF(callbackData->readMethod);
    Py_DECREF(callbackData->writeMethod);
    Py_DECREF(callbackData->tellMethod);
    Py_DECREF(callbackData->seekMethod);
    Py_DECREF(callbackData->fileLikeObj);

    free(callbackData);
}

static PyObject* pyail_getCallbackDataFromFileLikeObject(PyObject* self, PyObject* args)
{
    PyObject* fileLikeObj;

    if (!PyArg_ParseTuple(args, "O", &fileLikeObj))
        return NULL;

    Py_INCREF(fileLikeObj);

    PyObject* readMethod;
    if ((readMethod = PyObject_GetAttrString(fileLikeObj, "read")) == NULL)
       return NULL;

    PyObject* writeMethod;
    if ((writeMethod = PyObject_GetAttrString(fileLikeObj, "write")) == NULL)
       return NULL;

    PyObject* tellMethod;
    if ((tellMethod = PyObject_GetAttrString(fileLikeObj, "tell")) == NULL)
       return NULL;

    PyObject* seekMethod;
    if ((seekMethod = PyObject_GetAttrString(fileLikeObj, "seek")) == NULL)
       return NULL;

    PyAIL_callback_data* callbackData = malloc(sizeof(PyAIL_callback_data));

    callbackData->fileLikeObj = fileLikeObj;

    callbackData->readMethod = readMethod;
    callbackData->writeMethod = writeMethod;
    callbackData->tellMethod = tellMethod;
    callbackData->seekMethod = seekMethod;

    return PyCapsule_New(callbackData, NULL, pyail_callbackDataDestructor);
}

void pyail_imgCapsuleDestructor(PyObject* imgCapsule)
{
    printf("CLOSING IMAGE\n");
    AImgHandle img = PyCapsule_GetPointer(imgCapsule, NULL);
    AImgClose(img);
}

static PyObject* pyail_open(PyObject* self, PyObject* args)
{
    PyObject* callbackDataCapsule;
    if (!PyArg_ParseTuple(args, "O", &callbackDataCapsule))
        return NULL;

    PyAIL_callback_data* callbackData;
    if((callbackData = PyCapsule_GetPointer(callbackDataCapsule, NULL)) == NULL)
        return NULL;

    AImgHandle img;
    int32_t detectedFileFormat;
    AImgOpen(pyail_ReadCallback, pyail_TellCallback, pyail_SeekCallback, callbackData, &img, &detectedFileFormat);

    // return a tuple (imgCapsule, detectedFileFormat)
    PyObject* imgCapsule = PyCapsule_New(img, NULL, pyail_imgCapsuleDestructor);
    PyObject* retval = Py_BuildValue("(Oi)", imgCapsule, detectedFileFormat);
    Py_DECREF(imgCapsule);

    return retval;
}

static PyObject* pyail_getInfo(PyObject* self, PyObject* args)
{
    PyObject* capsule;
    if (!PyArg_ParseTuple(args, "O", &capsule))
        return NULL;

    AImgHandle* img;
    if((img = PyCapsule_GetPointer(capsule, NULL)) == NULL)
        return NULL;

    int32_t width, height, numChannels, bytesPerChannel, floatOrInt, decodedImgFormat;
    AImgGetInfo(img, &width, &height, &numChannels, &bytesPerChannel, &floatOrInt, &decodedImgFormat);

    return Py_BuildValue("(iiiiii)", width, height, numChannels, bytesPerChannel, floatOrInt, decodedImgFormat);
}

static PyObject* pyail_decode(PyObject* self, PyObject* args)
{
    PyObject* imgCapsule;
    PyObject* destObj;

    int forceImageFormat;
    if (!PyArg_ParseTuple(args, "OOi", &imgCapsule, &destObj, &forceImageFormat))
        return NULL;

    AImgHandle* img;
    if((img = PyCapsule_GetPointer(imgCapsule, NULL)) == NULL)
        return NULL;

    PyObject* destArrayObj;
    if((destArrayObj = PyArray_FROM_OTF(destObj, NPY_NOTYPE, 0)) == NULL)
        return NULL;

    void* dest = PyArray_DATA(destArrayObj);

    int err = AImgDecodeImage(img, dest, forceImageFormat);

    Py_DECREF(destArrayObj);

    return Py_BuildValue("i", err);
}

char* asd = "HELLLOOOOOOOO TESTTTT";

static PyObject* pyail_test_c_func(PyObject* self, PyObject* args)
{
    PyObject* capsule;
    if (!PyArg_ParseTuple(args,"O", &capsule))
        return NULL;

    PyAIL_callback_data* d;
    if((d = PyCapsule_GetPointer(capsule, NULL)) == NULL)
        return NULL;

    const char* msg = "HELLO FROM C";
    pyail_WriteCallback(d, msg, strlen(msg));
    /*char* buf = malloc(999);

    pyail_ReadCallback(d, buf, 10);

    buf[10] = 0;

    printf(buf);

    printf("XXXX %d XXXX\n", pyail_TellCallback(d));

    pyail_SeekCallback(d, 5);

    printf("XXXX %d XXXX\n", pyail_TellCallback(d));*/



    //return PyString_FromStringAndSize(asd, strlen(asd));
    /*PyObject* obj;

    if (!PyArg_ParseTuple(args,"O", &obj))
        return NULL;


    PyObject* read_meth;
    if ((read_meth = PyObject_GetAttrString(obj, "read")) == NULL)
       return NULL;

    PyObject* read_args = Py_BuildValue("(i)", 100);

    PyObject* data = PyObject_Call(read_meth, read_args, NULL);

    Py_DECREF(read_args);

    if(data == NULL)
        return NULL;

    char* bytes;
    Py_ssize_t len;

    PyBytes_AsStringAndSize(data, &bytes, &len);

    bytes[len-1] = 0;

    printf(bytes);

    Py_DECREF(data);*/



    /*PyObject* pycallback;


    if (!PyArg_ParseTuple(args,"s", &pycallback))
    {
        return NULL;
    }


    if (!PyCallable_Check(pycallback))
    {
        PyErr_SetString(PyExc_TypeError, "parameter must be callable");
        return NULL;
    }

    PyObject* arglist = Py_BuildValue("(i,i)", 1, 3);

    PyObject* result = PyEval_CallObject(pycallback, arglist);
    Py_DECREF(arglist);

    if(result == NULL)
        return NULL;

    Py_DECREF(result);*/

    Py_RETURN_NONE;
}



static PyMethodDef module_methods[] = {
    {"test", pyail_test_c_func, METH_VARARGS, NULL},
    {"getCallbackDataFromFileLikeObject", pyail_getCallbackDataFromFileLikeObject, METH_VARARGS, NULL},
    {"open", pyail_open, METH_VARARGS, NULL},
    {"getInfo", pyail_getInfo, METH_VARARGS, NULL},
    {"decode", pyail_decode, METH_VARARGS, NULL},

    {NULL, NULL, 0, NULL}
};

/* Initialize the module */
PyMODINIT_FUNC initail_py_native(void)
{
    PyObject *m = Py_InitModule3("ail_py_native", module_methods, NULL);
    if (m == NULL)
        return;

    /* Load `numpy` functionality. */
    import_array();

    AImgInitialise();
}
