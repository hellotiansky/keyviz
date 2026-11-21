cmake_minimum_required(VERSION 3.24)
project(KeyvizNative VERSION 0.2.0 LANGUAGES CXX RC)

if(NOT WIN32)
    message(FATAL_ERROR "KeyvizNative currently supports Windows only.")
endif()

add_executable(KeyvizNative WIN32
    src/main.cpp
    src/input_core.cpp
    src/event_engine.cpp
    src/renderer.cpp
    resources/keyviz.rc
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

install(TARGETS KeyvizNative
    RUNTIME DESTINATION .
)

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

    add_executable(InputCoreTests
        tests/input_core_tests.cpp
        src/input_core.cpp
    )
    target_include_directories(InputCoreTests PRIVATE include)
    target_compile_features(InputCoreTests PRIVATE cxx_std_20)
    target_compile_definitions(InputCoreTests PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
    target_link_libraries(InputCoreTests PRIVATE user32)
    if(MSVC)
        target_compile_options(InputCoreTests PRIVATE /W4 /permissive- /EHsc /utf-8)
    endif()
    add_test(NAME InputCoreTests COMMAND InputCoreTests)
endif()
