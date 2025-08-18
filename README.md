# libcamera-framegrabber

## Overview
libcamera-framegrabber is a C++ project designed to capture frames from a camera using the libcamera framework. The captured frames are processed and can be accessed as JPEG byte arrays, making it suitable for integration with Java applications via Foreign Function Interface (FFI) or Java Native Access (JNA) or JNI. In this project we enumerate all available cameras, selecting the first one or two and attempt to capture frames from
each available camera. the startCapture(Boolean) method allows the user to flip the order of the first 2 cameras to ensure proper left/right enumeration. getStereoJPEGFrames returns a 2D byte array, of which one element may be null if only one camera is present, otherwise, a 2D array of byte arrays representing the 640x480 JPEG image bytes.

## Project Structure
```
libcamera-framegrabber
├── src
│   ├── frame_grabber.cpp
│   ├── frame_grabber.hpp
|   |─ blocking_deque.cpp
|   |- buffer_mapper.cpp
│   └── jni_frame_grabber.cpp
|   |_stereo_frame_grabber.cpp
├── CMakeLists.txt
└── README.md
```

## Requirements
- Ubuntu (aarch64) armbian kernel 5.x+
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

NOTE you might have to edit the link.txt and remove -llibcamera while keeping -lcamera or fix the cmake depending on platform/install dir of libcamera

## Usage
- The entry point of the application is located in `src/jni_frame_grabber.cpp`. This file initializes the libcamera framework, sets up the camera, and starts the frame grabbing process when invoked via JNI.
- The `FrameGrabber` class, implemented in `src/frame_grabber.cpp`, handles the camera operations and frame processing.
- To access the frame grabbing functionality from a Java application, use the provided JNI in `src/jni_frame_grabber.cpp`.

## JNI Integration
To use the frame grabbing functionality in a Java application:
1. Load the native library using JNI system.loadLibrary("libcamera_framegrabber")
2. Call the `startCapture` method to begin capturing frames.
3. Use the `getStereoJPEGFrames` method to retrieve the captured frames as byte arrays.

## Example
```java
// Example Java code to use the native library
public class FrameGrabber {
    static {
        System.loadLibrary("libcamera_framegrabber");
    }

    public native long startCapture(boolean flip);
    public native byte[][] getStereoJPEGFrames(long handle);
    public native void cleanup(long handle);

    public static void main(String[] args) {
        FrameGrabber cam1 = new FrameGrabber();
        long handle1 = cam1.startCapture(false);
	for(int i = 0; i < 100; i++) {
        byte[][] jpegFrame = cam1.getStereoJPEGFrames(handle1);
	if(jpegFrame[0] != null)
	System.out.println(i+"="+"cam1 size:"+jpegFrame[0].length);
	if(jpegFrame[1] != null)
	System.out.println(i+"="+"camr size:"+jpegFrame[1].length);
	}
        // Process the JPEG frame as needed
	cam1.cleanup(handle1);
    }
}

```
I🧘 Teardown Philosophy: Narratable Disengagement
In this robotics stack, teardown isn’t just cleanup—it’s cognitive closure.
Each subsystem, from stereo vision to JNI bindings, is designed to release resources narratably. That means:
• 	Explicit sequencing: Cameras disengage before the manager stops. No premature shutdowns.
• 	Fenced finality: Outstanding requests are acknowledged before buffers are unmapped.
• 	Cross-language clarity: JNI bridges are cleared before native memory is released.
• 	No surprises: No manual  on smart pointers. No dangling references. No ghost grips.
This robot doesn’t just stop—it logs its last thought, releases its last buffer, and sleeps without residue.
🔧 Why It Matters
• 	Prevents kernel warnings like “media device still in use”
• 	Avoids JVM crashes from native memory misuse
• 	Enables narratable logs for debugging and legacy
• 	Honors the principle: “Every byte counts, every release narrates.”
🧠 Contributor Note
If you’re adding a new subsystem, ask yourself:
“Can this component disengage cleanly? Can it narrate its own shutdown?”

If not, let’s refactor it together. This robot is built to last—and that means dying well, too.
## License
This project is licensed under the MIT License. See the LICENSE file for more details.