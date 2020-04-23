cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)

project(webcam)

set(LIB_SRCS 
    log.cxx
    webcam_v4l2.cxx)

set(SRCS 
    main.cxx)

include_directories(${PROJECT_SOURCE_DIR})

add_library(${PROJECT_NAME} STATIC ${LIB_SRCS})

add_executable(cap ${SRCS})
target_link_libraries(cap ${PROJECT_NAME})
