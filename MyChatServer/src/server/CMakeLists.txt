# 定义了一个SRC_LIST变量，包含了该目录下面的所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

find_path(MUDUO_INCLUDE_DIR muduo/net/TcpServer.h PATHS /usr/include /usr/local/include)
find_path(MYSQL_INCLUDE_DIR mysql/mysql.h PATHS /usr/include /usr/local/include /usr/include/x86_64-linux-gnu)
find_library(MUDUO_NET_LIB muduo_net PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
find_library(MUDUO_BASE_LIB muduo_base PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
find_library(MYSQLCLIENT_LIB mysqlclient PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
find_library(HIREDIS_LIB hiredis PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)

if(MUDUO_INCLUDE_DIR AND MYSQL_INCLUDE_DIR AND MUDUO_NET_LIB AND MUDUO_BASE_LIB AND MYSQLCLIENT_LIB AND HIREDIS_LIB)
	add_executable(ChatServer main.cpp ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

	target_include_directories(ChatServer PRIVATE
		${MUDUO_INCLUDE_DIR}
		${MYSQL_INCLUDE_DIR}
	)

	target_link_libraries(ChatServer PRIVATE
		${MUDUO_NET_LIB}
		${MUDUO_BASE_LIB}
		${MYSQLCLIENT_LIB}
		${HIREDIS_LIB}
		pthread
	)

	install(TARGETS ChatServer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
	message(WARNING "ChatServer dependencies not fully found; skipping ChatServer target. Install muduo/mysqlclient/hiredis dev packages to enable it.")
endif()
