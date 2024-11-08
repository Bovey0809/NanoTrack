cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 11)
project(nanotrack_demo)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g") # Debug模式 

set(ROOT_DIR ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "Current build dir:" ${CMAKE_CURRENT_LIST_DIR})

set(OpenCV_STATIC ON) 
# 更新 OpenCV 路径
set(OpenCV_DIR "/opt/homebrew/Cellar/opencv/4.10.0_12/lib/cmake/opencv4")  

# 查找 OpenCV
find_package(OpenCV REQUIRED)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found.")
endif()

message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

# 引入外部库的方式2 以导入外部库的方式(不会重新编译)重新构建一个ncnn库
add_library(ncnn STATIC IMPORTED) # 导入 ncnn dynamic lib 
set_target_properties(ncnn PROPERTIES IMPORTED_LOCATION  ${ROOT_DIR}/lib/libncnn.a)

# OpenMP 配置
if(APPLE)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(OpenMP_C "${CMAKE_C_COMPILER}")
        set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include")
        set(OpenMP_C_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib")
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
        set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I/opt/homebrew/opt/libomp/include")
        set(OpenMP_CXX_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY "/opt/homebrew/opt/libomp/lib/libomp.dylib")
    endif()
endif()

# 查找 OpenMP
find_package(OpenMP REQUIRED)
if(OPENMP_FOUND)
    message("OPENMP FOUND")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    include_directories("/opt/homebrew/opt/libomp/include")
endif()

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ncnn/
)

if(APPLE)
    # 添加宏定义来避免命名冲突
    add_definitions(-D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=0)
    add_definitions(-DCHECK_RANGE_UINT64_)  # 避免某些宏定义冲突
    
    # 添加 macOS 框架支持
    find_library(APPKIT_LIBRARY AppKit)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(UNIFORMTYPEIDENTIFIERS_LIBRARY UniformTypeIdentifiers)
    set(EXTRA_LIBS ${APPKIT_LIBRARY} ${FOUNDATION_LIBRARY} ${UNIFORMTYPEIDENTIFIERS_LIBRARY})
endif()

# 首先定义可执行文件目标
add_executable(nanotrack_demo demo_ncnn.cpp nanotrack.cpp)

# 然后添加链接库
target_link_libraries(
    nanotrack_demo 
    ncnn
    ${OpenCV_LIBS}
    ${EXTRA_LIBS}
) 

# 为 Objective-C++ 支持添加编译选项
if(APPLE)
    set_source_files_properties(demo_ncnn.cpp PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

if(UNIX AND NOT APPLE)
    # 在 Linux 上添加 GTK 支持
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
    add_definitions(${GTK3_CFLAGS_OTHER})
    target_link_libraries(nanotrack_demo ${GTK3_LIBRARIES})
endif()