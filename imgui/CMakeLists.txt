if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_SDL=2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_SDL=2")
else()
    find_package(SDL2 REQUIRED)
endif()

# Build Dear Imgui with opengl3 + sdl backend
add_library(imgui STATIC
        imgui.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        imgui_demo.cpp
        backends/imgui_impl_opengl3.cpp
        backends/imgui_impl_sdl.cpp
)

target_include_directories(imgui
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/backends"
    PRIVATE ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(imgui PRIVATE ${SDL2_LIBRARIES} ${CMAKE_DL_LIBS})