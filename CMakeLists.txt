cmake_minimum_required(VERSION 3.10)
project(MyChat VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

# 编译选项和标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SERVER "Build ChatServer backend" ON)
option(BUILD_CLIENT "Build Qt ChatClient frontend" ON)

# 配置最终可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
include_directories(${PROJECT_SOURCE_DIR}/include/db)
include_directories(${PROJECT_SOURCE_DIR}/include/model)
include_directories(${PROJECT_SOURCE_DIR}/include/redis)
# 加载子目录
add_subdirectory(src)

set(CPACK_PACKAGE_NAME "MyChat")
set(CPACK_PACKAGE_VENDOR "Tony")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")
set(CPACK_GENERATOR "TGZ")
include(CPack)