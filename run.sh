#!/bin/bash
# 执行脚本：运行 SessionC 自动化测试（test 模式）或 C 示例程序（example 模式）
# 用法：
#   ./run.sh                  # 默认 test 模式：跑全部 gtest 用例，输出 c_session_test_report.json
#   ./run.sh -m test [args]   # 同上；额外参数透传给 gtest（如 --gtest_filter='TreeInsert.*'）
#   ./run.sh -m example       # 跑 example/ 下的 C 示例（需先在根 CMakeLists 启用 example 并重新编译）

ROOT="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="${ROOT}/client/lib:${LD_LIBRARY_PATH}"

mode="test"
while getopts ":m:" opt; do
    case $opt in
        m) mode="$OPTARG" ;;
        *) echo "无效选项: -$OPTARG" >&2; exit 1 ;;
    esac
done
shift $((OPTIND - 1))

run_test() {
    if [ -x "${ROOT}/build/test/main" ]; then
        echo "/******* Start SessionC test ********/"
        cd "${ROOT}/build/test"
        ./main --gtest_output="json:c_session_test_report.json" "$@"
    else
        echo "!!! 错误: build/test/main 不存在或不可执行，请先执行 ./compile.sh"
        exit 1
    fi
}

run_example() {
    local dir="${ROOT}/build/example"
    if [ ! -d "$dir" ]; then
        echo "!!! 错误: ${dir} 不存在。请先在根 CMakeLists.txt 取消 add_subdirectory(\"example\") 注释并重新 ./compile.sh"
        exit 1
    fi
    local found=0
    for exe in tree_example table_example; do
        if [ -x "${dir}/${exe}" ]; then
            echo "/******* Start example: ${exe} ********/"
            "${dir}/${exe}"
            found=1
        fi
    done
    [ $found -eq 0 ] && { echo "!!! 错误: ${dir} 下未找到可执行示例。"; exit 1; }
}

case $mode in
    test)    run_test "$@" ;;
    example) run_example ;;
    *)       echo "无效模式: ${mode}（可选 test 或 example）" >&2; exit 1 ;;
esac
