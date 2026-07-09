cmake_minimum_required(VERSION 3.20)
project(BulgarianIME LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC normally reads source files using the system code page.
# Force UTF-8 so Unicode comments/strings stay stable on Windows.
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_library(bgime_core
    core/src/Utf.cpp
    core/src/Mapping.cpp
    core/src/Transliterator.cpp
    core/src/Composer.cpp
)

target_include_directories(bgime_core PUBLIC core/include)

target_compile_options(bgime_core PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic>
)

# Windows hook app: this is the one you run for system-wide typing.
# Put it before bgime-cli so Visual Studio/CMake UIs show it first.
if(WIN32)
    add_executable(BulgarianIME WIN32 windows-hook/BgimeWinHook.cpp)
    target_link_libraries(BulgarianIME PRIVATE bgime_core user32)
    target_compile_definitions(BulgarianIME PRIVATE UNICODE _UNICODE)

    # Debug version with a visible console. Use this if you want to verify the hook is running.
    add_executable(BulgarianIME_DebugConsole windows-hook/BgimeWinHookConsole.cpp)
    target_link_libraries(BulgarianIME_DebugConsole PRIVATE bgime_core user32)
    target_compile_definitions(BulgarianIME_DebugConsole PRIVATE UNICODE _UNICODE)
endif()

# CLI is only for manual text conversion tests.
add_executable(bgime-cli tools/bgime-cli.cpp)
target_link_libraries(bgime-cli PRIVATE bgime_core)

add_executable(bgime-tests tests/TransliteratorTests.cpp)
target_link_libraries(bgime-tests PRIVATE bgime_core)

enable_testing()
add_test(NAME bgime-tests COMMAND bgime-tests)
