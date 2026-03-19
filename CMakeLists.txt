cmake_minimum_required(VERSION 3.20)

project(GameboySokol LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(emugameboy
  src/main_emu.cpp
  # src/disassembler.cpp
  src/cpu.cpp
  # src/sokol.cpp
)

add_custom_target(run
  COMMAND emugameboy
  DEPENDS emugameboy
  WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)

target_include_directories(emugameboy PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor)

if(APPLE)
  enable_language(OBJCXX)
  set_source_files_properties(src/sokol.cpp PROPERTIES LANGUAGE OBJCXX)

  target_link_libraries(emugameboy PRIVATE
    "-framework Cocoa"
    "-framework QuartzCore"
    "-framework OpenGL"
    "-framework AudioToolbox"
  )
elseif(UNIX AND NOT APPLE)
  target_link_libraries(emugameboy PRIVATE
    GL
    dl
    m
    pthread
    X11
    Xi
    Xcursor
    Xrandr
    asound
  )
elseif(WIN32)
  target_link_libraries(emugameboy PRIVATE
    kernel32
    user32
    shell32
    gdi32
    d3d11
    dxgi
  )
endif()
