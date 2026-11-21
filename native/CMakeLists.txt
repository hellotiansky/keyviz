cmake_minimum_required(VERSION 3.24)
project(KeyvizNative VERSION 0.2.0 LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "KeyvizNative currently supports Windows only.")
endif()

add_executable(KeyvizNative WIN32
    src/main.cpp
    src/input_core.cpp
    src/event_engine.cpp
    src/renderer.cpp
)

target_include_directories(KeyvizNative PRIVATE include)
target_compile_features(KeyvizNative PRIVATE cxx_std_20)
target_compile_definitions(KeyvizNative PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)
target_link_libraries(KeyvizNative PRIVATE
    user32
    comdlg32
    shell32
    gdi32
    d2d1
    dwrite
)

if(MSVC)
    target_compile_options(KeyvizNative PRIVATE /W4 /permissive- /EHsc /utf-8)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(HistoryStoreTests
        tests/history_store_tests.cpp
    )
    target_include_directories(HistoryStoreTests PRIVATE include)
    target_compile_features(HistoryStoreTests PRIVATE cxx_std_20)
    target_compile_definitions(HistoryStoreTests PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    if(MSVC)
        target_compile_options(HistoryStoreTests PRIVATE /W4 /permissive- /EHsc /utf-8)
    endif()
    add_test(NAME HistoryStoreTests COMMAND HistoryStoreTests)
endif()
