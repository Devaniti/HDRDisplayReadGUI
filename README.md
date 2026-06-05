# HDR Measurement GUI

HDR Measurement GUI is an app that allows you to measure HDR displays with ArgyllCMS

You specify list of patches to measure, and HDR Display GUI guides you on how to setup your intsrument and runs the measurements using its own signal generator and ArgyllCMS's spotread to take measurements

Each patch specifies color to output in HDR10 format, as well as window size for the patch

# Usage

- Make sure that latest [Visual C++ v14 Redistributable](https://aka.ms/vc14/vc_redist.x64.exe) is installed
- Start the app
- Select path to ArgyllCMS's spotread.exe
- Select list of patches to measure
- Select display to take measurements of
- Connect instrument
- (depending on instrument) Calibrate instrument
- Place instrument on the test window
- Start measurements
  - If measuring large number of patches, you may need to re-calibrate instrument mid-run
- Save results into .csv file

You can select custom list of patches to measure. Expected format is .csv file, with no header, with 1 line per patch: `WindowSize,HDR10R,HDR10G,HDR10B`, where
- `WindowSize` - Float value corresponding to Window Size, in percentage of display pixels, to show patch on, in range from 0.0 to 1.0. All pixels outside this window will be black
- `HDR10R`/`HDR10G`/`HDR10B` - Integers representing R/G/B values respectively, in HDR10 color space

e.g.
```
0.01,0,0,0
0.01,128,128,128
0.01,256,256,256
0.01,516,516,516
0.01,1023,1023,1023
```

Result is .csv file, with header, of following formar: `Window Size,HDR10R,HDR10G,HDR10B,XYZ X,XYZ Y,XYZ Z`, where
- `Window Size`/`HDR10R`/`HDR10G`/`HDR10B` - Measured patch, in exact same format as in the aforementioned list of custom patches
- `XYZ X`/`XYZ Y`/`XYZ Z` - Measurement result

e.g.
```
Screen Percentage,HDR10 R,HDR10 G,HDR10,XYZ X,XYZ Y,XYZ Z
0.01,0,0,0,0.005659,0.005773,0.00956
0.01,32,32,32,0.002031,0.001154,0.004873
0.01,64,64,64,0.042634,0.040656,0.070284
0.01,96,96,96,0.168207,0.169226,0.24845
0.01,128,128,128,0.393999,0.396544,0.576013
0.01,160,160,160,0.77168,0.780505,1.10945
0.01,192,192,192,1.42878,1.44564,2.07244
0.01,224,224,224,2.2657,2.29621,3.25914
0.01,256,256,256,3.58686,3.63639,5.16566
0.01,288,288,288,5.51208,5.58401,8.00599
0.01,320,320,320,8.42551,8.53027,12.2003
0.01,352,352,352,11.9881,12.173,17.2616
...
```

# Shoutout to ArgyllCMS

This project wouldn't be possible without ArgyllCMS. Please consider [donating to ArgyllCMS](https://www.argyllcms.com/#contribute) if you find it useful. Instruments are not cheap 🙃.

# Building

## Prerequisites

* CMake 3.25 or Later
* Visual Studio 2022 with C++ development components installed
  * Other Generators or older versions of Visual Studio may work, but are not tested

## How to build

In the repository root:
1. Initialize git submodules 
```
git submodule update --init --recursive
```
2. Configure CMake project
```
cmake -S . -B build
```
3. Build generated project
```
cmake --build build -j --config Release
```

After build, artifacts will be in the `build/{Configuration}` folder
