#include "frame_grabber.hpp"
#include <jni.h>
#include "blocking_deque.cpp" 

extern "C" {

JNIEXPORT jlong JNICALL Java_FrameGrabber_startCapture(JNIEnv *env, jobject obj, jstring jname) {
    FrameGrabber* grabber = new FrameGrabber();
    const char* nameChars = env->GetStringUTFChars(jname, nullptr);
    std::string cameraName = std::string(nameChars);
    grabber->startCapture(cameraName);
    env->ReleaseStringUTFChars(jname, nameChars);
    return reinterpret_cast<jlong>(grabber);
}

JNIEXPORT jbyteArray JNICALL Java_FrameGrabber_getJPEGFrame(JNIEnv* env, jobject obj, jlong handle) {
    FrameGrabber* grabber = reinterpret_cast<FrameGrabber*>(handle);
    // Block until a frame is available
    std::vector<uint8_t> frame = grabber->getJPEGFrame();
    // Create a new Java byte array
    jbyteArray result = env->NewByteArray(static_cast<jsize>(frame.size()));
    if (result == nullptr) return nullptr; // OutOfMemoryError
    // Copy frame data into Java array
    env->SetByteArrayRegion(result, 0, static_cast<jsize>(frame.size()), reinterpret_cast<jbyte*>(frame.data()));
    return result;
}

JNIEXPORT void JNICALL Java_FrameGrabber_cleanup(JNIEnv *env, jobject obj, jlong handle) {
    FrameGrabber* grabber = reinterpret_cast<FrameGrabber*>(handle);
    if (grabber) {
        delete grabber;
        grabber = nullptr;
    }
}
}