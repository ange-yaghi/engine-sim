# Engine Simulator
![Alt text](docs/public/screenshot_v01.png?raw=true)
---
## **Warning: code is in development and will change frequently**
---

## What is this?

This is a real-time internal combustion engine simulation **designed specifically to produce engine audio and simulate engine response characteristics.** It is NOT a scientific tool and cannot be expected to provide accurate figures for the purposes of engineering or engine tuning.

## Why is the code so sloppy?

I wrote this to demo in a [YouTube video](https://youtu.be/RKT-sKtR970), not as a real product. If you would like it to become a usable product please reach out to me or join my Discord (link can be found in the description of the aforementioned YouTube video). I use this codebase for my own purposes and so it might change frequently and without warning.

## How do I use it?

The UI is extremely minimalistic and there are only a few controls used to interact with the engine:

| Key/Input | Action |
| :---: | :---: |
| A | Toggle ignition |
| S | Hold for starter |
| D | Enable dyno |
| F | Enter fullscreen mode |
| Shift | Clutch |
| Up Arrow | Up Gear | 
| Down Arrow | Down Gear | 
| Z + Scroll | Volume |
| X + Scroll | Convolution Level |
| C + Scroll | High freq gain |
| V + Scroll | Low freq noise |
| B + Scroll | High freq noise |
| N + Scroll | Simulation freq |
| Escape | Exit the program |
| Q, W, E, R | Change throttle position |
| 1, 2, 3, 4, 5 | Simulation time warp |
| Tab | Change screen | 

## How do I build it?
**Note: this project currently only builds on Windows!**

### Step 1 - Clone the repository
```git clone --recurse-submodules https://github.com/ange-yaghi/engine-sim```

### Step 2 - Install CMake
Install the latest version of CMake [here](https://cmake.org/) if it's not already installed.

### Step 3 - Install Dependencies
You will need to install the following dependencies and CMake will need to be able to locate them (ie. they need to be listed on your PATH):

    1. SDL2
    2. SDL2_image
    3. Boost (make sure to build the optional dependencies)

### Step 4 - Build and Run
From the root directory of the project, run the following commands:

```
mkdir build
cd build
cmake ..
cmake --build .
```

If these steps are successful, a Visual Studio solution will be generated in ```build```. You can open this project with Visual Studio and then run the ```engine-sim-app``` project. If you encounter an error telling you that you're missing DLLs, you will have to copy those DLLs to your EXE's directory.
