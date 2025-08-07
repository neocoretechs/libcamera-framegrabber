project(libcamera-framegrabber)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_library(frame_grabber src/frame_grabber.cpp)
target_link_libraries(frame_grabber PRIVATE libcamera)

add_executable(libcamera_framegrabber src/jni_frame_grabber.cpp)
target_link_libraries(libcamera_framegrabber PRIVATE frame_grabber)