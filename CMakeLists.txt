cmake_minimum_required(VERSION 3.20)
project(waves)

set(CMAKE_CXX_STANDARD 17)

if(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    set(CMAKE_EXECUTABLE_SUFFIX .html)
endif()

# Enable gcc/clang warnings
if(CMAKE_CXX_COMPILER_ID MATCHES GNU OR CMAKE_CXX_COMPILER_ID MATCHES Clang)
    set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter")
    set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

include(GNUInstallDirs)

include(CTest)
if(BUILD_TESTING)
    # Load GTest
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
endif()

add_subdirectory(src)
add_subdirectory(imgui)
