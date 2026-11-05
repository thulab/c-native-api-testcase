#!/bin/sh
# ============================================================================
# c-native-api-testcase 的 PLM 统一入口
#
# 测试框架：GoogleTest
# PLM 调用：sh start.sh case '<CASE_ID>'   （cwd = 仓库根）
# 产物：$PLM_REPORT_DIR/case-<ID>.xml（JUnit XML，每个 testcase 带 case_id）
#
# 数据库连接通过环境变量覆盖（不设置时使用默认值）：
#   IOTDB_HOST / IOTDB_PORT / IOTDB_USER / IOTDB_PASSWORD
# PLM 的 session-c 不注入 IOTDB_HOST/IOTDB_PORT，默认连执行机本地 127.0.0.1:6667；
# 手工运行或需要其他地址时，设置上述环境变量即可。
# ============================================================================
set -u

REPO_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd) || exit 1
cd "$REPO_ROOT" || exit 1

REPORTS_DIR="${PLM_REPORT_DIR:-reports}"
AUTOMATION_TYPE="${PLM_AUTOMATION_TYPE:-session-c}"
PYTHON="${PYTHON:-python3}"
BIN="$REPO_ROOT/build/test/main"

usage() {
    cat >&2 <<'EOF'
Usage: sh start.sh <command> [target]
  prepare              编译 C 测试程序（需先准备 client/include、client/lib）
  list-cases --json    输出用例清单 JSON（stdout 仅 JSON）
  case <case-id>       执行单条用例
  cases <id1,id2>      批量执行（兼容保留；PLM 只逐条派发）
  help                 帮助
EOF
}

safe_name() {
    printf '%s' "$1" | tr '/' '_'
}

prepare() {
    if [ ! -f "$REPO_ROOT/client/include/SessionC.h" ] || [ ! -f "$REPO_ROOT/client/lib/libiotdb_session.so" ]; then
        echo "[plm] 缺少 C 客户端依赖，请先准备 client/include/SessionC.h 与 client/lib/libiotdb_session.so（见 README）" >&2
        return 1
    fi
    if [ ! -f "$REPO_ROOT/client/lib/libgtest.a" ]; then
        echo "[plm] 缺少 gtest 静态库 client/lib/libgtest.a（见 README）" >&2
        return 1
    fi
    "$REPO_ROOT/compile.sh" || return 1
    [ -x "$BIN" ] || { echo "[plm] 编译后未找到 $BIN" >&2; return 1; }
    return 0
}

# C 测试程序连执行机本地 TimechoDB（默认 127.0.0.1:6667）。
# PLM 的 session-c 类型不创建 K8s 实例、不注入 IOTDB_HOST/IOTDB_PORT；
# 如需指定其他地址，设置 IOTDB_HOST / IOTDB_PORT 即可。
list_cases() {
    "$PYTHON" scripts/plm/plm_cases.py list --automation-type "$AUTOMATION_TYPE"
}

case_one() {
    [ -z "${1:-}" ] && { usage; return 2; }
    target=$("$PYTHON" scripts/plm/plm_cases.py lookup "$1") || return 5
    [ -x "$BIN" ] || { echo "[plm] 测试程序未编译，请先执行 sh start.sh prepare" >&2; return 1; }

    case "$REPORTS_DIR" in
        /*) : ;;
        *) REPORTS_DIR="$REPO_ROOT/$REPORTS_DIR" ;;
    esac
    mkdir -p "$REPORTS_DIR"
    final="$REPORTS_DIR/case-$(safe_name "$1").xml"
    raw="$REPORTS_DIR/.gtest-$(safe_name "$1").xml"
    rm -f -- "$final" "$raw"

    "$BIN" --gtest_filter="$target" --gtest_output="xml:$raw"
    rc=$?

    if ! "$PYTHON" scripts/plm/plm_cases.py junit-post "$raw" "$final" "$1"; then
        rm -f -- "$raw" "$final"
        return 1
    fi
    rm -f -- "$raw"
    return $rc
}

cases_batch() {
    [ -z "${1:-}" ] && { usage; return 2; }
    ok=0
    old_ifs=$IFS
    IFS=','
    for raw in $1; do
        id=$(printf '%s' "$raw" | tr -d '[:space:]')
        [ -z "$id" ] && continue
        case_one "$id" || ok=1
    done
    IFS=$old_ifs
    return $ok
}

command="${1:-help}"
target="${2:-}"
case "$command" in
    prepare) prepare ;;
    list-cases)
        [ "$target" = "--json" ] || { usage; exit 2; }
        list_cases
        ;;
    case) case_one "$target" ;;
    cases) cases_batch "$target" ;;
    help|-h|--help) usage ;;
    *) usage; exit 2 ;;
esac
