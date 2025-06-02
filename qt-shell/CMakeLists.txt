cmake_minimum_required(VERSION 3.14)
project(qt-shell)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

# Windows 平台特殊设置
if(WIN32)
    # 设置 Windows 应用程序属性（不显示控制台窗口）
    set(CMAKE_WIN32_EXECUTABLE TRUE)
    # 确保使用 UTF-8 编码
    add_compile_options("/utf-8")
endif()

find_package(Qt5 COMPONENTS Widgets WebEngineWidgets REQUIRED)

# 使用本地的 QHotkey 而不是 FetchContent
add_subdirectory(QHotkey)

add_executable(qt-shell main.cpp)
target_link_libraries(qt-shell PRIVATE Qt5::Widgets Qt5::WebEngineWidgets QHotkey::QHotkey) 