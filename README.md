# XEngine [![License](https://img.shields.io/github/license/ShawnTSH1229/XEngine.svg)](https://github.com/ShawnTSH1229/XEngine/blob/master/LICENSE)

<p align="center">
    <img src="/Resource/Logo.jpg" width="60%" height="60%">
</p>

[previous deprecated repository](https://github.com/lvcheng1229/XEnigine). 

## Getting Started

Visual Studio 2019 or 2022 is recommended, XEngine is only tested on Windows.

1.Cloning the repository with `git clone https://github.com/ShawnTSH1229/XEngine.git`.

2.Configuring the build

```shell
# Create a build directory
mkdir build
cd build

# x86-64 using a Visual Studio solution
cmake -G "Visual Studio 17 2022" ../
```

## Features

1.[Virtual Shadow Map](https://shawntsh1229.github.io/2024/05/01/Virtual-Shadow-Map-In-XEngine/):

Virtual Shadow Maps (VSMs) is the new shadow mapping method used in Unreal Engine 5. I implemented a **simplified virtual shadow maps** in my **personal game engine**. Here is a brief introduction from the official unreal engine 5 documentation:
>Virtual Shadow Maps have been developed with the following goals:
>
>* Significantly increase shadow resolution to match highly detailed Nanite geometry
>* Plausible soft shadows with reasonable, controllable performance costs
>* Provide a simple solution that works by default with limited amounts of adjustment needed
>* Replace the many Stationary Light shadowing techniques with a single, unified path
>
>Conceptually, virtual shadow maps are just very **high-resolution** shadow maps. In their current implementation, they have a **virtual resolution** of 16k x 16k pixels. **Clipmaps** are used to increase resolution further for Directional Lights. To keep performance high at reasonable memory cost, VSMs split the >shadow map into tiles (or Pages) that are 128x128 each. Pages are allocated and rendered only as needed to shade **on-screen pixels** based on an analysis of the depth buffer. The pages are **cached** between frames unless they are invalidated by moving objects or light, which further improves performance.

<p align="left">
    <img src="/Resource/renderdoc.png" width="60%" height="60%">
</p>

<p align="left">
    <img src="/Resource/physical_tile_visualize.png" width="50%" height="50%">
</p>

2.[Sky atmosphere based on UE's solution](https://shawntsh1229.github.io/2024/05/05/Sky-Atmosphere-In-XEngine/):

<p align="left">
    <img src="/Resource/skyatmosphere.png" width="80%" height="80%">
</p>

3.Screen Space Reflection

4.SVO GI

5.HZB Culling

6.Vulkan & Vulkan Ray Tracing

7.DirectX12

## Source Tree

