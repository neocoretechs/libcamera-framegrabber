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
BufferMapper bufferMapper;
std::mutex mtx;
std::vector<uint8_t> sharedFrame;
BlockingDeque frameQueue(10); // Holds JPEG frames
static std::shared_ptr<Camera> camera;
FrameBufferAllocator *allocator = nullptr;
int startCapture(std::string cameraId) {
    std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
    cm->start();
    for (auto const &cam : cm->cameras())
    std::cout << cam->id() << std::endl;
    auto cameras = cm->cameras();
    if (cameras.empty()) {
        std::cout << "No cameras were identified on the system."
              << std::endl;
        cm->stop();
        return EXIT_FAILURE;
    }
    // get camerId from Java
    std::string cameraId = cameras[0]->id();
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
    std::vector<std::unique_ptr<Request>> requests;
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
        if (request->status() == Request::RequestCancelled)
            return;        
    }
    camera->requestCompleted.connect(requestComplete);
    camera->start();
    for (std::unique_ptr<Request> &request : requests) {
        for(;;) {
            for (const std::unique_ptr<FrameBuffer> &buffer : buffers) {
                for (const FrameBuffer::Plane &plane : buffer->planes()) {
                    int fd = plane.fd.get();
                    size_t length = plane.length;
                    void *data = bufferMapper.map(fd, length);
                    if (!data) {
                        std::cerr << "Failed to map buffer plane\n";
                        continue;
                    }
                }
            }
            camera->queueRequest(request.get());
            request->reuse(Request::ReuseBuffers); 
        }
    }
}
static void requestComplete(Request *request)
{
    const std::map<const Stream *, FrameBuffer *> &buffers = request->buffers();
    for (auto bufferPair : buffers) {
        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();
        std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << " bytesused: ";
        unsigned int nplane = 0;
        for (const FrameMetadata::Plane &plane : metadata.planes()){
            std::cout << plane.bytesused;
            if (++nplane < metadata.planes().size()) std::cout << "/";
        }
        int fd = buffer->planes()[0].fd.get();
        size_t size = metadata.planes()[0].bytesused;
        void *data = bufferMapper.get(fd);
        std::unique_lock<std::mutex> lock(mtx);
        sharedFrame.assign((uint8_t *)data, (uint8_t *)data + size);
        frameQueue.push(sharedFrame);
    }
}

std::vector<uint8_t> getJPEGFrame() {
    return frameQueue.pop();
}

void closeCamera() {           
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
