project(libcamera-framegrabber)
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)

find_package(JNI REQUIRED)
include_directories(include ${JNI_INCLUDE_DIRS})

add_library(frame_grabber SHARED src/frame_grabber.cpp)
target_link_libraries(frame_grabber PRIVATE camera)

add_library(libcamera_framegrabber SHARED src/jni_frame_grabber.cpp)
target_link_libraries(libcamera_framegrabber PRIVATE frame_grabber camera)