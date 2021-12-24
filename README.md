# Digital Picture Frame

Originally intended to display shaders from [Shadertoy](https://www.shadertoy.com), on the [Raspberry Pi](#raspberry-pi), this repo has been modified to display pictures in sequence from a file. Made to be used in a digital picture frame

It should work with any GPU and display controller hardware, provided a DRM/KMS driver is available.

## Features
1. Loads pictures in sequence from /home/pi/Projects/Pictures/
2. (TODO) Error handling when loading pictures (right now it will just crash if it's not a valid image file)
3. Robust size correction, adding black bars as necessary to fit the picture to the screen without changing aspect ratio
4. If OpenGL fails to load a picture it will move to the next one in the sequence (this can happen unexpectedly for larger images due to the hardware limitations. Even for pictures within the designated max image size, OpenGL was unable to load larger pictures)
5. (TODO) Fading between displaying each picture

## Credits

The DRM/KMS ceremony code is copied from [kmscube](https://gitlab.freedesktop.org/mesa/kmscube/).

The original [kms-glsl repo](https://github.com/astefanutti/kms-glsl) written by astefanutti
