/*
 * 树模型 - Schema 管理（数据库 / 时间序列）及其异常
 * 覆盖用例: 11-21, 103-111
 */
#include "c_test_common.h"

using namespace ctest;

namespace {
// 每个测试独立使用的数据库前缀，避免相互干扰。
const char* DB_A = "root.c_schema_a";
const char* DB_B = "root.c_schema_b";
}  // namespace

/* ====================== 数据库 ====================== */

// 用例11: 创建数据库
TEST(TreeSchema, Case11_CreateDatabase) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    dropDatabaseIfExists(s, DB_A);
    EXPECT_EQ(ts_session_create_database(s, DB_A), TS_OK) << ts_get_last_error();
    EXPECT_EQ(ts_session_delete_database(s, DB_A), TS_OK);
    closeDestroyTree(s);
}

// 用例12: 删除（空）数据库
TEST(TreeSchema, Case12_DeleteDatabase) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    dropDatabaseIfExists(s, DB_A);
    ASSERT_EQ(ts_session_create_database(s, DB_A), TS_OK);
    EXPECT_EQ(ts_session_delete_database(s, DB_A), TS_OK) << ts_get_last_error();
    closeDestroyTree(s);
}

// 用例13: 批量删除数据库
TEST(TreeSchema, Case13_DeleteDatabases) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    dropDatabaseIfExists(s, DB_A);
    dropDatabaseIfExists(s, DB_B);
    ASSERT_EQ(ts_session_create_database(s, DB_A), TS_OK);
    ASSERT_EQ(ts_session_create_database(s, DB_B), TS_OK);
    const char* dbs[] = {DB_A, DB_B};
    EXPECT_EQ(ts_session_delete_databases(s, dbs, 2), TS_OK) << ts_get_last_error();
    closeDestroyTree(s);
}

/* ====================== 时间序列 ====================== */

// 用例14: 创建单条时间序列 + 存在性检查
TEST(TreeSchema, Case14_CreateTimeseries) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* path = "root.c_schema_ts.d1.s1";
    dropTimeseriesIfExists(s, path);
    EXPECT_EQ(ts_session_create_timeseries(s, path, TS_TYPE_INT64, TS_ENCODING_RLE,
                                           TS_COMPRESSION_SNAPPY), TS_OK)
        << ts_get_last_error();
    bool exists = false;
    EXPECT_EQ(ts_session_check_timeseries_exists(s, path, &exists), TS_OK);
    EXPECT_TRUE(exists);
    ts_session_delete_timeseries(s, path);
    closeDestroyTree(s);
}

// 用例15: 扩展建序列（属性/标签/别名）
TEST(TreeSchema, Case15_CreateTimeseriesEx) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* path = "root.c_schema_ts.d1.s_ex";
    dropTimeseriesIfExists(s, path);
    const char* tagKeys[]   = {"tag1"};
    const char* tagVals[]   = {"v1"};
    const char* attrKeys[]  = {"attr1"};
    const char* attrVals[]  = {"a1"};
    EXPECT_EQ(ts_session_create_timeseries_ex(s, path, TS_TYPE_INT64, TS_ENCODING_RLE,
                                              TS_COMPRESSION_SNAPPY,
                                              0, nullptr, nullptr,
                                              1, tagKeys, tagVals,
                                              1, attrKeys, attrVals,
                                              "alias_s_ex"), TS_OK)
        << ts_get_last_error();
    bool exists = false;
    EXPECT_EQ(ts_session_check_timeseries_exists(s, path, &exists), TS_OK);
    EXPECT_TRUE(exists);
    ts_session_delete_timeseries(s, path);
    closeDestroyTree(s);
}

// 用例16: 批量建序列
TEST(TreeSchema, Case16_CreateMultiTimeseries) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* paths[] = {"root.c_schema_ts.d2.s1", "root.c_schema_ts.d2.s2"};
    TSDataType_C types[] = {TS_TYPE_INT64, TS_TYPE_DOUBLE};
    TSEncoding_C encs[]  = {TS_ENCODING_RLE, TS_ENCODING_RLE};
    TSCompressionType_C comps[] = {TS_COMPRESSION_SNAPPY, TS_COMPRESSION_SNAPPY};
    for (auto p : paths) dropTimeseriesIfExists(s, p);
    EXPECT_EQ(ts_session_create_multi_timeseries(s, 2, paths, types, encs, comps), TS_OK)
        << ts_get_last_error();
    EXPECT_EQ(ts_session_delete_timeseries_batch(s, paths, 2), TS_OK);
    closeDestroyTree(s);
}

// 用例17: 创建对齐时间序列
TEST(TreeSchema, Case17_CreateAlignedTimeseries) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* dev = "root.c_schema_al.dev";
    const char* meas[] = {"m1", "m2"};
    TSDataType_C types[] = {TS_TYPE_INT64, TS_TYPE_INT64};
    TSEncoding_C encs[]  = {TS_ENCODING_RLE, TS_ENCODING_RLE};
    TSCompressionType_C comps[] = {TS_COMPRESSION_SNAPPY, TS_COMPRESSION_SNAPPY};
    ts_session_delete_timeseries(s, "root.c_schema_al.dev.m1");
    ts_session_delete_timeseries(s, "root.c_schema_al.dev.m2");
    EXPECT_EQ(ts_session_create_aligned_timeseries(s, dev, 2, meas, types, encs, comps), TS_OK)
        << ts_get_last_error();
    closeDestroyTree(s);
}

// 用例18: 已存在路径 exists=true
TEST(TreeSchema, Case18_CheckExistsTrue) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* path = "root.c_schema_ts.d3.s1";
    ensureTimeseries(s, path, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    bool exists = false;
    EXPECT_EQ(ts_session_check_timeseries_exists(s, path, &exists), TS_OK);
    EXPECT_TRUE(exists);
    ts_session_delete_timeseries(s, path);
    closeDestroyTree(s);
}

// 用例19: 不存在路径 exists=false
TEST(TreeSchema, Case19_CheckExistsFalse) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* path = "root.c_schema_ts.d3.s_none";
    dropTimeseriesIfExists(s, path);
    bool exists = true;
    EXPECT_EQ(ts_session_check_timeseries_exists(s, path, &exists), TS_OK);
    EXPECT_FALSE(exists);
    closeDestroyTree(s);
}

// 用例20: 单条删除后 exists=false
TEST(TreeSchema, Case20_DeleteTimeseries) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* path = "root.c_schema_ts.d4.s1";
    ensureTimeseries(s, path, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    EXPECT_EQ(ts_session_delete_timeseries(s, path), TS_OK);
    bool exists = true;
    EXPECT_EQ(ts_session_check_timeseries_exists(s, path, &exists), TS_OK);
    EXPECT_FALSE(exists);
    closeDestroyTree(s);
}

// 用例21: 批量删除时间序列
TEST(TreeSchema, Case21_DeleteTimeseriesBatch) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* paths[] = {"root.c_schema_ts.d5.s1", "root.c_schema_ts.d5.s2"};
    for (auto p : paths)
        ensureTimeseries(s, p, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    EXPECT_EQ(ts_session_delete_timeseries_batch(s, paths, 2), TS_OK);
    for (auto p : paths) {
        bool exists = true;
        EXPECT_EQ(ts_session_check_timeseries_exists(s, p, &exists), TS_OK);
        EXPECT_FALSE(exists);
    }
    closeDestroyTree(s);
}

/* ====================== Schema 异常 ====================== */

// 用例103: 重复创建同名数据库 -> 非 TS_OK
TEST(TreeSchemaError, Case103_CreateDuplicateDatabase) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    dropDatabaseIfExists(s, DB_A);
    ASSERT_EQ(ts_session_create_database(s, DB_A), TS_OK);
    EXPECT_NE(ts_session_create_database(s, DB_A), TS_OK);
    EXPECT_GT(strlen(ts_get_last_error()), 0u);
    ts_session_delete_database(s, DB_A);
    closeDestroyTree(s);
}

// 用例104: 删除不存在数据库 -> 行为可控（非 TS_OK 或幂等）
TEST(TreeSchemaError, Case104_DeleteNonexistDatabase) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    dropDatabaseIfExists(s, "root.c_schema_none");
    TsStatus st = ts_session_delete_database(s, "root.c_schema_none");
    (void)st;  // 不强约束，仅要求不崩溃
    closeDestroyTree(s);
    SUCCEED();
}

// 用例105: delete_databases count 与数组不一致 -> 非 TS_OK，不越界。
// 说明：C 契约下 count 大于数组长度属调用方未定义行为（会越界读取），无法安全触发；
// 这里以非法 count(-1) 验证实现对非法计数的防御（内部 reserve 抛异常被捕获为非 OK）。
TEST(TreeSchemaError, Case105_DeleteDatabasesCountMismatch) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* dbs[] = {DB_A};  // 数组合法，仅 count 非法
    TsStatus st = ts_session_delete_databases(s, dbs, -1);
    EXPECT_NE(st, TS_OK);
    closeDestroyTree(s);
}

// 用例106: 重复创建同一 timeseries -> 非 TS_OK
TEST(TreeSchemaError, Case106_CreateDuplicateTimeseries) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* path = "root.c_schema_ts.dup.s1";
    ensureTimeseries(s, path, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    EXPECT_NE(ts_session_create_timeseries(s, path, TS_TYPE_INT64, TS_ENCODING_RLE,
                                           TS_COMPRESSION_SNAPPY), TS_OK);
    ts_session_delete_timeseries(s, path);
    closeDestroyTree(s);
}

// 用例107: 非法路径（不以 root. 开头）-> 非 TS_OK
TEST(TreeSchemaError, Case107_IllegalPath) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    EXPECT_NE(ts_session_create_timeseries(s, "invalid.d.s", TS_TYPE_INT64, TS_ENCODING_RLE,
                                           TS_COMPRESSION_SNAPPY), TS_OK);
    EXPECT_GT(strlen(ts_get_last_error()), 0u);
    closeDestroyTree(s);
}

// 用例108: 类型与编码不兼容（BOOLEAN + GORILLA）-> 非 TS_OK
TEST(TreeSchemaError, Case108_IncompatibleTypeEncoding) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* path = "root.c_schema_ts.bad.s1";
    dropTimeseriesIfExists(s, path);
    EXPECT_NE(ts_session_create_timeseries(s, path, TS_TYPE_BOOLEAN, TS_ENCODING_GORILLA,
                                           TS_COMPRESSION_SNAPPY), TS_OK);
    closeDestroyTree(s);
}

// 用例109: 对齐序列数组与 count 不一致 -> 非 TS_OK，不崩溃。
// 说明同用例105：以非法 count(-1) 验证防御，避免越界读取。
TEST(TreeSchemaError, Case109_AlignedCountMismatch) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* dev = "root.c_schema_al.bad";
    const char* meas[] = {"m1"};
    TSDataType_C types[] = {TS_TYPE_INT64};
    TSEncoding_C encs[]  = {TS_ENCODING_RLE};
    TSCompressionType_C comps[] = {TS_COMPRESSION_SNAPPY};
    TsStatus st = ts_session_create_aligned_timeseries(s, dev, -1, meas, types, encs, comps);
    EXPECT_NE(st, TS_OK);
    closeDestroyTree(s);
}

// 用例110: check_timeseries_exists exists 出参为 NULL -> 非 TS_OK
TEST(TreeSchemaError, Case110_CheckExistsNullOut) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    EXPECT_NE(ts_session_check_timeseries_exists(s, "root.c_schema_ts.x.s1", nullptr), TS_OK);
    closeDestroyTree(s);
}

// 用例111: 删除不存在序列 -> 行为可控，不崩溃
TEST(TreeSchemaError, Case111_DeleteNonexistTimeseries) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    TsStatus st = ts_session_delete_timeseries(s, "root.c_schema_ts.none.s1");
    (void)st;
    closeDestroyTree(s);
    SUCCEED();
}
