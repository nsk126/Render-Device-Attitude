# Render Orientation

This is a tool I built to render 3D orientation of a IMU device that sends attitude and heading readings through a serial port.

## Depndencies

The tool depends on SDL2 and OpenGL.

`sudo apt-get install libsdl2-dev libglfw3-dev libglew-dev libglm-dev`

## Build

To build the file,

1. Make a new folder in the project directory called **build**.
    - `mkdir build`
    - `cd build`
    - `cmake ..`
    - `make`
 
## Usage

- Any Attitude & Heading data send over serial needs to follow the format `"pitch,roll,yaw\r"`. All values can be of floating point but they need a _comma_(,) delimiter.
- Fix the serial baudrate to `115200`.
- By default the serial port initiated in the program is `/dev/ttyACM0`. However, you can change the port in the script.
- You can render a simple 2D representation of the orientation using SDL2 or a 3D representation using GLFW GLEW GLM(OpenGL).
- If you are using OpenGL, remember to keep the shader files in your project directory.

## Tested On

1. Ubuntu 18.04 LTS
