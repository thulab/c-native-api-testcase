# 测试可执行文件：聚合树模型与表模型全部用例
add_executable(main
    ${CMAKE_SOURCE_DIR}/test/main.cpp
    # 树模型
    ${CMAKE_SOURCE_DIR}/test/tree/tree_lifecycle_test.cpp
    ${CMAKE_SOURCE_DIR}/test/tree/tree_schema_test.cpp
    ${CMAKE_SOURCE_DIR}/test/tree/tree_tablet_test.cpp
    ${CMAKE_SOURCE_DIR}/test/tree/tree_insert_test.cpp
    ${CMAKE_SOURCE_DIR}/test/tree/tree_query_test.cpp
    ${CMAKE_SOURCE_DIR}/test/tree/tree_delete_test.cpp
    # 表模型
    ${CMAKE_SOURCE_DIR}/test/table/table_lifecycle_test.cpp
    ${CMAKE_SOURCE_DIR}/test/table/table_write_query_test.cpp
    ${CMAKE_SOURCE_DIR}/test/table/table_prepared_test.cpp
    # 公共：错误处理/资源/稳定性
    ${CMAKE_SOURCE_DIR}/test/common/error_resource_test.cpp
    ${CMAKE_SOURCE_DIR}/test/common/stability_test.cpp
)

# 测试用公共头文件目录
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/test/common)

# 链接：iotdb_session(.so) + thrift + gtest + 系统库
target_link_libraries(main
    iotdb_session
    "${CMAKE_SOURCE_DIR}/client/lib/libthrift.a"
    "${CMAKE_SOURCE_DIR}/client/lib/libgtest.a"
    dl
    pthread
)

# 运行期定位 libiotdb_session.so
set_target_properties(main PROPERTIES
    BUILD_RPATH "${CMAKE_SOURCE_DIR}/client/lib"
    INSTALL_RPATH "${CMAKE_SOURCE_DIR}/client/lib"
)
