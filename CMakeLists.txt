cmake_minimum_required(VERSION 3.20)
project(gvis LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # replaces `bear`

# ---- Paths ------------------------------------------------------------
if(NOT DEFINED ENV{VULKAN_SDK})
    set(ENV{VULKAN_SDK} "$ENV{HOME}/VulkanSDK/1.4.341.1/macOS")
endif()
set(VULKAN_SDK "$ENV{VULKAN_SDK}")
set(BREW_PREFIX "/opt/homebrew")
list(APPEND CMAKE_PREFIX_PATH "${BREW_PREFIX}" "${VULKAN_SDK}")

# ---- Dependencies -----------------------------------------------------
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_program(SLANGC slangc HINTS "${VULKAN_SDK}/bin" REQUIRED)

# ---- Executable -------------------------------------------------------
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")
add_executable(gvis ${SOURCES})

set_target_properties(gvis PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

target_include_directories(gvis PRIVATE
    src/platform
    src/renderer
    src/renderer/shaders
    src/vulkan
    src/fileio
)

# Vendored headers (stb, tinyobj) as SYSTEM => their warnings are suppressed,
# so you can drop the #pragma GCC diagnostic block around tiny_obj_loader.h
target_include_directories(gvis SYSTEM PRIVATE
    include
    "${VULKAN_SDK}/include"
    "${BREW_PREFIX}/include"
)

target_compile_options(gvis PRIVATE
    # -O2 -Wall -Werror -fsanitize=address
    -Wall -Werror
)
# target_link_options(gvis PRIVATE -fsanitize=address)
target_link_options(gvis PRIVATE)

target_link_libraries(gvis PRIVATE
    Vulkan::Vulkan
    glfw
    "-framework Cocoa"
    "-framework Metal"
    "-framework QuartzCore"
    "-framework IOKit"
    "-framework CoreVideo"
)
set_target_properties(gvis PROPERTIES
    BUILD_RPATH "${VULKAN_SDK}/lib"
)

# ---- Shaders ----------------------------------------------------------
set(SHADER_SRC "${CMAKE_SOURCE_DIR}/shaders/shader.slang")
set(SHADER_OUT "${CMAKE_SOURCE_DIR}/shaders/slang.spv")

add_custom_command(
    OUTPUT "${SHADER_OUT}"
    COMMAND "${SLANGC}" "${SHADER_SRC}"
    -target spirv
    -profile spirv_1_4
    -emit-spirv-directly
    -fvk-use-entrypoint-name
    -entry vertMain
    -entry fragMain
    -o "${SHADER_OUT}"
    DEPENDS "${SHADER_SRC}"
    COMMENT "Compiling Slang shader"
    VERBATIM
)
add_custom_target(shaders DEPENDS "${SHADER_OUT}")


# should need this
# ---- Textures ---------------------------------------------------------
# add_custom_target(textures
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     "${CMAKE_SOURCE_DIR}/src/textures"
#     "${CMAKE_SOURCE_DIR}/textures"
#     COMMENT "Copying textures"
# )

# add_dependencies(gvis shaders textures)
add_dependencies(gvis shaders)

# ---- Run --------------------------------------------------------------
add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E env
    "VK_ICD_FILENAMES=${VULKAN_SDK}/share/vulkan/icd.d/MoltenVK_icd.json"
    "VK_LAYER_PATH=${VULKAN_SDK}/share/vulkan/explicit_layer.d"
    $<TARGET_FILE:gvis>
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS gvis
    USES_TERMINAL
)
