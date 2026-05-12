# Compiler and tools
CXX = g++
GLSLC = glslc
CFLAGS = -std=c++20 -O2 -IEngine -IGame -IInclude -ILib
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

# Source files — recursive for Game, flat for Engine
ENGINE_SOURCES = $(wildcard $(ENGINE_DIR)/*.cpp)
GAME_SOURCES   = $(shell find $(GAME_DIR) -name '*.cpp')
ROOT_SOURCES   = VulkanApp.cpp main.cpp

SOURCES = $(ENGINE_SOURCES) $(GAME_SOURCES) $(ROOT_SOURCES)

# Object files
ENGINE_OBJECTS = $(patsubst $(ENGINE_DIR)/%.cpp, $(BUILD_ENGINE_DIR)/%.o, $(ENGINE_SOURCES))
GAME_OBJECTS   = $(patsubst $(GAME_DIR)/%.cpp,   $(BUILD_GAME_DIR)/%.o,   $(GAME_SOURCES))
ROOT_OBJECTS   = $(patsubst %.cpp,               $(BUILD_DIR)/%.o,         $(ROOT_SOURCES))

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
all: shaders textures $(TARGET)

# Shaders
shaders: $(SHADER_OBJECTS)

# Textures
textures: $(TEXTURE_OBJECT)

# Link
$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

# Compile engine sources
$(BUILD_ENGINE_DIR)/%.o: $(ENGINE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

# Compile game sources (handles subdirectories)
$(BUILD_GAME_DIR)/%.o: $(GAME_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

# Compile root sources
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) -c $< -o $@

# Shaders
$(BUILD_SHADER_DIR)/vert.spv: $(SHADER_DIR)/shader.vert
	@mkdir -p $(BUILD_SHADER_DIR)
	$(GLSLC) $< -o $@

$(BUILD_SHADER_DIR)/frag.spv: $(SHADER_DIR)/shader.frag
	@mkdir -p $(BUILD_SHADER_DIR)
	$(GLSLC) $< -o $@

# Textures
$(BUILD_TEXTURE_DIR)/textures.png: $(TEXTURE_DIR)/textures.png
	@mkdir -p $(BUILD_TEXTURE_DIR)
	cp $< $@

# Run
test: all
	./$(TARGET)

# Clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all shaders textures test clean