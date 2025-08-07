#ifndef FRAME_GRABBER_HPP
#define FRAME_GRABBER_HPP

#include <vector>
#include <string>

class FrameGrabber {
public:
    FrameGrabber();
    ~FrameGrabber();

    int startCapture(std::string cameraId);
    std::vector<uint8_t> getJPEGFrame();

private:

};

#endif // FRAME_GRABBER_HPP