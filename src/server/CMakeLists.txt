#定义SRC_LIST包含了目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./redis REDIS_LIST)
aux_source_directory(./model MODEL_LIST)

#
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)