# Atlas Imager

This is an open source program made with Qt and C++. This program utilizes OpenCV to do image matching to try and find the best matching images with the atlas provided. 

## Dependencies

- CMake version 3.21 or greater
- A compiler with C++23 support
- A Qt version greater than 6.8
- OpenCV
- The Atlas Model images
- Rat Brain Images

## Buiilding

With the proper dependencies installed simply run the following in a terminal:

```
cmake -B build -S .
cmake --build build
```