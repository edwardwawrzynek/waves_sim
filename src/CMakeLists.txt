if (CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s USE_SDL=2")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s USE_SDL=2 -s MAX_WEBGL_VERSION=2")
    # Setup file system packaging
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${CMAKE_SOURCE_DIR}/shaders/@/home/edward/Documents/waves_sim/shaders/")
    # Setup shell
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/web_shell.html")
else ()
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)
endif ()

include_directories(${SDL2_INCLUDE_DIRS} ${OPENGL_INCLUDE_DIRS} ../glm)

add_executable(waves_sim main.cpp geometry.cpp)
target_link_libraries(waves_sim PRIVATE ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${CMAKE_DL_LIBS} imgui)

install(TARGETS waves_sim DESTINATION ${CMAKE_INSTALL_BINDIR})