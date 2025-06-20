cmake_minimum_required(VERSION 3.14)
project(zdf-exam-desktop)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 14)

# Windows 平台特殊设置
if(WIN32)
    # 设置 Windows 应用程序属性（不显示控制台窗口）
    set(CMAKE_WIN32_EXECUTABLE TRUE)
    # 确保使用 UTF-8 编码
    add_compile_options("/utf-8")
endif()

# 确保找到Qt WebEngine相关组件
find_package(Qt5 COMPONENTS Core Widgets WebEngineWidgets WebEngine REQUIRED)

# 使用本地的 QHotkey 而不是 FetchContent
add_subdirectory(QHotkey)

add_executable(zdf-exam-desktop main.cpp)
target_link_libraries(zdf-exam-desktop PRIVATE 
    Qt5::Core 
    Qt5::Widgets 
    Qt5::WebEngineWidgets 
    Qt5::WebEngine 
    QHotkey::QHotkey
) 