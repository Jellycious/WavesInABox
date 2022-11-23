# WavesInABox
A wavefront simulator and artwork in one.

![Animation](https://github.com/Jellycious/WavesInABox/blob/main/screenshots/wavesinabox.gif)
## Description:
WavesInABox is an artwork that explores the border between chaos and order in a surface of water.Imagine a body of water enclosed in a box. If the water is touched a wave will be created. This wave will travel through the water until it hits the wall. When this happens it will reflect and interact with other waves. Will the result be orderly patterns or disorderly movement.

WavesInABox is a real-time wavefront simulator written from scratch. The simulation can be viewed in 3D. The simulation uses the finite difference method to simulate the [wave equation](https://en.wikipedia.org/wiki/Wave_equation) in two dimensions. Solving the wave equation is gpu-accelerated using OpenGL compute shaders. 

![]()
<img src="/screenshots/screenshot1.png" width=300 height=300>
<img src="/screenshots/screenshot2.png" width=300 height=300>
<img src="/screenshots/screenshot3.png" width=300 height=300>
<img src="/screenshots/screenshot4.png" width=300 height=300>

## Interaction
Move camera using \<a\> \<w\> \<s\> \<d\>.

Increase/decrease Damping with \<spacebar\> / \<backspace\>

Add/remove wave source  \<left mouse\> / \<right mouse\>

## Building from source
#### To build the project for ubuntu:
1. Install depedencies
```
sudo apt install libglfw3-dev libgles2-mesa-dev libegl1-mesa-dev libstb-dev libglm-dev
sudo apt install build-essential
```
2. Install cglm library (https://github.com/recp/cglm)
3. Build Source Code
```
cd WavesInABox
make
```
## Dependencies
- [GLFW](https://www.glfw.org/)
- [OpenGL](https://www.opengl.org/)
- [GLM](https://github.com/g-truc/glm)
- [CGLM](https://github.com/recp/cglm)
