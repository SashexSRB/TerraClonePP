# Compiler and tools
CXX = g++
GLSLC = ~/VulkanSDK/1.4.321.1/x86_64/bin/glslc
CFLAGS = -std=c++20 -O2 -IEngine -IGame
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# Directories
ENGINE_DIR = Engine
GAME_DIR = Game
SHADER_DIR = $(ENGINE_DIR)/Shaders
TEXTURE_DIR = $(GAME_DIR)/Textures
BUILD_DIR = Build
BUILD_ENGINE_DIR = $(BUILD_DIR)/Engine
BUILD_GAME_DIR = $(BUILD_DIR)/Game
BUILD_SHADER_DIR = $(BUILD_ENGINE_DIR)/Shaders
BUILD_TEXTURE_DIR = $(BUILD_GAME_DIR)/Textures

# Source files
ENGINE_SOURCES = $(wildcard $(ENGINE_DIR)/*.cpp)
GAME_SOURCES = $(wildcard $(GAME_DIR)/*.cpp)
ROOT_SOURCES = VulkanApp.cpp main.cpp
SOURCES = $(ENGINE_SOURCES) $(GAME_SOURCES) $(ROOT_SOURCES)

# Object files
ENGINE_OBJECTS = $(patsubst $(ENGINE_DIR)/%.cpp,$(BUILD_ENGINE_DIR)/%.o,$(ENGINE_SOURCES))
GAME_OBJECTS = $(patsubst $(GAME_DIR)/%.cpp,$(BUILD_GAME_DIR)/%.o,$(GAME_SOURCES))
ROOT_OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(ROOT_SOURCES))
OBJECTS = $(ENGINE_OBJECTS) $(GAME_OBJECTS) $(ROOT_OBJECTS)

# Shader files
SHADER_SOURCES = $(SHADER_DIR)/shader.vert $(SHADER_DIR)/shader.frag
SHADER_OBJECTS = $(BUILD_SHADER_DIR)/vert.spv $(BUILD_SHADER_DIR)/frag.spv

# Texture files
TEXTURE_SOURCE = $(TEXTURE_DIR)/textures.png
TEXTURE_OBJECT = $(BUILD_TEXTURE_DIR)/textures.png

# Target executable
TARGET = $(BUILD_DIR)/TerraClonePP

# Default target
all: shaders textures engine game $(TARGET)

# Stage 0: Compile shaders
shaders: $(SHADER_OBJECTS)

# Stage 0.5: Copy textures
textures: $(TEXTURE_OBJECT)

# Stage 1: Build engine objects (depends on shaders)
engine: shaders $(ENGINE_OBJECTS)

# Stage 2: Build game objects (depends on engine)
game: engine $(GAME_OBJECTS)

# Link all objects into executable
$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile engine source files
$(BUILD_ENGINE_DIR)/%.o: $(ENGINE_DIR)/%.cpp
	@mkdir -p $(BUILD_ENGINE_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

# Compile game source files
$(BUILD_GAME_DIR)/%.o: $(GAME_DIR)/%.cpp
	@mkdir -p $(BUILD_GAME_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

# Compile root source files
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

# Compile vertex shader
$(BUILD_SHADER_DIR)/vert.spv: $(SHADER_DIR)/shader.vert
	@mkdir -p $(BUILD_SHADER_DIR)
	$(GLSLC) $< -o $@

# Compile fragment shader
$(BUILD_SHADER_DIR)/frag.spv: $(SHADER_DIR)/shader.frag
	@mkdir -p $(BUILD_SHADER_DIR)
	$(GLSLC) $< -o $@

# Copy texture file
$(BUILD_TEXTURE_DIR)/textures.png: $(TEXTURE_DIR)/textures.png
	@mkdir -p $(BUILD_TEXTURE_DIR)
	cp $< $@

# Run the executable
test: $(TARGET)
	./$(TARGET)

# Clean build artifacts, shaders, and textures
clean:
	rm -rf $(BUILD_DIR)

# Phony targets
.PHONY: all shaders textures engine game test clean
