cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)

project(webcam)

set(WEBCAM_LIB_SRCS 
    src/log.cxx
    src/webcam_v4l2.cxx)

set(TRANSFORM_LIB_SRCS
    src/jpeg_transform.cxx)

include_directories(${PROJECT_SOURCE_DIR}/include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)

add_library(${PROJECT_NAME} STATIC ${WEBCAM_LIB_SRCS})
add_library(jpegtrans STATIC ${TRANSFORM_LIB_SRCS})

add_executable(cap test/main.cxx)
target_link_libraries(cap ${PROJECT_NAME} jpegtrans turbojpeg)

add_executable(cap_yuv test/main_yuv.cxx)
target_link_libraries(cap_yuv ${PROJECT_NAME})
