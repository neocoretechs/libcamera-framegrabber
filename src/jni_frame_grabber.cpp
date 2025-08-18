#include "frame_grabber.hpp"
#include "stereo_frame_grabber.cpp"
#include <jni.h>

extern "C" {

JNIEXPORT jlong JNICALL Java_com_neocoretechs_robocore_video_FrameGrabber_startCapture(JNIEnv *env, jobject obj, jboolean jflip) {
    StereoFrameGrabber* grabber = new StereoFrameGrabber(jflip);
    return reinterpret_cast<jlong>(grabber);
}

JNIEXPORT jobjectArray JNICALL Java_com_neocoretechs_robocore_video_FrameGrabber_getStereoJPEGFrames(JNIEnv* env, jobject obj, jlong handle) {
    StereoFrameGrabber* grabber = reinterpret_cast<StereoFrameGrabber*>(handle);
    if (!grabber) return nullptr;
    // Create a Java byte[][] array with 2 elements
    jclass byteArrayClass = env->FindClass("[B");
    if (!byteArrayClass) return nullptr;
    jobjectArray stereoArray = env->NewObjectArray(2, byteArrayClass, nullptr);
    if (!stereoArray) return nullptr;
    // LEFT eye
    FrameGrabber* left = grabber->getLeft();
    if (left) {
        std::vector<uint8_t> jpegL = left->getJPEGFrame();
        if (!jpegL.empty()) {
            jbyteArray arrayL = env->NewByteArray(static_cast<jsize>(jpegL.size()));
            if (arrayL) {
                env->SetByteArrayRegion(arrayL, 0, static_cast<jsize>(jpegL.size()), reinterpret_cast<jbyte*>(jpegL.data()));
                env->SetObjectArrayElement(stereoArray, 0, arrayL);
            }
        }
    }
    // RIGHT eye
    FrameGrabber* right = grabber->getRight();
    if (right) {
        std::vector<uint8_t> jpegR = right->getJPEGFrame();
        if (!jpegR.empty()) {
            jbyteArray arrayR = env->NewByteArray(static_cast<jsize>(jpegR.size()));
            if (arrayR) {
                env->SetByteArrayRegion(arrayR, 0, static_cast<jsize>(jpegR.size()), reinterpret_cast<jbyte*>(jpegR.data()));
                env->SetObjectArrayElement(stereoArray, 1, arrayR);
            }
        }
    }
    return stereoArray;
}

JNIEXPORT void JNICALL Java_com_neocoretechs_robocore_video_FrameGrabber_cleanup(JNIEnv *env, jobject obj, jlong handle) {
    StereoFrameGrabber* grabber = reinterpret_cast<StereoFrameGrabber*>(handle);
    grabber->closeCamera();
}
}