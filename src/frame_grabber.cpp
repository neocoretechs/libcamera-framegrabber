#include <iostream>
#include <iomanip>
#include <memory>
#include <thread>
#include <chrono>
#include <libcamera/camera.h>
#include "frame_grabber.hpp"
#include <vector>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <mutex>

#include "buffer_mapper.cpp"
#include "blocking_deque.cpp"
using namespace libcamera;
using namespace std::chrono_literals;
std::vector<std::unique_ptr<Request>> requests;
BufferMapper bufferMapper;
BlockingDeque frameQueue(10); // Holds JPEG frames
static std::shared_ptr<Camera> camera;
FrameBufferAllocator *allocator = nullptr;
std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
std::thread captureThread;
bool running = true;
FrameGrabber::FrameGrabber() {
}
static void requestComplete(Request *request)
{
    const std::map<const Stream *, FrameBuffer *> &buffers = request->buffers();
    for (auto bufferPair : buffers) {
        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();
        int fd = buffer->planes()[0].fd.get();
        size_t size = metadata.planes()[0].bytesused;
        std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << " bytesused: " << size << std::endl;
        void *data = bufferMapper.map(fd,size);
        if(!data) {
            std::cerr << " Failed to map buffer fd=" << fd << std::endl;
            continue;
        }
        frameQueue.push(std::vector<uint8_t>((uint8_t *)data, (uint8_t *)data + size));
    }
}

int FrameGrabber::startCapture(std::string cameraId) {
    cm->start();
    for (auto const &cam : cm->cameras())
    std::cout << cam->id() << std::endl;
    auto cameras = cm->cameras();
    if (cameras.empty()) {
        std::cout << "No cameras were identified on the system." << std::endl;
        cm->stop();
        return EXIT_FAILURE;
    }
    // get camerId from Java
    //std::string cameraId = cameras[0]->id();
    camera = cm->get(cameraId);
    camera->acquire();
  
    std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration( { StreamRole::Viewfinder } );
    StreamConfiguration &streamConfig = config->at(0);
    std::cout << "Default viewfinder configuration is: " << streamConfig.toString() << std::endl;
    streamConfig.size.width = 640;
    streamConfig.size.height = 480;
    camera->configure(config.get());
    allocator = new FrameBufferAllocator(camera);
    for (StreamConfiguration &cfg : *config) {
        int ret = allocator->allocate(cfg.stream());
        if (ret < 0) {
            std::cerr << "Can't allocate buffers" << std::endl;
            return -ENOMEM;
        }
        size_t allocated = allocator->buffers(cfg.stream()).size();
        std::cout << "Allocated " << allocated << " buffers for stream" << std::endl;
    }
    Stream *stream = streamConfig.stream();
    const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);

    for (unsigned int i = 0; i < buffers.size(); ++i) {
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request){
            std::cerr << "Can't create request" << std::endl;
            return -ENOMEM;
        }
        const std::unique_ptr<FrameBuffer> &buffer = buffers[i];
        int ret = request->addBuffer(stream, buffer.get());
        if (ret < 0) {
            std::cerr << "Can't set buffer for request"<< std::endl;
            return ret;
        }
        requests.push_back(std::move(request));
 
    }
    camera->requestCompleted.connect(requestComplete);
    camera->start();
    captureThread = std::thread([this]() {
        while (running) {
            if (requests.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            for (std::unique_ptr<Request> &request : requests) {
                camera->queueRequest(request.get());
                request->reuse(Request::ReuseBuffers);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Prevent CPU hogging
            }
        }
    });
    return 0; 
}

std::vector<uint8_t> FrameGrabber::getJPEGFrame() {
    return frameQueue.pop();
}

void FrameGrabber::closeCamera() {
    running = false;
    if (captureThread.joinable()) captureThread.join();         
    while (!frameQueue.empty())
        frameQueue.pop();
    bufferMapper.cleanup();
    delete allocator;
    allocator = nullptr;
    if (camera) {
        camera->stop();
        camera->release();
        camera.reset();
    }
}
FrameGrabber::~FrameGrabber() {
    closeCamera(); // Ensure cleanup happens automatically
}
