# Xiclops - C

A single camera test sandbox that uses only the Ximea and Raylib libraries.
The default camera is camera 0, but you can pass in a camera ID as a command
line argument if you want to use a different camera.

## Command Line Options

- `-h` displays the help text and exits
- `-c` is the flag for camera ID choice (`int`, default = 0)
- `-v` is the flag for verbosity level (`int`, default = 2 a.k.a `INFO`)
- `-z` is the flag for zoom (new / original) (`float`, default = 1.0)
