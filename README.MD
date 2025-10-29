# Synthwave Compute

This repo is an example project on how to draw on a texture using a compute shader to display
it on screen using [Nova Engine](https://github.com/Tiwann/Nova).

![](Docs/SynthwaveCompute_NovaEngine_Screenshot.gif)

## How to build
Clone the project with all submodule: <br>
`git clone git@github.com:Tiwann/SynthwaveCompute.git --recursive`

I use CMake as my build system. Version 4.0 is required so you might need to update it. <br>
Then simply configure the project with : <br>
> `cd SynthWaveCompute` <br>
> `cmake -S . -B Build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF` <br>
> `cmake --build Build`

## Dependencies
* [Vulkan SDK (version 1.4)](https://vulkan.lunarg.com/sdk/home)
* [GLFW](https://www.glfw.org)
* [Box2D](https://box2d.org)
* [miniaudio](https://github.com/mackron/miniaudio)
* [assimp](https://www.assimp.org)

Note that all the dependencies are directly included as submodules. If you didn't clone the project using `--recursive` arguement
you can initialize the submodules with `git submodule update --init --recursive` from the root of the repo.

## Credits
Credits to [stduhpf](https://www.shadertoy.com/user/stduhpf) for the incredible shader. The shader was rewritten in [Slang](https://shader-slang.org), the shading language that I use in my engine. <br>
See [Synthwave.slang](Synthwave/Assets/Synthwave.slang)
