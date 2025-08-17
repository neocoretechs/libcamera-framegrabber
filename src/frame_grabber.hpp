#ifndef FRAME_GRABBER_HPP
#define FRAME_GRABBER_HPP

#include <vector>
#include <cstdint>
#include <string>
#include <iostream>
#include <iomanip>
#include <memory>
#include <chrono>
#include <libcamera/camera.h>
#include <vector>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <algorithm>
#include "buffer_mapper.cpp"
#include "blocking_deque.cpp"

class FrameGrabber {
public:
    FrameGrabber(libcamera::CameraManager* cm);
    ~FrameGrabber();

    int startCapture(std::string cameraId);
    std::vector<uint8_t> getJPEGFrame();
    void closeCamera();
    void requestComplete(libcamera::Request *request);

private:
    std::mutex mtx_;
    std::vector<std::unique_ptr<libcamera::Request>> requests;
    std::queue<libcamera::Request *> requestQueue;
    BufferMapper bufferMapper;
    BlockingDeque frameQueue;
    std::shared_ptr<libcamera::Camera> camera;
    libcamera::Stream *stream = nullptr;
    libcamera::FrameBufferAllocator *allocator = nullptr;
    std::unique_ptr<libcamera::CameraManager> cm;
    bool running = true;
};

#endif // FRAME_GRABBER_HPP