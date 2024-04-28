set(TARGET_NAME XEngineRuntime)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

#source_group("Hello" FILES Hello/hello.h Hello/hello.cpp)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES} ${SOURCE_FILES})

link_libraries("d3dcompiler.lib")
link_libraries("D3D12.lib")
link_libraries("dxgi.lib")

#add_library(${TARGET_NAME} ${HEADER_FILES} ${SOURCE_FILES})
add_executable()


#https://zhuanlan.zhihu.com/p/55207498
#include and taget include
target_include_directories(
  ${TARGET_NAME} 
  PUBLIC 
  ${XENGINE_ROOT_DIR}/Source
)

add_definitions(-X_PLATFORM_WIN)
add_definitions(-X_RHI_DX12)