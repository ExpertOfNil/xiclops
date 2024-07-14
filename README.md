# Xiclops - C

A single camera test sandbox that uses only the Ximea and Raylib libraries.
The default camera is camera 0, but you can pass in a camera ID as a command
line argument if you want to use a different camera.

## Command Line Options

- `-h` displays the help text and exits
- `-c` is the flag for camera ID choice (`int`, default = 0)
- `-v` is the flag for verbosity level (`int`, default = 2 a.k.a `INFO`)
- `-z` is the flag for zoom (new / original) (`float`, default = 1.0)

## Multi-Camera

The `multi-camera` branch uses the single camera foundation, but uses threaded (1 thread per camera)
acquisition of images and renders (currently up to 4) images to a single window.

### Observations

- 27 FPS rendering a single acquired image to 4 quadrants of the window
- 27 FPS rendering 4 separate camera feeds to 4 separate windows via 4 separate app instances
- 23 FPS rendering 4 camera feeds to 4 quadrants of the window
    - 5 threads total, 4 acquisition threads
    - Mutexes over the returned `XI_IMG` don't seem to be needed
    - Transport of images (4 x 3840 x 2160) to GPU seems to be the bottleneck
