# 最低CMake版本和项目名
cmake_minimum_required(VERSION 3.10)
project(ChatClientQt LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Qt自动检测（优先Qt6，找不到则用Qt5）
find_package(Qt6 COMPONENTS Widgets Network QUIET)
if(Qt6_FOUND)
	set(QT_LIBS Qt6::Widgets Qt6::Network)
	set(QT_VERSION_MAJOR 6)
else()
	find_package(Qt5 COMPONENTS Widgets Network REQUIRED)
	set(QT_LIBS Qt5::Widgets Qt5::Network)
	set(QT_VERSION_MAJOR 5)
endif()

set(CLIENT_UI_SRCS
	ui/MainWindow.cpp
	ui/LoginDialog.cpp
	ui/AddFriendDialog.cpp
	ui/JoinGroupDialog.cpp
	ui/CreateGroupDialog.cpp
)
set(CLIENT_UI_HDRS
	ui/MainWindow.h
	ui/LoginDialog.h
	ui/AddFriendDialog.h
	ui/JoinGroupDialog.h
	ui/CreateGroupDialog.h
)
set(CLIENT_UI_UIS
	ui/MainWindow.ui
	ui/LoginDialog.ui
)

# 网络层源文件
set(CLIENT_NETWORK_SRCS
	network/NetworkClient.cpp
)
set(CLIENT_NETWORK_HDRS
	network/NetworkClient.h
)

add_executable(ChatClient 
	main.cpp 
	${CLIENT_UI_SRCS} 
	${CLIENT_UI_HDRS} 
	${CLIENT_UI_UIS}
	${CLIENT_NETWORK_SRCS}
	${CLIENT_NETWORK_HDRS}
)

# 包含thirdparty和Qt UI生成头文件路径
target_include_directories(ChatClient PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/ui
	${CMAKE_CURRENT_SOURCE_DIR}/network
	${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty
	${CMAKE_CURRENT_BINARY_DIR} # Qt生成的ui_*.h头文件在build目录
	${CMAKE_CURRENT_SOURCE_DIR}/../../include/server
	${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
target_link_libraries(ChatClient PRIVATE pthread ${QT_LIBS})

install(TARGETS ChatClient RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})