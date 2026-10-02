## GVIS engine... or sandbox... or sim... not sure what I'm doing here yet. 
(Gvis pronounced as: a small german child saying geewiz)

Vulkan-based engine project from scratch cause I can. Most of this is based off the vulkan tutorials by khronos cause I actually have no idea what I'm doing. 

I'll keep track of a few small ideas / milestones on this project here and the rest on my little project site: [here](https://quartz.frederich.ca/3D-Graphics-Project)
![alt text](image.png)

### Project looks like this guy ⭣
```
.
├── CMakeLists.txt 
├── include 
├── resources
│   ├── models
│   └── textures
├── shaders
└── src
    ├── fileio [ file r / w ]
    ├── platform [ using glfw ]
    ├── renderer
    │   └── shaders [ shader helpers ]
    └── vulkan [ device ]
```

### Building the Project
didn't take long to move to `CMake` and holee it's so much nicer once it's set up
```
cmake -S . -B build
cmake --build build --target run
```

### Dependencies
- MoltenVK
- Cmake
- stb_image
- tinyobj
