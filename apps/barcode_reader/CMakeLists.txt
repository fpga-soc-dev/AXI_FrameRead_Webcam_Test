cmake_minimum_required(VERSION 3.20)


project(BarcodeReader)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(PkgConfig REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(OpenCV REQUIRED)
find_package(ZXing REQUIRED)
pkg_check_modules(GSTREAMER     REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)


set(SOURCES
    main.cpp
    BarcodeDetector.cpp
    BarcodeProcessor.cpp
    CameraPipeline.cpp
    FrameBufferDisplay.cpp
)


add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ZXing::ZXing
    fmt::fmt
    spdlog::spdlog_header_only
    ${OpenCV_LIBS}
    PkgConfig::GSTREAMER
    PkgConfig::GSTREAMER_APP
)

add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)

target_compile_options(${PROJECT_NAME} PUBLIC -Wall -Wextra -Wpedantic)


