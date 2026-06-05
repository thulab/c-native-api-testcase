放置库文件：`libiotdb_session.so`、`libthrift.a`、`libgtest.a`。
由根目录 `./setup_client.sh` 自动拷贝。注意 `libiotdb_session.so` 必须来自包含
SessionC 的 client-cpp 构建（导出 `ts_*` C 符号）。
