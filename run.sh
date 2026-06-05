#!/bin/bash
# 执行脚本：运行 SessionC 自动化测试并输出 JSON 报告

ROOT="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="${ROOT}/client/lib:${LD_LIBRARY_PATH}"

if [ -x "./build/test/main" ]; then
    echo "/******* Start SessionC test ********/"
    cd ./build/test
    ./main --gtest_output="json:c_session_test_report.json" "$@"
else
    echo "!!! 错误: build/test/main 不存在或不可执行，请先执行 ./compile.sh"
    exit 1
fi
