CXX = g++
SHELL = sh

CFLAGS = \
	-std=c++20 \
	-O2 \
	-fsanitize=address \
	-Wall \
	-Werror \
	-MMD \
	-MP

LDFLAGS = \
    -L$(VULKAN_SDK)/lib \
    -L$(BREW_PREFIX)/lib \
    -Wl,-rpath,$(VULKAN_SDK)/lib \
    -lglfw \
    -lvulkan \
    -framework Cocoa \
    -framework Metal \
    -framework QuartzCore \
    -framework IOKit \
    -framework CoreVideo

VULKAN_SDK = $(HOME)/VulkanSDK/1.4.341.1/macOS
SLANG_COMPILER = $(VULKAN_SDK)/bin/slangc
SHADER_SRC = src/renderer/shaders/shader.slang
SHADER_OUT = shaders/slang.spv
BREW_PREFIX = /opt/homebrew
SRC_DIR = src
INCLUDE_DIR = include
INCLUDES = -I$(INCLUDE_DIR) \
		   -I$(SRC_DIR)/platform \
		   -I$(SRC_DIR)/renderer \
		   -I$(SRC_DIR)/renderer/shaders \
		   -I$(SRC_DIR)/vulkan \
		   -I$(SRC_DIR)/fileio \
		   -I$(VULKAN_SDK)/include \
		   -I$(BREW_PREFIX)/include
BUILD_DIR = build
BIN_DIR = bin
TARGET = $(BIN_DIR)/VulkanTest

SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')

OBJECTS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCES)) 

DEPS := $(OBJECTS:.o=.d)  
-include $(DEPS)  

export VK_ICD_FILENAMES = $(VULKAN_SDK)/share/vulkan/icd.d/MoltenVK_icd.json
export VK_LAYER_PATH = $(VULKAN_SDK)/share/vulkan/explicit_layer.d

$(SHADER_OUT): $(SHADER_SRC)
	@mkdir -p $(@D)
	$(SLANG_COMPILER) $(SHADER_SRC) \
		-target spirv \
		-profile spirv_1_4 \
		-emit-spirv-directly \
		-fvk-use-entrypoint-name \
		-entry vertMain \
		-entry fragMain \
		-o $(SHADER_OUT)

$(TARGET): $(OBJECTS) $(SHADER_OUT)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJECTS) $(CFLAGS) $(LDFLAGS) -o $(TARGET)
	@echo "build target: $@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp  
	@mkdir -p $(@D)  
	$(CXX) $(CFLAGS) $(INCLUDES) -c $< -o $@  

run: $(TARGET)
	./$(TARGET)

all: $(TARGET)  
	 
clean:  
	@rm -rf $(BUILD_DIR) $(BIN_DIR) $(SHADER_OUT)  
	@echo "Cleaned build and bin directories"  

.PHONY: run clean all
