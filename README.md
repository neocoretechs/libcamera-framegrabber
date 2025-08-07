# libcamera-framegrabber

## Overview
libcamera-framegrabber is a C++ project designed to capture frames from a camera using the libcamera framework. The captured frames are processed and can be accessed as JPEG byte arrays, making it suitable for integration with Java applications via Foreign Function Interface (FFI) or Java Native Access (JNA) or JNI.

## Project Structure
```
libcamera-framegrabber
├── src
│   ├── frame_grabber.cpp
│   ├── frame_grabber.hpp
|   |─ blocking_deque.cpp
|   |- buffer_mapper.cpp
│   └── jni_frame_grabber.cpp
├── include
│   └── frame_grabber.hpp
├── CMakeLists.txt
└── README.md
```

## Requirements
- Ubuntu (aarch64)
- C++17 or later
- libcamera library

## Setup Instructions
1. **Install Dependencies**: Ensure that you have the necessary dependencies installed, including libcamera and CMake.
   ```bash
   sudo apt-get install libcamera-dev cmake
   ```

2. **Clone the Repository**: Clone the project repository to your local machine.
   ```bash
   git clone <repository-url>
   cd libcamera-framegrabber
   ```

3. **Build the Project**:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```

## Usage
- The entry point of the application is located in `src/main.cpp`. This file initializes the libcamera framework, sets up the camera, and starts the frame grabbing process.
- The `FrameGrabber` class, implemented in `src/frame_grabber.cpp`, handles the camera operations and frame processing.
- To access the frame grabbing functionality from a Java application, use the provided FFI in `src/ffi_interface.cpp`.

## FFI/JNA Integration
To use the frame grabbing functionality in a Java application:
1. Load the native library using JNA.
2. Call the `startCapture` method to begin capturing frames.
3. Use the `getJPEGFrame` method to retrieve the captured frames as byte arrays.

## Example
```java
// Example Java code to use the native library
public class FrameGrabberExample {
    static {
        System.loadLibrary("libcamera_framegrabber");
    }

    public native void startCapture();
    public native byte[] getJPEGFrame();

    public static void main(String[] args) {
        FrameGrabberExample example = new FrameGrabberExample();
        example.startCapture();
        byte[] jpegFrame = example.getJPEGFrame();
        // Process the JPEG frame as needed
    }
}
```

## License
This project is licensed under the MIT License. See the LICENSE file for more details.