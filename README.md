# XEngine [![License](https://img.shields.io/github/license/ShawnTSH1229/XEngine.svg)](https://github.com/ShawnTSH1229/XEngine/blob/master/LICENSE)

![XEngine](/Resource/Logo.jpg?raw=true "XEngine")

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

High resolution virtual shadow map with shadow tile cache and clip maps

<p align="left">
    <img src="/Resource/renderdoc.png" width="60%" height="60%">
</p>

<p align="left">
    <img src="/Resource/physical_tile_visualize.png" width="50%" height="50%">
</p>

## Source Tree

