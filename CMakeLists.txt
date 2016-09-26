CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT (AIL)

add_library(AIL SHARED
	exr.cpp
    exr.h
    png.h
    png.cpp
    AIL.cpp
    AIL.h
    AIL_internal.h
    ImageLoaderBase.h
)

add_library(AIL_py SHARED python.c)
set_target_properties(AIL_py PROPERTIES OUTPUT_NAME "ail_py_native" PREFIX "")

set(AIL_COMPILE_FLAGS "")
if(UNIX)
    set(AIL_COMPILE_FLAGS "-Wall -pedantic -Wextra -g -std=c++11 -Wno-unknown-pragmas")
endif()

set_target_properties(AIL PROPERTIES COMPILE_FLAGS "${AIL_COMPILE_FLAGS}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(OpenEXR REQUIRED)
find_package(PythonLibs 2 REQUIRED)
find_package(NumPy REQUIRED)
find_package(PNG REQUIRED)

target_link_libraries(AIL_py
    AIL
    ${PYTHON_LIBRARIES})

target_link_libraries(AIL
	${OPENEXR_LIBRARIES}
    ${PNG_LIBRARIES}
)

include_directories(
	${OPENEXR_INCLUDE_DIRS}
        ${PYTHON_INCLUDE_DIR}
)

install (TARGETS AIL
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
         RUNTIME DESTINATION lib)

add_subdirectory(tests)
