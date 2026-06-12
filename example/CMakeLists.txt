# 示例程序：仅 include SessionC.h 的可运行 C 示例（树模型 / 表模型）
# 默认不参与测试构建；如需编译，在根 CMakeLists.txt 取消 add_subdirectory("example") 注释后重新 cmake。
# 链接库与 test 目标一致：thrift 已内嵌 libiotdb_session.so，不再外链 libthrift.a。

add_executable(tree_example "tree_example.c")
add_executable(table_example "table_example.c")

foreach(tgt tree_example table_example)
    target_link_libraries(${tgt} iotdb_session dl pthread)
    set_target_properties(${tgt} PROPERTIES
        BUILD_RPATH   "${CMAKE_SOURCE_DIR}/client/lib"
        INSTALL_RPATH "${CMAKE_SOURCE_DIR}/client/lib")
endforeach()
