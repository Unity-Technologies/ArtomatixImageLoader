CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/Artomatix/hunter/archive/v43-OpenEXR.tar.gz"
    SHA1 "faefc63600c5e431a59d406bbd0db743a659f1a9"
    LOCAL
)

PROJECT (AIL)

###########
# Options #
###########

set(TESTS_ENABLED ON CACHE BOOL "enable tests")
set(PYTHON_ENABLED OFF CACHE BOOL "enable python binding")
set(EXR_ENABLED ON CACHE BOOL "enable loading EXR files")
set(PNG_ENABLED ON CACHE BOOL "enable loading PNG files")
set(JPEG_ENABLED ON CACHE BOOL "enable loading JPEG files")
set(TIFF_ENABLED ON CACHE BOOL "enable loading TIFF files")
set(TGA_ENABLED ON CACHE BOOL "enable loading TGA files")
set(HDR_ENABLED ON CACHE BOOL "enable loading HDR files")

set(BUILD_SHARE_TYPE SHARED CACHE STRING "set build type, valid values: SHARED|STATIC")

set(CMAKE_POSITION_INDEPENDENT_CODE YES)

add_library(AIL ${BUILD_SHARE_TYPE}
    exr.cpp exr.h
    png.h png.cpp
    jpeg.h jpeg.cpp
    tga.h tga.cpp
    tiff.h tiff.cpp
    AIL.h AIL.cpp
    hdr.h hdr.cpp
    IExifHandler.hpp
    JpegExifHandler.hpp JpegExifHandler.cpp
    AIL_internal.h
    ImageLoaderBase.h
    extern/stb_image.h
    extern/stb_image_write.h
)


if(WIN32)
    add_definitions(-DNOMINMAX) # sigh... https://stackoverflow.com/a/2789509
endif()

set(AIL_COMPILE_FLAGS "")
if(UNIX)
    set(AIL_COMPILE_FLAGS "-Wall -pedantic -Wextra -g -std=c++11 -Wno-unknown-pragmas")
    set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--exclude-libs,ALL") # we don't want to export symbols for libpng, etc
endif()
target_compile_definitions(AIL PRIVATE -DIS_AIL_COMPILE)

set_target_properties(AIL PROPERTIES COMPILE_FLAGS "${AIL_COMPILE_FLAGS}" DEBUG_POSTFIX "")

install (TARGETS AIL
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
         RUNTIME DESTINATION lib)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(PYTHON_ENABLED)
    add_library(AIL_py SHARED python.c)
    set_target_properties(AIL_py PROPERTIES OUTPUT_NAME "ail_py_native" PREFIX "")

    find_package(PythonInterp 2 REQUIRED)
    find_package(PythonLibs ${PYTHON_VERSION_STRING} REQUIRED)
    find_package(NumPy REQUIRED)

    target_link_libraries(AIL_py
        AIL
        ${PYTHON_LIBRARIES})
    include_directories(${PYTHON_INCLUDE_DIR} ${PYTHON_NUMPY_INCLUDE_DIR})
endif()

if(EXR_ENABLED)
    hunter_add_package(OpenEXR)
    find_package(OpenEXR REQUIRED)
    target_link_libraries(AIL ${OPENEXR_LIBRARIES})
    include_directories(${OPENEXR_INCLUDE_DIRS})
    add_definitions(-DHAVE_EXR)
endif()

if(PNG_ENABLED)
    hunter_add_package(PNG)
    find_package(PNG CONFIG REQUIRED)
    target_link_libraries(AIL PNG::png)
    add_definitions(-DHAVE_PNG)
endif()

if(JPEG_ENABLED)
    hunter_add_package(Jpeg)
    find_package(JPEG CONFIG REQUIRED)
    target_link_libraries(AIL JPEG::jpeg)
    add_definitions(-DHAVE_JPEG)
endif()

if(TGA_ENABLED)
    add_definitions(-DHAVE_TGA)
endif()

if(TIFF_ENABLED)
    hunter_add_package(TIFF)
    find_package(TIFF CONFIG REQUIRED)
    target_link_libraries(AIL TIFF::libtiff)
    add_definitions(-DHAVE_TIFF)
endif()

if (HDR_ENABLED)
    add_definitions(-DHAVE_HDR)
endif()

SET(CMAKE_DEBUG_POSTFIX "")

#########
# Tests #
#########
if(TESTS_ENABLED)
    add_subdirectory(tests)
endif()
