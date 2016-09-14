CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#include_directories($ENV{CLI_INCLUDE_DIRS})
#link_directories($ENV{CLI_LINK_DIRS})

PROJECT (ArtomatixImageLoaders)

add_library(ArtomatixImageLoader SHARED
	exr.cpp
    exr.h
    AIL.cpp
    AIL.h
    AIL_internal.h
    ImageLoaderBase.h
)

if(UNIX)
    set_target_properties(ArtomatixImageLoader PROPERTIES COMPILE_FLAGS "-Wall -pedantic -Wextra -g -std=c++11 -Wno-unknown-pragmas")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#find_package(OpenCV REQUIRED)
#find_package(TIFF REQUIRED)
find_package(OpenEXR REQUIRED)

target_link_libraries(ArtomatixImageLoader
	#${OpenCV_LIBS} 
	#${TIFF_LIBRARIES}
	${OPENEXR_LIBRARIES}
)


#message("qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq 	${OPENCV_LIBS} ${TIFF_INCLUDE_DIR} ${OPENEXR_INCLUDE_DIRS}")

include_directories(
	#${OpenCV_INCLUDE_DIRS}
	#${TIFF_INCLUDE_DIR}
	${OPENEXR_INCLUDE_DIRS}
)

install (TARGETS ArtomatixImageLoader
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
         RUNTIME DESTINATION lib)

add_subdirectory(tests)
