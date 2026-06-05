#!/bin/bash
#
# 准备 client/include 与 client/lib。
#
# 背景：C 与 C++ 客户端共用同一套 include/ 与 lib/，差别只在于 C 侧只 include SessionC.h。
# 头文件位于：iotdb-client/client-cpp/target/client-cpp-*-SNAPSHOT-cpp-linux-x86_64/include
# 库文件位于：iotdb-client/client-cpp/target/client-cpp-*-SNAPSHOT-cpp-linux-x86_64/lib
# 另需 gtest 头文件与静态库（测试框架）。
#
# 可用环境变量覆盖默认路径：
#   CPP_CLIENT_DIR  指向 client-cpp-*-cpp-linux-x86_64 目录
#   GTEST_DIR       指向含 include/gtest 与 lib/libgtest.a 的目录

set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"

CPP_CLIENT_DIR="${CPP_CLIENT_DIR:-$(ls -d /data/iotdb-test/tool/timechodb-master/iotdb-client/client-cpp/target/client-cpp-*-cpp-linux-x86_64 2>/dev/null | head -1)}"
GTEST_DIR="${GTEST_DIR:-/data/iotdb-test/safe/client}"

echo "CPP_CLIENT_DIR = ${CPP_CLIENT_DIR}"
echo "GTEST_DIR      = ${GTEST_DIR}"

if [ -z "${CPP_CLIENT_DIR}" ] || [ ! -d "${CPP_CLIENT_DIR}/include" ]; then
    echo "!!! 未找到 C++ 客户端解压包，请设置 CPP_CLIENT_DIR"
    exit 1
fi

mkdir -p "${ROOT}/client/include" "${ROOT}/client/lib"

# 1) 唯一的 IoTDB 头文件 SessionC.h
cp -f "${CPP_CLIENT_DIR}/include/SessionC.h" "${ROOT}/client/include/"

# 2) gtest 头文件
cp -rf "${GTEST_DIR}/include/gtest" "${ROOT}/client/include/"

# 3) 运行库（导出 ts_* C 符号的 libiotdb_session.so 必须来自含 SessionC 的构建）
cp -f "${CPP_CLIENT_DIR}/lib/libiotdb_session.so" "${ROOT}/client/lib/"
cp -f "${CPP_CLIENT_DIR}/lib/libthrift.a"        "${ROOT}/client/lib/"

# 4) gtest 静态库
cp -f "${GTEST_DIR}/lib/libgtest.a" "${ROOT}/client/lib/"

echo "校验 libiotdb_session.so 是否导出 ts_session_new ..."
if nm -D "${ROOT}/client/lib/libiotdb_session.so" 2>/dev/null | grep -q " ts_session_new"; then
    echo "OK: 含 SessionC C 符号"
else
    echo "!!! 警告: libiotdb_session.so 未导出 ts_session_new，链接将失败。"
    echo "    请确认使用的是包含 SessionC 的 client-cpp 构建产物。"
fi

echo "完成。client/include:"
ls "${ROOT}/client/include"
echo "client/lib:"
ls "${ROOT}/client/lib"
