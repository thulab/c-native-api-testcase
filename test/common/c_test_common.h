/*
 * SessionC 自动化测试 - 公共头文件
 *
 * 约定（与需求文档 V2.0.10.1-接口模块-SessionC接口 一致）：
 *   - 测试代码以 C++ 编译（使用 gtest 框架），但访问 IoTDB 的能力只通过唯一头文件 SessionC.h；
 *     不引用 Session.h / TableSession.h 等任何其它客户端头文件。
 *   - 连接参数集中在此处，便于切换环境。
 */
#ifndef C_TEST_COMMON_H
#define C_TEST_COMMON_H

#include "SessionC.h"      // 唯一的 IoTDB 头文件
#include "gtest/gtest.h"   // 测试框架

#include <cstdlib>
#include <cstring>
#include <string>

namespace ctest {

/* ---------------- 连接参数（环境变量优先） ----------------
 *
 * 优先级：IOTDB_HOST / IOTDB_PORT / IOTDB_USER / IOTDB_PASSWORD > 默认值。
 * PLM 的 session-c 不注入 IOTDB_HOST/IOTDB_PORT，默认连执行机本地 127.0.0.1:6667；
 * 手工运行或需要其他地址时设置上述环境变量。账号口令默认 TimechoDB 的 root/TimechoDB@2021。
 * 独立运行（如 111 节点）如需 root/root，设置 IOTDB_USER=root IOTDB_PASSWORD=root 即可。
 */
inline const char* envOr(const char* name, const char* fallback) {
    const char* value = std::getenv(name);
    return (value != nullptr && value[0] != '\0') ? value : fallback;
}

inline int envPortOr(const char* name, int fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') return fallback;
    return std::atoi(value);
}

static const char* kHost = envOr("IOTDB_HOST", "127.0.0.1");
static const int   kPort = envPortOr("IOTDB_PORT", 6667);
static const char* kUser = envOr("IOTDB_USER", "root");
static const char* kPass = envOr("IOTDB_PASSWORD", "TimechoDB@2021");

static const char* kWrongPass = "wrong_password_for_test";
static const char* kWrongUser = "no_such_user_for_test";

/* ---------------- 树模型会话辅助 ---------------- */

// 新建并打开一个树模型会话；失败时通过 gtest 断言报错并返回 nullptr。
inline CSession* newOpenTreeSession() {
    CSession* s = ts_session_new(kHost, kPort, kUser, kPass);
    EXPECT_NE(s, nullptr) << "ts_session_new 返回空: " << ts_get_last_error();
    if (!s) return nullptr;
    TsStatus st = ts_session_open(s);
    EXPECT_EQ(st, TS_OK) << "ts_session_open 失败: " << ts_get_last_error();
    if (st != TS_OK) {
        ts_session_destroy(s);
        return nullptr;
    }
    return s;
}

// 关闭并销毁树模型会话（对 nullptr 安全）。
inline void closeDestroyTree(CSession* s) {
    if (!s) return;
    ts_session_close(s);
    ts_session_destroy(s);
}

inline void dropTimeseriesIfExists(CSession* s, const char* path) {
    bool exists = false;
    if (ts_session_check_timeseries_exists(s, path, &exists) == TS_OK && exists) {
        ts_session_delete_timeseries(s, path);
    }
}

inline void ensureTimeseries(CSession* s, const char* path, TSDataType_C type,
                             TSEncoding_C enc, TSCompressionType_C comp) {
    dropTimeseriesIfExists(s, path);
    EXPECT_EQ(ts_session_create_timeseries(s, path, type, enc, comp), TS_OK)
        << "create timeseries 失败 " << path << ": " << ts_get_last_error();
}

inline void dropDatabaseIfExists(CSession* s, const char* db) {
    // 删除不存在的库会返回非 OK，这里忽略返回值仅用于清场。
    (void)ts_session_delete_database(s, db);
}

// 执行查询并统计行数（逐行 destroy，验证迭代/资源释放）。
inline int treeQueryRowCount(CSession* s, const char* sql, int fetchSize = 1024) {
    CSessionDataSet* ds = nullptr;
    EXPECT_EQ(ts_session_execute_query(s, sql, &ds), TS_OK)
        << "query 失败: " << sql << " : " << ts_get_last_error();
    EXPECT_NE(ds, nullptr);
    if (!ds) return -1;
    ts_dataset_set_fetch_size(ds, fetchSize);
    int n = 0;
    while (ts_dataset_has_next(ds)) {
        CRowRecord* r = ts_dataset_next(ds);
        if (!r) break;
        ++n;
        ts_row_record_destroy(r);
    }
    ts_dataset_destroy(ds);
    return n;
}

/* ---------------- 表模型会话辅助 ---------------- */

// 新建并打开一个表模型会话（database 默认为空字符串）。
inline CTableSession* newOpenTableSession(const char* database = "") {
    CTableSession* s = ts_table_session_new(kHost, kPort, kUser, kPass, database);
    EXPECT_NE(s, nullptr) << "ts_table_session_new 返回空: " << ts_get_last_error();
    if (!s) return nullptr;
    TsStatus st = ts_table_session_open(s);
    EXPECT_EQ(st, TS_OK) << "ts_table_session_open 失败: " << ts_get_last_error();
    if (st != TS_OK) {
        ts_table_session_destroy(s);
        return nullptr;
    }
    return s;
}

inline void closeDestroyTable(CTableSession* s) {
    if (!s) return;
    ts_table_session_close(s);
    ts_table_session_destroy(s);
}

// 在表模型会话上准备一个干净的数据库并 USE 之。
inline void tablePrepareDatabase(CTableSession* s, const char* db) {
    std::string sql;
    sql = std::string("DROP DATABASE IF EXISTS ") + db;
    ts_table_session_execute_non_query(s, sql.c_str());
    sql = std::string("CREATE DATABASE ") + db;
    EXPECT_EQ(ts_table_session_execute_non_query(s, sql.c_str()), TS_OK)
        << "CREATE DATABASE 失败: " << ts_get_last_error();
    sql = std::string("USE \"") + db + "\"";
    EXPECT_EQ(ts_table_session_execute_non_query(s, sql.c_str()), TS_OK)
        << "USE 失败: " << ts_get_last_error();
}

inline int tableQueryRowCount(CTableSession* s, const char* sql, int fetchSize = 1024) {
    CSessionDataSet* ds = nullptr;
    EXPECT_EQ(ts_table_session_execute_query(s, sql, &ds), TS_OK)
        << "table query 失败: " << sql << " : " << ts_get_last_error();
    EXPECT_NE(ds, nullptr);
    if (!ds) return -1;
    ts_dataset_set_fetch_size(ds, fetchSize);
    int n = 0;
    while (ts_dataset_has_next(ds)) {
        CRowRecord* r = ts_dataset_next(ds);
        if (!r) break;
        ++n;
        ts_row_record_destroy(r);
    }
    ts_dataset_destroy(ds);
    return n;
}

}  // namespace ctest

#endif  // C_TEST_COMMON_H
