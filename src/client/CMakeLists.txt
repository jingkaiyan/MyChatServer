# 指定生成可执行文件
add_executable(ChatClient main.cpp)
# 添加依赖项
target_link_libraries(ChatClient pthread)