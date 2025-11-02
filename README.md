Bezier Patch Renderer
A modern OpenGL application for rendering and manipulating bicubic Bezier patches with real-time interaction and Phong shading.

https://img.shields.io/badge/OpenGL-3.3+-blue.svg
https://img.shields.io/badge/C++-17-orange.svg
https://img.shields.io/badge/GLM-0.9.9-green.svg
https://img.shields.io/badge/License-MIT-yellow.svg

Features
 Bicubic Bezier Patch Rendering - Real-time rendering of 4×4 control point surfaces

 Phong Shading - Dynamic lighting with diffuse component calculation

 Interactive Control - Manipulate control points in 3D space

 Dynamic Tessellation - Adjust patch resolution from 1 to 50 samples

 Camera Control - Orbit and examine the patch from any angle

 Visual Guides - Display control points and coordinate axes

 Performance Optimized - Efficient buffer updates and rendering

Screenshots
(Add your screenshots here)

Installation
Prerequisites
C++17 compatible compiler

CMake 3.10+

OpenGL 3.3+ compatible graphics card

Dependencies
GLFW - Window and input management

GLAD - OpenGL function loading

GLM - Mathematics library for graphics

Building from Source
bash
# Clone the repository
git clone https://github.com/yourusername/bezier-patch-renderer.git
cd bezier-patch-renderer

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run the application
./bezier_patch
Usage
Basic Controls
Camera Movement
W/S - Move forward/backward

A/D - Move left/right

R - Reset camera view

Control Point Selection
Left Arrow - Select previous control point

Right Arrow - Select next control point

Selected point appears in yellow

Control Point Manipulation
I/K - Increase/decrease Y coordinate

J/L - Increase/decrease X coordinate

U/O - Increase/decrease Z coordinate

Patch Resolution
+ - Increase tessellation level

- - Decrease tessellation level

Features Overview
Bezier Surface Rendering

Renders a 16-control point bicubic Bezier patch

Uses iterative Bernstein polynomial evaluation

Real-time surface updates when control points change

Lighting System

Phong shading model implementation

Dynamic light source following camera

Diffuse component calculation per fragment

Visual Elements

Control points displayed as large dots

Coordinate axes (X-red, Y-green, Z-blue)

Smooth normal calculation for realistic shading

Technical Details
Bezier Patch Evaluation
The patch is evaluated using the formula:

cpp
P(u,v) = ΣΣ B_i(u) * B_j(v) * P_ij
Where B_i(u) and B_j(v) are Bernstein basis functions:

cpp
float B(int i, float t) {
    int n = 3;
    int C[4] = {1, 3, 3, 1};
    return C[i] * pow(t, i) * pow(1 - t, n - i);
}
Normal Calculation
Surface normals are computed by averaging face normals of adjacent triangles:

cpp
glm::vec3 v1 = vertices[idxRight] - vertices[idx];
glm::vec3 v2 = vertices[idxDown] - vertices[idx];
glm::vec3 normal = glm::normalize(glm::cross(v1, v2));
Shader Implementation
Vertex Shader:

Transforms vertices to clip space

Passes normals and fragment positions

Applies model-view-projection matrices

Fragment Shader:

Calculates diffuse lighting using Lambert's cosine law

Uses camera position for light direction

Outputs final shaded color

File Structure
text
bezier-patch-renderer/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   └── shaders/
│       ├── vertex_shader.glsl
│       └── fragment_shader.glsl
├── include/
│   └── (header files)
├── lib/
│   └── (dependency libraries)
└── README.md
Configuration
Default Control Points
The application starts with a predefined set of 16 control points:

cpp
glm::vec3 controlPoints[16] = {
    {0.0, 0.0, 0.0}, {2.0, 0.0, 1.5}, {4.0, 0.0, 2.9}, {6.0, 0.0, 0.0},
    {0.0, 2.0, 1.1}, {2.0, 2.0, 3.9}, {4.0, 2.0, 3.1}, {6.0, 2.0, 0.7},
    {0.0, 4.0, -0.5},{2.0, 4.0, 2.6},{4.0, 4.0, 2.4},{6.0, 4.0, 0.4},
    {0.0, 6.0, 0.3}, {2.0, 6.0, -1.1},{4.0, 6.0, 1.3},{6.0, 6.0, -0.2}
};
Performance Optimization
Selective Updates: Buffers are only updated when geometry changes

Efficient Normal Calculation: Vertex normals are computed during tessellation

GL State Management: Proper OpenGL state setup and cleanup

Customization
Adding New Control Points
Modify the controlPoints array in main.cpp to create different surface shapes.

Changing Shading
Edit the fragment shader to implement different lighting models:

Specular highlights

Ambient occlusion

Texturing

Adjusting Camera
Modify initial camera parameters:

cpp
glm::vec3 camPos = glm::vec3(3.0f, 5.0f, 15.0f);
Troubleshooting
Common Issues
Black Screen

Verify OpenGL 3.3+ support

Check shader compilation logs

Poor Performance

Reduce tessellation level

Update graphics drivers

Control Points Not Visible

Ensure point size is enabled

Check point rendering code

Debug Mode
Compile with debug flags for detailed OpenGL error reporting:

bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
Contributing
Fork the repository

Create a feature branch (git checkout -b feature/amazing-feature)

Commit your changes (git commit -m 'Add amazing feature')

Push to the branch (git push origin feature/amazing-feature)

Open a Pull Request

License
This project is licensed under the MIT License - see the LICENSE file for details.

Acknowledgments
OpenGL documentation and community

GLFW, GLAD, and GLM developers

Computer Graphics course materials and instructors
