cmake_minimum_required(VERSION 3.20)

project(OpenNitemare3D
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(OpenNitemare3D
    src/main.cpp
    src/core/Engine.cpp
    src/render/Renderer.cpp

    src/formats/BinaryIO.cpp
    src/formats/DatArchive.cpp
    src/formats/DefinitionTable.cpp
    src/formats/DemoFile.cpp
    src/formats/MapArchive.cpp
    src/formats/Pcx8.cpp
    src/formats/ImgArchive.cpp
)

target_include_directories(OpenNitemare3D
    PRIVATE
        src
)

if(MSVC)
    target_compile_options(OpenNitemare3D PRIVATE /W4 /permissive-)
else()
    target_compile_options(OpenNitemare3D PRIVATE -Wall -Wextra -Wpedantic)
endif()
