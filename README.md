# Atlas Imager

A C++ program to render Rat Brain images and align them with LGN or PAG Atlases. 

## Dependencies

- CMake version 3.21 or greater
- A compiler with C++23 support
- A Qt version greater than 6.8
- OpenCV
- The Atlas Model images
- Rat Brain Images

## The Qt Application

### Building the Qt Application

OpenCV needs to be compiled separately, installed, and copied into `thirdparty/opencv`. As of March 3rd, 2026, the OpenCV Version being compiled is 4.13.0. In another folder outside of this repository do the following: 

- `git clone -b 4.13.0 <OpenCV Github Link> --single-branch`
- Navigate inside the opencv directory
- run: `cmake -B build -S . -DCMAKE_INSTALL_PREFIX=install`
- after configuration run: `cmake --build build --config Release --target install --parallel 8`
- After it builds, you should see a folder in opencv called, `install`. Copy the contents inside of install into thirdparty/opencv exactly how the install folder looks. 

The rest of the instructions is assuming that the developer has installed Qt in `C:\Qt` for Windows and `/opt/Qt` for Linux. It is currently unknown where OSX installs Qt. Run the follwing in a terminal:

- `cmake -B build -S . -DCMAKE_PREFIX_PATH=<Path to Qt installation>/6.x.x/<compiler version>`
- after configuration run: `cmake --build build`

### Launching the Qt Application

The application should be located inside of the build folder as `build/AtlasImager/AtlasImager`. This will have a `.exe` extension if it was built in Windows. Ensure the Dataset folder is populated with the images before launching. LGN and PAG images should be in their own directories. 

## The WebAssmebly Application

### Building the WebAssembly Application

This is a lot more difficult and requires a lot more hands on development. A few requirements before beginning:

- A Python version greater than 3.12
- The emsdk
- An OpenCV compiled with emsdk

It is left to the developer on how to obtain the appropriate python version and emsdk. However, the emsdk is readily available online and has some very easy instructions on how to obtain and set it up. However, the user should be warned that compiling wasm is much easier done using Linux than on Windows. 

Once the user can run `emcc --version` in their terminal. The following should be done with OpenCV: 

- Open CMakeLists.txt in `opencv/modules/js/CMakeLists.txt` and search for: `DEMANGLE_SUPPORT=1` and comment out the line that it uses. 
- run `emcmake python3 ./opencv/platforms/js/build_js.py build_js --build_loader --cmake_option="-DCMAKE_CXX_STANDARD=17"`
- copy the contents of `build_js` into `thirdparty/opencv` exactly how it is populated. 
- navigate back to the Atlas Imager directory and run: `emcmake cmake -B build-wasm -S . -DAS_WASM=ON`
- after configuration run: `cmake --build build-wasm`

### Running the WebAssembly Application

Copy the AtlasImager.js and AtlasImager.wasm files into the appropriate web application and launch a web browser. 