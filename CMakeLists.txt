CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT (AIL)

add_library(AIL SHARED
	exr.cpp
    exr.h
    png.h
    png.cpp
    jpeg.h
    jpeg.cpp
    AIL.cpp
    AIL.h
    AIL_internal.h
    ImageLoaderBase.h
)

set(AIL_COMPILE_FLAGS "")

if(UNIX)
    set(AIL_COMPILE_FLAGS "-Wall -pedantic -Wextra -g -std=c++11 -Wno-unknown-pragmas")
endif()

set_target_properties(AIL PROPERTIES COMPILE_FLAGS "${AIL_COMPILE_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(OpenEXR REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)

target_link_libraries(AIL
	${OPENEXR_LIBRARIES}
    ${PNG_LIBRARIES}
    ${JPEG_LIBRARIES}
)

include_directories(
	${OPENEXR_INCLUDE_DIRS}
)

install (TARGETS AIL
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
         RUNTIME DESTINATION lib)

add_subdirectory(tests)
