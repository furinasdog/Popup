cmake_minimum_required(VERSION 3.15)
project(popup VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- 源文件 ----
add_library(popup SHARED
    src/popup.cpp
)

# 定义 POPUP_EXPORTS，让头文件使用 dllexport
target_compile_definitions(popup PRIVATE POPUP_EXPORTS)

# 头文件路径
target_include_directories(popup
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Windows 专属：链接 user32（MessageBox 所在库）
if(WIN32)
    target_link_libraries(popup PRIVATE user32)
endif()

# ---- 安装规则（可选）----
install(TARGETS popup
    RUNTIME DESTINATION bin      # .dll
    LIBRARY DESTINATION lib      # .so（Linux 备用）
    ARCHIVE DESTINATION lib      # .lib（导入库）
)
install(FILES include/popup.h DESTINATION include)

# ---- 简单测试可执行文件（可选，用于验证）----
option(POPUP_BUILD_TEST "Build test executable" OFF)
if(POPUP_BUILD_TEST)
    add_executable(popup_test test/main.cpp)
    target_link_libraries(popup_test PRIVATE popup)
endif()
