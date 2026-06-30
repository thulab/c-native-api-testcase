/*
 * 树模型 - 查询、结果集、行记录取值及其异常
 * 覆盖用例: 48-64, 125-139
 */
#include "c_test_common.h"

using namespace ctest;

namespace {
const char* DEV = "root.c_query.d1";

// 准备 3 条 INT64 序列并写入 [0,100) 行。
void prepareData(CSession* s, int rows = 100) {
    const char* p1 = "root.c_query.d1.s1";
    const char* p2 = "root.c_query.d1.s2";
    const char* p3 = "root.c_query.d1.s3";
    ensureTimeseries(s, p1, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ensureTimeseries(s, p2, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ensureTimeseries(s, p3, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* meas[] = {"s1", "s2", "s3"};
    for (int64_t t = 0; t < rows; t++) {
        const char* vals[] = {"1", "2", "3"};
        ts_session_insert_record_str(s, DEV, t, 3, meas, vals);
    }
}
void cleanData(CSession* s) {
    const char* paths[] = {"root.c_query.d1.s1", "root.c_query.d1.s2", "root.c_query.d1.s3"};
    ts_session_delete_timeseries_batch(s, paths, 3);
}
}  // namespace

// 用例48: execute_query 普通查询
TEST(TreeQuery, Case48_ExecuteQuery) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s);
    EXPECT_EQ(treeQueryRowCount(s, "select s1,s2,s3 from root.c_query.d1"), 100);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例49: execute_query_with_timeout
TEST(TreeQuery, Case49_QueryWithTimeout) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query_with_timeout(s, "select s1 from root.c_query.d1", 60000, &ds), TS_OK);
    ASSERT_NE(ds, nullptr);
    int n = 0;
    while (ts_dataset_has_next(ds)) { CRowRecord* r = ts_dataset_next(ds); if (!r) break; n++; ts_row_record_destroy(r); }
    EXPECT_EQ(n, 100);
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例50: execute_non_query 执行 insert/DDL
TEST(TreeQuery, Case50_ExecuteNonQuery) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_query.d1.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    EXPECT_EQ(ts_session_execute_non_query(s, "insert into root.c_query.d1(timestamp,s1) values(200,10)"), TS_OK);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_query.d1 where time=200", &ds), TS_OK);
    ASSERT_TRUE(ts_dataset_has_next(ds));
    CRowRecord* r = ts_dataset_next(ds);
    EXPECT_EQ(ts_row_record_get_int64(r, 0), 10);
    ts_row_record_destroy(r);
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例51: raw_data_query 正常时间范围
TEST(TreeQuery, Case51_RawDataQuery) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 50);
    const char* paths[] = {"root.c_query.d1.s1", "root.c_query.d1.s2", "root.c_query.d1.s3"};
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_raw_data_query(s, 3, paths, 0, 50, &ds), TS_OK);
    int n = 0;
    while (ts_dataset_has_next(ds)) { CRowRecord* r = ts_dataset_next(ds); if (!r) break; n++; ts_row_record_destroy(r); }
    EXPECT_EQ(n, 50);
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例52: raw_data_query 负时间戳范围
TEST(TreeQuery, Case52_RawDataQueryNegativeTime) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_query.d1.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* meas[] = {"s1"};
    for (int64_t t = -5; t < 5; t++) { const char* v[] = {"1"}; ts_session_insert_record_str(s, DEV, t, 1, meas, v); }
    const char* paths[] = {"root.c_query.d1.s1"};
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_raw_data_query(s, 1, paths, -5, 5, &ds), TS_OK);
    int n = 0;
    while (ts_dataset_has_next(ds)) { CRowRecord* r = ts_dataset_next(ds); if (!r) break; n++; ts_row_record_destroy(r); }
    EXPECT_GE(n, 1);  // 至少能读回负时间戳数据
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例53: raw_data_query 包含空值列（部分列在某时间点为空）
TEST(TreeQuery, Case53_RawDataQueryWithNull) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_query.d1.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ensureTimeseries(s, "root.c_query.d1.s2", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* m1[] = {"s1"};
    const char* v1[] = {"1"};
    ts_session_insert_record_str(s, DEV, 1, 1, m1, v1);   // 仅 s1，s2 为空
    const char* m2[] = {"s2"};
    const char* v2[] = {"2"};
    ts_session_insert_record_str(s, DEV, 2, 1, m2, v2);   // 仅 s2，s1 为空
    const char* paths[] = {"root.c_query.d1.s1", "root.c_query.d1.s2"};
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_raw_data_query(s, 2, paths, 0, 10, &ds), TS_OK);
    bool sawNull = false;
    while (ts_dataset_has_next(ds)) {
        CRowRecord* r = ts_dataset_next(ds);
        if (!r) break;
        int fc = ts_row_record_get_field_count(r);
        for (int i = 0; i < fc; i++) if (ts_row_record_is_null(r, i)) sawNull = true;
        ts_row_record_destroy(r);
    }
    EXPECT_TRUE(sawNull);
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例54: last_data_query 不带 lastTime
TEST(TreeQuery, Case54_LastDataQuery) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 10);
    const char* paths[] = {"root.c_query.d1.s1", "root.c_query.d1.s2"};
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_last_data_query(s, 2, paths, &ds), TS_OK);
    ASSERT_NE(ds, nullptr);
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例55: last_data_query_with_time 带 lastTime
TEST(TreeQuery, Case55_LastDataQueryWithTime) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 10);
    const char* paths[] = {"root.c_query.d1.s1"};
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_last_data_query_with_time(s, 1, paths, 5, &ds), TS_OK);
    ASSERT_NE(ds, nullptr);
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例56: last_data_query_with_time future 时间无结果
TEST(TreeQuery, Case56_LastDataQueryFutureEmpty) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 10);
    const char* paths[] = {"root.c_query.d1.s1"};
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_last_data_query_with_time(s, 1, paths, 999999999LL, &ds), TS_OK);
    ASSERT_NE(ds, nullptr);
    EXPECT_FALSE(ts_dataset_has_next(ds));
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例57: dataset_get_column_count
TEST(TreeQuery, Case57_ColumnCount) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 1);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1,s2,s3 from root.c_query.d1", &ds), TS_OK);
    EXPECT_EQ(ts_dataset_get_column_count(ds), 4);  // Time + s1 + s2 + s3
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例58: dataset_get_column_name / column_type
TEST(TreeQuery, Case58_ColumnNameType) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 1);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1,s2,s3 from root.c_query.d1", &ds), TS_OK);
    int n = ts_dataset_get_column_count(ds);
    EXPECT_STREQ(ts_dataset_get_column_name(ds, 0), "Time");
    for (int i = 0; i < n; i++) {
        const char* cn = ts_dataset_get_column_name(ds, i);
        const char* ct = ts_dataset_get_column_type(ds, i);
        EXPECT_NE(cn, nullptr);
        EXPECT_NE(ct, nullptr);
        EXPECT_GT(strlen(ct), 0u);
    }
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例59: set_fetch_size 大于 1 的批大小
TEST(TreeQuery, Case59_SetFetchSize) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 100);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_query.d1", &ds), TS_OK);
    ts_dataset_set_fetch_size(ds, 1024);
    int n = 0;
    while (ts_dataset_has_next(ds)) { CRowRecord* r = ts_dataset_next(ds); if (!r) break; n++; ts_row_record_destroy(r); }
    EXPECT_EQ(n, 100);
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

/* ---------------- RowRecord 取值（多类型） ---------------- */

namespace {
const char* MDEV = "root.c_query.dm";
void prepareMultiType(CSession* s) {
    const char* pb = "root.c_query.dm.sb";
    const char* pi = "root.c_query.dm.si";
    const char* pl = "root.c_query.dm.sl";
    const char* pf = "root.c_query.dm.sf";
    const char* pd = "root.c_query.dm.sd";
    const char* pt = "root.c_query.dm.st";
    dropTimeseriesIfExists(s, pb); dropTimeseriesIfExists(s, pi); dropTimeseriesIfExists(s, pl);
    dropTimeseriesIfExists(s, pf); dropTimeseriesIfExists(s, pd); dropTimeseriesIfExists(s, pt);
    ts_session_create_timeseries(s, pb, TS_TYPE_BOOLEAN, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ts_session_create_timeseries(s, pi, TS_TYPE_INT32, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ts_session_create_timeseries(s, pl, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ts_session_create_timeseries(s, pf, TS_TYPE_FLOAT, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ts_session_create_timeseries(s, pd, TS_TYPE_DOUBLE, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ts_session_create_timeseries(s, pt, TS_TYPE_TEXT, TS_ENCODING_PLAIN, TS_COMPRESSION_SNAPPY);
    const char* names[] = {"sb", "si", "sl", "sf", "sd", "st"};
    TSDataType_C types[] = {TS_TYPE_BOOLEAN, TS_TYPE_INT32, TS_TYPE_INT64, TS_TYPE_FLOAT, TS_TYPE_DOUBLE, TS_TYPE_TEXT};
    bool bv = true; int32_t iv = 42; int64_t lv = 100; float fv = 2.5f; double dv = 3.25; const char* tv = "hi";
    const void* vals[] = {&bv, &iv, &lv, &fv, &dv, tv};
    ts_session_insert_record(s, MDEV, 500, 6, names, types, vals);
}
void cleanMultiType(CSession* s) {
    const char* paths[] = {"root.c_query.dm.sb","root.c_query.dm.si","root.c_query.dm.sl",
                           "root.c_query.dm.sf","root.c_query.dm.sd","root.c_query.dm.st"};
    ts_session_delete_timeseries_batch(s, paths, 6);
}
}  // namespace

// 用例60: row timestamp + field_count
TEST(TreeQuery, Case60_RowTimestampFieldCount) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareMultiType(s);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select sb,si,sl,sf,sd,st from root.c_query.dm where time=500", &ds), TS_OK);
    ASSERT_TRUE(ts_dataset_has_next(ds));
    CRowRecord* r = ts_dataset_next(ds);
    EXPECT_EQ(ts_row_record_get_timestamp(r), 500);
    EXPECT_EQ(ts_row_record_get_field_count(r), 6);
    ts_row_record_destroy(r);
    ts_dataset_destroy(ds);
    cleanMultiType(s);
    closeDestroyTree(s);
}

// 用例61: row get bool/int32/int64/float/double/string
TEST(TreeQuery, Case61_RowTypedGetters) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareMultiType(s);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select sb,si,sl,sf,sd,st from root.c_query.dm where time=500", &ds), TS_OK);
    ASSERT_TRUE(ts_dataset_has_next(ds));
    CRowRecord* r = ts_dataset_next(ds);
    EXPECT_EQ(ts_row_record_get_bool(r, 0), true);
    EXPECT_EQ(ts_row_record_get_int32(r, 1), 42);
    EXPECT_EQ(ts_row_record_get_int64(r, 2), 100);
    EXPECT_NEAR(ts_row_record_get_float(r, 3), 2.5f, 1e-4);
    EXPECT_NEAR(ts_row_record_get_double(r, 4), 3.25, 1e-9);
    EXPECT_STREQ(ts_row_record_get_string(r, 5), "hi");
    ts_row_record_destroy(r);
    ts_dataset_destroy(ds);
    cleanMultiType(s);
    closeDestroyTree(s);
}

// 用例62: row get_data_type
TEST(TreeQuery, Case62_RowDataType) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareMultiType(s);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select sb,si,sl,sf,sd,st from root.c_query.dm where time=500", &ds), TS_OK);
    ASSERT_TRUE(ts_dataset_has_next(ds));
    CRowRecord* r = ts_dataset_next(ds);
    EXPECT_EQ(ts_row_record_get_data_type(r, 0), TS_TYPE_BOOLEAN);
    EXPECT_EQ(ts_row_record_get_data_type(r, 1), TS_TYPE_INT32);
    EXPECT_EQ(ts_row_record_get_data_type(r, 2), TS_TYPE_INT64);
    ts_row_record_destroy(r);
    ts_dataset_destroy(ds);
    cleanMultiType(s);
    closeDestroyTree(s);
}

// 用例63: row is_null 空字段识别
TEST(TreeQuery, Case63_RowIsNull) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_query.dn.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ensureTimeseries(s, "root.c_query.dn.s2", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* m[] = {"s1"};
    const char* v[] = {"1"};
    ts_session_insert_record_str(s, "root.c_query.dn", 1, 1, m, v);  // s2 缺失
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1,s2 from root.c_query.dn where time=1", &ds), TS_OK);
    ASSERT_TRUE(ts_dataset_has_next(ds));
    CRowRecord* r = ts_dataset_next(ds);
    EXPECT_FALSE(ts_row_record_is_null(r, 0));
    EXPECT_TRUE(ts_row_record_is_null(r, 1));
    ts_row_record_destroy(r);
    ts_dataset_destroy(ds);
    const char* paths[] = {"root.c_query.dn.s1", "root.c_query.dn.s2"};
    ts_session_delete_timeseries_batch(s, paths, 2);
    closeDestroyTree(s);
}

/* ---------------- 查询与结果集异常 ---------------- */

// 用例125: 语法错误 SQL -> 非 TS_OK
TEST(TreeQueryError, Case125_SyntaxError) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    CSessionDataSet* ds = nullptr;
    EXPECT_NE(ts_session_execute_query(s, "selec * fro xxx", &ds), TS_OK);
    EXPECT_GT(strlen(ts_get_last_error()), 0u);
    closeDestroyTree(s);
}

// 用例126: 空结果集
TEST(TreeQueryError, Case126_EmptyResult) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_query.d1.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_query.d1 where time=99999999", &ds), TS_OK);
    ASSERT_NE(ds, nullptr);
    EXPECT_FALSE(ts_dataset_has_next(ds));
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例127: execute_query dataSet 出参为 NULL -> 非 TS_OK
TEST(TreeQueryError, Case127_QueryNullOut) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    EXPECT_NE(ts_session_execute_query(s, "show databases", nullptr), TS_OK);
    closeDestroyTree(s);
}

// 用例128: 非法非查询 SQL -> 非 TS_OK
TEST(TreeQueryError, Case128_NonQueryIllegal) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    EXPECT_NE(ts_session_execute_non_query(s, "create timeseries"), TS_OK);
    EXPECT_GT(strlen(ts_get_last_error()), 0u);
    closeDestroyTree(s);
}

// 用例129: raw_data_query pathCount=0
TEST(TreeQueryError, Case129_RawDataQueryZeroPaths) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    CSessionDataSet* ds = nullptr;
    TsStatus st = ts_session_execute_raw_data_query(s, 0, nullptr, 0, 100, &ds);
    (void)st;  // 非 TS_OK 或空结果集，均不崩溃
    if (ds) ts_dataset_destroy(ds);
    closeDestroyTree(s);
    SUCCEED();
}

// 用例130: raw_data_query startTime>endTime
TEST(TreeQueryError, Case130_RawDataQueryReversedTime) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 10);
    const char* paths[] = {"root.c_query.d1.s1"};
    CSessionDataSet* ds = nullptr;
    TsStatus st = ts_session_execute_raw_data_query(s, 1, paths, 100, 0, &ds);
    if (st == TS_OK && ds) {
        EXPECT_FALSE(ts_dataset_has_next(ds));  // 反转区间应为空
    }
    if (ds) ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
    SUCCEED();
}

// 用例131: last_data_query 不存在 path -> 空集
TEST(TreeQueryError, Case131_LastDataQueryNonexist) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* paths[] = {"root.c_query.nope.s1"};
    CSessionDataSet* ds = nullptr;
    TsStatus st = ts_session_execute_last_data_query(s, 1, paths, &ds);
    if (st == TS_OK && ds) EXPECT_FALSE(ts_dataset_has_next(ds));
    if (ds) ts_dataset_destroy(ds);
    closeDestroyTree(s);
    SUCCEED();
}

// 用例132: column_name 越界 -> 返回空（实现返回 ""），不崩溃
TEST(TreeQueryError, Case132_ColumnNameOutOfRange) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 1);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_query.d1", &ds), TS_OK);
    int n = ts_dataset_get_column_count(ds);
    const char* name = ts_dataset_get_column_name(ds, n + 5);
    EXPECT_TRUE(name == nullptr || name[0] == '\0');
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例133: column_type 越界（index=-1）-> 返回空，不崩溃
TEST(TreeQueryError, Case133_ColumnTypeOutOfRange) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 1);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_query.d1", &ds), TS_OK);
    const char* t = ts_dataset_get_column_type(ds, -1);
    EXPECT_TRUE(t == nullptr || t[0] == '\0');
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例134: 结果集耗尽后再 next -> NULL
TEST(TreeQueryError, Case134_NextAfterExhausted) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 3);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_query.d1", &ds), TS_OK);
    while (ts_dataset_has_next(ds)) { CRowRecord* r = ts_dataset_next(ds); if (r) ts_row_record_destroy(r); }
    EXPECT_FALSE(ts_dataset_has_next(ds));
    EXPECT_EQ(ts_dataset_next(ds), nullptr);
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例135: dataSet use-after-free 防御（以 NULL 句柄验证不崩溃）
TEST(TreeQueryError, Case135_DatasetUseAfterFreeDefense) {
    EXPECT_FALSE(ts_dataset_has_next(nullptr));
    EXPECT_EQ(ts_dataset_next(nullptr), nullptr);
    SUCCEED();
}

// 用例136: row get_data_type 越界 -> TS_TYPE_INVALID
TEST(TreeQueryError, Case136_DataTypeOutOfRange) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 1);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_query.d1", &ds), TS_OK);
    ASSERT_TRUE(ts_dataset_has_next(ds));
    CRowRecord* r = ts_dataset_next(ds);
    EXPECT_EQ(ts_row_record_get_data_type(r, 999), TS_TYPE_INVALID);
    ts_row_record_destroy(r);
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例137: getter 类型不匹配（对文本列调用 int getter）-> 可控，不崩溃
TEST(TreeQueryError, Case137_GetterTypeMismatch) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareMultiType(s);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select st from root.c_query.dm where time=500", &ds), TS_OK);
    ASSERT_TRUE(ts_dataset_has_next(ds));
    CRowRecord* r = ts_dataset_next(ds);
    int32_t v = ts_row_record_get_int32(r, 0);  // 文本列用 int getter
    (void)v;  // 返回值无效但不崩溃
    ts_row_record_destroy(r);
    ts_dataset_destroy(ds);
    cleanMultiType(s);
    closeDestroyTree(s);
    SUCCEED();
}

// 用例138: row is_null 越界 -> 返回 true，不崩溃
TEST(TreeQueryError, Case138_IsNullOutOfRange) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 1);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_query.d1", &ds), TS_OK);
    ASSERT_TRUE(ts_dataset_has_next(ds));
    CRowRecord* r = ts_dataset_next(ds);
    EXPECT_TRUE(ts_row_record_is_null(r, 999));
    ts_row_record_destroy(r);
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}

// 用例139: set_fetch_size <= 0 边界
TEST(TreeQueryError, Case139_FetchSizeNonPositive) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareData(s, 20);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_query.d1", &ds), TS_OK);
    ts_dataset_set_fetch_size(ds, 0);  // 非法批大小
    int n = 0;
    while (ts_dataset_has_next(ds)) { CRowRecord* r = ts_dataset_next(ds); if (!r) break; n++; ts_row_record_destroy(r); }
    EXPECT_EQ(n, 20);  // 仍能完整读出（采用默认批大小）
    ts_dataset_destroy(ds);
    cleanData(s);
    closeDestroyTree(s);
}
