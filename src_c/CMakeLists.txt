CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

include("cmake/HunterGate.cmake")
HunterGate(
    URL "https://github.com/Artomatix/hunter/archive/OpenEXR-test-2.tar.gz"
    SHA1 "c86e9e6fde6b96fc57ac2184692baeb4d71e9fd6"
    LOCAL
)

PROJECT (AIL)

add_library(AIL SHARED
	exr.cpp
    exr.h
    png.h
    png.cpp
    jpeg.h
    jpeg.cpp
    tga.h
    tga.cpp
    AIL.cpp
    AIL.h
    AIL_internal.h
    ImageLoaderBase.h
    extern/stb_image.h
    extern/stb_image_write.h
)

set(AIL_COMPILE_FLAGS "")
if(UNIX)
    set(AIL_COMPILE_FLAGS "-Wall -pedantic -Wextra -g -std=c++11 -Wno-unknown-pragmas")
endif()
target_compile_definitions(AIL PRIVATE -DIS_AIL_COMPILE)

set_target_properties(AIL PROPERTIES COMPILE_FLAGS "${AIL_COMPILE_FLAGS}")

install (TARGETS AIL
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
         RUNTIME DESTINATION lib)


###########
# Options #
###########

set(PYTHON_ENABLED ON CACHE BOOL "enable python binding")
set(EXR_ENABLED ON CACHE BOOL "enable loading EXR files")
set(PNG_ENABLED ON CACHE BOOL "enable loading PNG files")
set(JPEG_ENABLED ON CACHE BOOL "enable loading JPEG files")
set(TGA_ENABLED ON CACHE BOOL "enable loading TGA files")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(PYTHON_ENABLED)
	add_library(AIL_py SHARED python.c)
	set_target_properties(AIL_py PROPERTIES OUTPUT_NAME "ail_py_native" PREFIX "")

	find_package(PythonLibs 2 REQUIRED)
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


#########
# Tests #
#########
add_subdirectory(tests)
