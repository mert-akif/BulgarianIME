cmake_minimum_required(VERSION 3.20)
project(BulgarianIME VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
set_target_properties(bgime_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_compile_options(bgime_core PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -pedantic>
)

# Windows global-hook MVP.
if(WIN32)
    add_executable(BulgarianIME WIN32 windows-hook/BgimeWinHook.cpp)
    target_link_libraries(BulgarianIME PRIVATE bgime_core user32)
    target_compile_definitions(BulgarianIME PRIVATE UNICODE _UNICODE)

    add_executable(BulgarianIME_DebugConsole windows-hook/BgimeWinHookConsole.cpp)
    target_link_libraries(BulgarianIME_DebugConsole PRIVATE bgime_core user32)
    target_compile_definitions(BulgarianIME_DebugConsole PRIVATE UNICODE _UNICODE)
endif()

# Linux Fcitx5 input-method addon. Kept optional so the core/CLI can still be
# built on Linux machines where Fcitx5 development headers are not installed.
option(BGIME_BUILD_FCITX5 "Build the Linux Fcitx5 input-method addon" ON)
if(UNIX AND NOT APPLE AND BGIME_BUILD_FCITX5)
    find_package(Fcitx5Core QUIET)
    if(Fcitx5Core_FOUND)
        add_library(bulgarianime SHARED
            linux-fcitx5/BulgarianFcitxEngine.cpp
        )
        target_include_directories(bulgarianime PRIVATE linux-fcitx5)
        target_link_libraries(bulgarianime PRIVATE bgime_core Fcitx5::Core)
        set_target_properties(bulgarianime PROPERTIES PREFIX "")

        configure_file(
            linux-fcitx5/bulgarianime-addon.conf.in
            ${CMAKE_CURRENT_BINARY_DIR}/bulgarianime-addon.conf
            @ONLY
        )

        install(TARGETS bulgarianime DESTINATION "${FCITX_INSTALL_LIBDIR}/fcitx5")
        install(
            FILES "${CMAKE_CURRENT_BINARY_DIR}/bulgarianime-addon.conf"
            RENAME bulgarianime.conf
            DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
        )
        install(
            FILES linux-fcitx5/bulgarianime.conf
            DESTINATION "${FCITX_INSTALL_PKGDATADIR}/inputmethod"
        )
    else()
        message(WARNING
            "Fcitx5Core development package was not found. "
            "The CLI/core will build, but the Linux IME addon will be skipped. "
            "On Ubuntu install libfcitx5core-dev."
        )
    endif()
endif()

add_executable(bgime-cli tools/bgime-cli.cpp)
target_link_libraries(bgime-cli PRIVATE bgime_core)

add_executable(bgime-tests tests/TransliteratorTests.cpp)
target_link_libraries(bgime-tests PRIVATE bgime_core)

enable_testing()
add_test(NAME bgime-tests COMMAND bgime-tests)
