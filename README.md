# Render Orientation

This is a tool i built to render 3D orientation of a IMU device that sents attitude and heading readings through a serial port.

## Depndencies

The tool depends on SDL2 and OpenGL.

`sudo apt-get install libsdl2-dev`

## Build

To build the file,

1. Make a new folder in the project directory called **build**.
    - `mkdir build`
2. Open bash in the build directory and execute the following commands.
    - `cmake ..`
    - `make`