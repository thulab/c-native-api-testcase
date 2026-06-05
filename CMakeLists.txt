# SessionC 原生接口自动化测试 - 主配置
cmake_minimum_required(VERSION 3.7)

project("c-native-api-testcase")

# C++17（测试代码以 C++ 编译，但 IoTDB 能力仅通过 SessionC.h 这一个头文件访问）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# 头文件目录：client/include 下含唯一的 SessionC.h 以及 gtest/
include_directories(${CMAKE_SOURCE_DIR}/client/include)

# boost（与 C++ 客户端一致的依赖）
find_package(Boost REQUIRED)
if(DEFINED BOOST_INCLUDEDIR)
    include_directories("${Boost_INCLUDE_DIR}")
endif()

# 库文件目录
link_directories(${CMAKE_SOURCE_DIR}/client/lib)

# 可选：地址消毒（默认关闭，避免预编译 .so 内部噪声导致整体失败）
# 开启： cmake -DENABLE_ASAN=ON ..
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(ENABLE_ASAN AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang") AND NOT MSVC)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

add_subdirectory("test")
