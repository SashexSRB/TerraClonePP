cmake_minimum_required(VERSION 3.20)
project(TerraClonePP VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ─── Sources ────────────────────────────────────────────────────────────────

file(GLOB_RECURSE ENGINE_SOURCES Engine/*.cpp)
file(GLOB_RECURSE GAME_SOURCES   Game/*.cpp)

add_executable(TerraClonePP
        main.cpp
        VulkanApp.cpp
        ${ENGINE_SOURCES}
        ${GAME_SOURCES}
        Game/World/Generator.h
        Game/World/Generator.cpp
        Game/Items/Item.h
        Game/Items/Item.cpp
        Game/World/ChunkManager.h
        Game/World/ChunkManager.cpp
        Game/World/ChunkMesher.cpp
        Game/World/ChunkMesher.h
        Game/World/WorldSerializer.cpp
        Game/World/WorldSerializer.h
)

# ─── Include paths ──────────────────────────────────────────────────────────

target_include_directories(TerraClonePP PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Game
        ${CMAKE_SOURCE_DIR}/Include
        ${CMAKE_SOURCE_DIR}/Lib
)

# ─── Asset path ─────────────────────────────────────────────────────────────

target_compile_definitions(TerraClonePP PRIVATE
        ASSET_PATH="${CMAKE_BINARY_DIR}/"
)

# ─── Find packages ──────────────────────────────────────────────────────────

find_package(Vulkan REQUIRED)
find_package(glfw3  REQUIRED)
find_package(glm    REQUIRED)

target_link_libraries(TerraClonePP PRIVATE
        Vulkan::Vulkan
        glfw
        glm::glm
        dl
        pthread
        X11
        Xxf86vm
        Xrandr
        Xi
        stdc++fs
)

# ─── Shaders ────────────────────────────────────────────────────────────────

find_program(GLSLC glslc REQUIRED)

set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/Engine/Shaders)
set(SHADER_BINARY_DIR ${CMAKE_BINARY_DIR}/Shaders)

file(GLOB SHADER_SOURCES
        ${SHADER_SOURCE_DIR}/*.vert
        ${SHADER_SOURCE_DIR}/*.frag
)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    get_filename_component(SHADER_EXT  ${SHADER} EXT)

    if(SHADER_EXT STREQUAL ".vert")
        set(SHADER_OUTPUT ${SHADER_BINARY_DIR}/vert.spv)
    elseif(SHADER_EXT STREQUAL ".frag")
        set(SHADER_OUTPUT ${SHADER_BINARY_DIR}/frag.spv)
    endif()

    add_custom_command(
            OUTPUT  ${SHADER_OUTPUT}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
            COMMAND ${GLSLC} ${SHADER} -o ${SHADER_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling shader ${SHADER_NAME}"
    )
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(TerraClonePP Shaders)

# ─── Textures ───────────────────────────────────────────────────────────────

set(TEXTURE_SOURCE ${CMAKE_SOURCE_DIR}/Game/Assets/Textures/textures.png)
set(TEXTURE_DEST   ${CMAKE_BINARY_DIR}/textures.png)

add_custom_command(
        OUTPUT  ${TEXTURE_DEST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TEXTURE_SOURCE} ${TEXTURE_DEST}
        DEPENDS ${TEXTURE_SOURCE}
        COMMENT "Copying textures"
)

add_custom_target(Textures ALL DEPENDS ${TEXTURE_DEST})
add_dependencies(TerraClonePP Textures)

# ─── Fonts ───────────────────────────────────────────────────────────────
set(FONT_SOURCE ${CMAKE_SOURCE_DIR}/Game/Assets/Fonts/ANDYB.TTF)
set(FONT_DEST   ${CMAKE_BINARY_DIR}/ANDYB.TTF)

add_custom_command(
        OUTPUT  ${FONT_DEST}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FONT_SOURCE} ${FONT_DEST}
        DEPENDS ${FONT_SOURCE}
        COMMENT "Copying font"
)
add_custom_target(Fonts ALL DEPENDS ${FONT_DEST})
add_dependencies(TerraClonePP Fonts)