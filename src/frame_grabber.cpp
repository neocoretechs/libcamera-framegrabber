#include <iostream>
#include <iomanip>
#include <memory>
#include <chrono>
#include <libcamera/camera.h>
#include "frame_grabber.hpp"
#include <vector>
#include <libcamera/camera_manager.h>
#include <libcamera/framebuffer_allocator.h>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <algorithm>
using namespace libcamera;
using namespace std::chrono_literals;

FrameGrabber::FrameGrabber(): frameQueue(10), cm(std::make_unique<CameraManager>()) {}

void FrameGrabber::requestComplete(libcamera::Request *request)
{
    if (request->status() == Request::RequestCancelled)
        return;
    const std::map<const Stream *, FrameBuffer *> &buffers = request->buffers();
    for (auto bufferPair : buffers) {
        FrameBuffer *buffer = bufferPair.second;
        const FrameMetadata &metadata = buffer->metadata();
        int fd = buffer->planes()[0].fd.get();
        size_t size = metadata.planes()[0].bytesused;
        std::cout << " seq: " << std::setw(6) << std::setfill('0') << metadata.sequence << " bytesused: " << size << std::endl;
        void *data = bufferMapper.get(fd);
        if(!data) {
            std::cerr << " Failed to get map buffer fd=" << fd << std::endl;
            continue;
        }
        std::cout << "Push frameQueue bytes from requestComplete:" << size << " seq: " << std::setw(6) << std::setfill('0') << buffer->metadata().sequence << std::endl;
        frameQueue.push(std::vector<uint8_t>((uint8_t *)data, (uint8_t *)data + size));
    }
    requestQueue.push(request);
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
    stream = streamConfig.stream();
    const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator->buffers(stream);

    for (unsigned int i = 0; i < buffers.size(); ++i) {
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request){
            std::cerr << "Can't create request" << std::endl;
            return -ENOMEM;
        }
        for (StreamConfiguration &cfg : *config) {
            Stream *stream = cfg.stream();
            const std::vector<std::unique_ptr<FrameBuffer>> &buffers =
                allocator->buffers(stream);
            const std::unique_ptr<FrameBuffer> &buffer = buffers[i];

            int ret = request->addBuffer(stream, buffer.get());
            if (ret < 0) {
                std::cerr << "Can't set buffer for request"
                      << std::endl;
                return ret;
            }
            for (const FrameBuffer::Plane &plane : buffer->planes()) {
                //void *memory = mmap(NULL, plane.length, PROT_READ, MAP_SHARED,
                //            plane.fd.get(), 0);
                //mappedBuffers_[plane.fd.get()] =
                //    std::make_pair(memory, plane.length);
                bufferMapper.map(plane.fd.get(), plane.length);
            }
        }
        requests.push_back(std::move(request));
    }
    camera->requestCompleted.connect(this, &FrameGrabber::requestComplete);
    int ret = camera->start();
    if (ret) {
        std::cout << "Failed to start capture" << std::endl;
        return ret;
    }
    for (std::unique_ptr<Request> &request : requests) {
        int ret = camera->queueRequest(request.get());
        //request->reuse(Request::ReuseBuffers);
        if (ret < 0) {
            std::cerr << "Can't queue request" << std::endl;
            camera->stop();
            return ret;
        }
    }
    return 0; 
}

std::vector<uint8_t> FrameGrabber::getJPEGFrame() {
     std::unique_lock<std::mutex> lock(mtx_);
    // int w, h, stride;
    if (!requestQueue.empty()) {
        Request *request = requestQueue.front();
        /*
        const Request::BufferMap &buffers = request->buffers();
        for (auto it = buffers.begin(); it != buffers.end(); ++it) {
            FrameBuffer *buffer = it->second;
            for (unsigned int i = 0; i < buffer->planes().size(); ++i) {
                const FrameBuffer::Plane &plane = buffer->planes()[i];
                const FrameMetadata::Plane &meta = buffer->metadata().planes()[i];   
                void *data = bufferMapper.get(plane.fd.get());
                int length = (std::min)(meta.bytesused, plane.length);
                std::cout << "Push frameQueue bytes from getJPEGFrame:" << length << " seq: " << std::setw(6) << std::setfill('0') << buffer->metadata().sequence << std::endl;
                frameQueue.push(std::vector<uint8_t>((uint8_t *)data, (uint8_t *)data + length));
            }
        }
        */
        requestQueue.pop();
        request->reuse(Request::ReuseBuffers);
        int ret = camera->queueRequest(request);
        //request->reuse(Request::ReuseBuffers);
        if (ret < 0) {
            std::cerr << "Can't queue request for re-use" << std::endl;
        }
    } 
    return frameQueue.pop();
}

void FrameGrabber::closeCamera() {
    running = false;
    if (camera) {     
        while (!frameQueue.empty())
            frameQueue.pop();
        while (!requestQueue.empty()) {
            requestQueue.pop();
            //std::cerr << "Popped outstanding request during teardown" << std::endl;
        }
        camera->requestCompleted.disconnect(this);
        bufferMapper.cleanup();
        camera->stop();
        allocator->free(stream);
        delete allocator;
        camera->release();
        camera.reset();
        cm->stop();
    }
}
FrameGrabber::~FrameGrabber() {
    closeCamera(); // Ensure cleanup happens automatically
}
