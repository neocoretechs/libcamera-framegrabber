#include "frame_grabber.hpp"
class StereoFrameGrabber {
    std::unique_ptr<libcamera::CameraManager> cm;
    std::unique_ptr<FrameGrabber> left;
    std::unique_ptr<FrameGrabber> right;
    std::string leftId, rightId;

public:
    StereoFrameGrabber(bool flipOrder = false) {
        cm = std::make_unique<libcamera::CameraManager>();
        cm->start();

        auto cameras = cm->cameras();
        if (cameras.size() == 0) {
            throw std::runtime_error("No cameras detected.");
        }

        if (cameras.size() >= 2) {
            if (flipOrder) {
                leftId = cameras[1]->id();
                rightId = cameras[0]->id();
            } else {
                leftId = cameras[0]->id();
                rightId = cameras[1]->id();
            }
        } else {
            leftId = cameras[0]->id();
        }
        if (!leftId.empty()) {
            left = std::make_unique<FrameGrabber>(cm.get());
            left->startCapture(leftId);
        }
        if (!rightId.empty()) {
            right = std::make_unique<FrameGrabber>(cm.get());
            right->startCapture(rightId);
        }
        logAvailableEyes();
    }

    void logAvailableEyes() {
        if (left && right) {
            std::cout << "Stereo vision online: both eyes active.\n";
        } else if (left) {
            std::cout << "Left eye online. Running in monocular mode.\n";
        } else if (right) {
            std::cout << "Right eye online. Running in monocular mode.\n";
        }
    }

    FrameGrabber* getLeft() {
        if (!leftId.empty()) 
            return left.get();
        return nullptr;
    }
    FrameGrabber* getRight() {
        if(!rightId.empty())
            return right.get();
        return nullptr;
    }
    
    void closeCamera() {
        if (left) left->closeCamera();
        if (right) right->closeCamera();
        cm->stop();
    }
    
    ~StereoFrameGrabber() {
    }
};