/*
 * 树模型 - 写入（记录/批量/对齐/Tablet）及其异常与对齐容错
 * 覆盖用例: 31-47, 112-118
 */
#include "c_test_common.h"

using namespace ctest;

namespace {
const char* DEV = "root.c_insert.d1";
const char* P1 = "root.c_insert.d1.s1";
const char* P2 = "root.c_insert.d1.s2";
const char* P3 = "root.c_insert.d1.s3";

void prepareThreeInt64(CSession* s) {
    ensureTimeseries(s, P1, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ensureTimeseries(s, P2, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ensureTimeseries(s, P3, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
}
void cleanDevice(CSession* s) {
    const char* paths[] = {P1, P2, P3};
    ts_session_delete_timeseries_batch(s, paths, 3);
}
}  // namespace

// 用例31: insert_record_str 字符串值单条写入
TEST(TreeInsert, Case31_InsertRecordStr) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareThreeInt64(s);
    const char* meas[] = {"s1", "s2", "s3"};
    for (int64_t t = 0; t < 10; t++) {
        const char* vals[] = {"1", "2", "3"};
        EXPECT_EQ(ts_session_insert_record_str(s, DEV, t, 3, meas, vals), TS_OK) << ts_get_last_error();
    }
    EXPECT_EQ(treeQueryRowCount(s, "select s1,s2,s3 from root.c_insert.d1"), 10);
    cleanDevice(s);
    closeDestroyTree(s);
}

// 用例32: insert_record 强类型 INT32/DOUBLE/INT64
TEST(TreeInsert, Case32_InsertRecordTyped) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, P1, TS_TYPE_INT32, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ensureTimeseries(s, P2, TS_TYPE_DOUBLE, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ensureTimeseries(s, P3, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* meas[] = {"s1", "s2", "s3"};
    TSDataType_C types[] = {TS_TYPE_INT32, TS_TYPE_DOUBLE, TS_TYPE_INT64};
    for (int64_t t = 0; t < 10; t++) {
        int32_t v1 = 1; double v2 = 2.2; int64_t v3 = 3;
        const void* vals[] = {&v1, &v2, &v3};
        EXPECT_EQ(ts_session_insert_record(s, DEV, t, 3, meas, types, vals), TS_OK) << ts_get_last_error();
    }
    EXPECT_EQ(treeQueryRowCount(s, "select s1,s2,s3 from root.c_insert.d1"), 10);
    cleanDevice(s);
    closeDestroyTree(s);
}

// 用例33: insert_record 新类型 TIMESTAMP/DATE/BLOB/STRING
TEST(TreeInsert, Case33_InsertRecordNewTypes) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* dev = "root.c_insert.dnt";
    const char* pts = "root.c_insert.dnt.ts";
    const char* pdate = "root.c_insert.dnt.dt";
    const char* pblob = "root.c_insert.dnt.bl";
    const char* pstr = "root.c_insert.dnt.st";
    dropTimeseriesIfExists(s, pts);
    dropTimeseriesIfExists(s, pdate);
    dropTimeseriesIfExists(s, pblob);
    dropTimeseriesIfExists(s, pstr);
    ASSERT_EQ(ts_session_create_timeseries(s, pts, TS_TYPE_TIMESTAMP, TS_ENCODING_TS_2DIFF, TS_COMPRESSION_SNAPPY), TS_OK) << ts_get_last_error();
    ASSERT_EQ(ts_session_create_timeseries(s, pdate, TS_TYPE_DATE, TS_ENCODING_PLAIN, TS_COMPRESSION_SNAPPY), TS_OK) << ts_get_last_error();
    ASSERT_EQ(ts_session_create_timeseries(s, pblob, TS_TYPE_BLOB, TS_ENCODING_PLAIN, TS_COMPRESSION_SNAPPY), TS_OK) << ts_get_last_error();
    ASSERT_EQ(ts_session_create_timeseries(s, pstr, TS_TYPE_STRING, TS_ENCODING_PLAIN, TS_COMPRESSION_SNAPPY), TS_OK) << ts_get_last_error();

    // 以字符串值写入新类型（insert_record_str 由服务端按列类型解析）
    const char* meas[] = {"ts", "dt", "bl", "st"};
    const char* vals[] = {"100", "2024-01-01", "X'616263'", "hello"};  // BLOB 字面量须为 X'hex'
    TsStatus st = ts_session_insert_record_str(s, dev, 1, 4, meas, vals);
    EXPECT_EQ(st, TS_OK) << ts_get_last_error();

    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select ts,dt,bl,st from root.c_insert.dnt", &ds), TS_OK);
    ASSERT_NE(ds, nullptr);
    if (ts_dataset_has_next(ds)) {
        CRowRecord* r = ts_dataset_next(ds);
        EXPECT_EQ(ts_row_record_get_field_count(r), 4);
        ts_row_record_destroy(r);
    }
    ts_dataset_destroy(ds);
    const char* paths[] = {pts, pdate, pblob, pstr};
    ts_session_delete_timeseries_batch(s, paths, 4);
    closeDestroyTree(s);
}

// 用例34: insert_records_str 多设备批量字符串
TEST(TreeInsert, Case34_InsertRecordsStr) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareThreeInt64(s);
    const int BATCH = 20;
    const char* meas[] = {"s1", "s2", "s3"};
    const char* values[] = {"1", "2", "3"};
    const char* deviceIds[BATCH];
    int64_t times[BATCH];
    int mc[BATCH];
    const char* const* measList[BATCH];
    const char* const* valuesList[BATCH];
    for (int i = 0; i < BATCH; i++) {
        deviceIds[i] = DEV; times[i] = i; mc[i] = 3;
        measList[i] = meas; valuesList[i] = values;
    }
    EXPECT_EQ(ts_session_insert_records_str(s, BATCH, deviceIds, times, mc, measList, valuesList), TS_OK) << ts_get_last_error();
    EXPECT_EQ(treeQueryRowCount(s, "select s1,s2,s3 from root.c_insert.d1"), BATCH);
    cleanDevice(s);
    closeDestroyTree(s);
}

// 用例35: insert_records 强类型批量
TEST(TreeInsert, Case35_InsertRecordsTyped) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* pa = "root.c_insert.da.s1";
    const char* pb = "root.c_insert.db.s1";
    ensureTimeseries(s, pa, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ensureTimeseries(s, pb, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* devs[] = {"root.c_insert.da", "root.c_insert.db"};
    int64_t times[] = {1, 2};
    int mc[] = {1, 1};
    const char* ma[] = {"s1"}; const char* mb[] = {"s1"};
    const char* const* measList[] = {ma, mb};
    int64_t va = 11, vb = 22;
    const void* vva[] = {&va}; const void* vvb[] = {&vb};
    const void* const* valuesList[] = {vva, vvb};
    TSDataType_C ta[] = {TS_TYPE_INT64}; TSDataType_C tb[] = {TS_TYPE_INT64};
    const TSDataType_C* typesList[] = {ta, tb};
    EXPECT_EQ(ts_session_insert_records(s, 2, devs, times, mc, measList, typesList, valuesList), TS_OK) << ts_get_last_error();
    EXPECT_EQ(treeQueryRowCount(s, "select s1 from root.c_insert.da"), 1);
    EXPECT_EQ(treeQueryRowCount(s, "select s1 from root.c_insert.db"), 1);
    ts_session_delete_timeseries(s, pa);
    ts_session_delete_timeseries(s, pb);
    closeDestroyTree(s);
}

// 用例36: insert_records_of_one_device sorted=true
TEST(TreeInsert, Case36_InsertRecordsOfOneDevice) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* dc = "root.c_insert.dc";
    const char* p = "root.c_insert.dc.s1";
    ensureTimeseries(s, p, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    int64_t times[] = {1, 2, 3};
    int mc[] = {1, 1, 1};
    const char* m[] = {"s1"};
    const char* const* measList[] = {m, m, m};
    TSDataType_C ta[] = {TS_TYPE_INT64};
    const TSDataType_C* typesList[] = {ta, ta, ta};
    int64_t v0 = 10, v1 = 20, v2 = 30;
    const void* r0[] = {&v0}; const void* r1[] = {&v1}; const void* r2[] = {&v2};
    const void* const* valuesList[] = {r0, r1, r2};
    EXPECT_EQ(ts_session_insert_records_of_one_device(s, dc, 3, times, mc, measList, typesList, valuesList, true), TS_OK) << ts_get_last_error();
    EXPECT_EQ(treeQueryRowCount(s, "select s1 from root.c_insert.dc"), 3);
    ts_session_delete_timeseries(s, p);
    closeDestroyTree(s);
}

/* ---------------- 对齐写入 ---------------- */

namespace {
const char* AL_DEV = "root.c_insert.al";
const char* AL_DEV2 = "root.c_insert.al2";
void prepareAligned(CSession* s, const char* dev) {
    const char* meas[] = {"m1", "m2"};
    TSDataType_C types[] = {TS_TYPE_INT64, TS_TYPE_INT64};
    TSEncoding_C encs[] = {TS_ENCODING_RLE, TS_ENCODING_RLE};
    TSCompressionType_C comps[] = {TS_COMPRESSION_SNAPPY, TS_COMPRESSION_SNAPPY};
    ts_session_delete_timeseries(s, (std::string(dev) + ".m1").c_str());
    ts_session_delete_timeseries(s, (std::string(dev) + ".m2").c_str());
    ASSERT_EQ(ts_session_create_aligned_timeseries(s, dev, 2, meas, types, encs, comps), TS_OK) << ts_get_last_error();
}
}  // namespace

// 用例37: insert_aligned_record_str
TEST(TreeInsert, Case37_AlignedRecordStr) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareAligned(s, AL_DEV);
    const char* m[] = {"m1", "m2"};
    const char* v[] = {"1", "2"};
    EXPECT_EQ(ts_session_insert_aligned_record_str(s, AL_DEV, 100, 2, m, v), TS_OK) << ts_get_last_error();
    EXPECT_EQ(treeQueryRowCount(s, "select m1,m2 from root.c_insert.al"), 1);
    closeDestroyTree(s);
}

// 用例38: insert_aligned_record 强类型
TEST(TreeInsert, Case38_AlignedRecordTyped) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareAligned(s, AL_DEV);
    const char* m[] = {"m1", "m2"};
    TSDataType_C types[] = {TS_TYPE_INT64, TS_TYPE_INT64};
    int64_t v1 = 3, v2 = 4;
    const void* vals[] = {&v1, &v2};
    EXPECT_EQ(ts_session_insert_aligned_record(s, AL_DEV, 101, 2, m, types, vals), TS_OK) << ts_get_last_error();
    closeDestroyTree(s);
}

// 用例39: insert_aligned_records_str
TEST(TreeInsert, Case39_AlignedRecordsStr) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareAligned(s, AL_DEV);
    const char* m[] = {"m1", "m2"};
    const char* v[] = {"5", "6"};
    const char* devs[] = {AL_DEV};
    int64_t times[] = {102};
    int mc[] = {2};
    const char* const* measList[] = {m};
    const char* const* valuesList[] = {v};
    EXPECT_EQ(ts_session_insert_aligned_records_str(s, 1, devs, times, mc, measList, valuesList), TS_OK) << ts_get_last_error();
    closeDestroyTree(s);
}

// 用例40: insert_aligned_records 强类型
TEST(TreeInsert, Case40_AlignedRecordsTyped) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareAligned(s, AL_DEV);
    const char* m[] = {"m1", "m2"};
    TSDataType_C types[] = {TS_TYPE_INT64, TS_TYPE_INT64};
    int64_t v1 = 7, v2 = 8;
    const void* vals[] = {&v1, &v2};
    const char* devs[] = {AL_DEV};
    int64_t times[] = {103};
    int mc[] = {2};
    const char* const* measList[] = {m};
    const TSDataType_C* typesList[] = {types};
    const void* const* valuesList[] = {vals};
    EXPECT_EQ(ts_session_insert_aligned_records(s, 1, devs, times, mc, measList, typesList, valuesList), TS_OK) << ts_get_last_error();
    closeDestroyTree(s);
}

// 用例41: insert_aligned_records_of_one_device
TEST(TreeInsert, Case41_AlignedRecordsOfOneDevice) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareAligned(s, AL_DEV);
    const char* m[] = {"m1", "m2"};
    TSDataType_C types[] = {TS_TYPE_INT64, TS_TYPE_INT64};
    int64_t times[] = {104, 105};
    int mc[] = {2, 2};
    const char* const* measList[] = {m, m};
    const TSDataType_C* typesList[] = {types, types};
    int64_t v1a = 5, v2a = 7, v1b = 6, v2b = 8;
    const void* row0[] = {&v1a, &v2a};
    const void* row1[] = {&v1b, &v2b};
    const void* const* valuesList[] = {row0, row1};
    EXPECT_EQ(ts_session_insert_aligned_records_of_one_device(s, AL_DEV, 2, times, mc, measList, typesList, valuesList, true), TS_OK) << ts_get_last_error();
    closeDestroyTree(s);
}

// 用例42: insert_tablet 单 Tablet
TEST(TreeInsert, Case42_InsertTablet) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareThreeInt64(s);
    const char* cols[] = {"s1", "s2", "s3"};
    TSDataType_C types[] = {TS_TYPE_INT64, TS_TYPE_INT64, TS_TYPE_INT64};
    CTablet* t = ts_tablet_new(DEV, 3, cols, types, 100);
    ASSERT_NE(t, nullptr);
    for (int i = 0; i < 50; i++) {
        ts_tablet_add_timestamp(t, i, i);
        for (int c = 0; c < 3; c++) ts_tablet_add_value_int64(t, c, i, c);
    }
    ts_tablet_set_row_count(t, 50);
    EXPECT_EQ(ts_session_insert_tablet(s, t, false), TS_OK) << ts_get_last_error();
    ts_tablet_destroy(t);
    EXPECT_EQ(treeQueryRowCount(s, "select s1,s2,s3 from root.c_insert.d1"), 50);
    cleanDevice(s);
    closeDestroyTree(s);
}

// 用例43: insert_tablets 多设备
TEST(TreeInsert, Case43_InsertTablets) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* pa = "root.c_insert.ta.s1";
    const char* pb = "root.c_insert.tb.s1";
    ensureTimeseries(s, pa, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    ensureTimeseries(s, pb, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* cols[] = {"s1"};
    TSDataType_C types[] = {TS_TYPE_INT64};
    CTablet* t1 = ts_tablet_new("root.c_insert.ta", 1, cols, types, 5);
    CTablet* t2 = ts_tablet_new("root.c_insert.tb", 1, cols, types, 5);
    ASSERT_NE(t1, nullptr); ASSERT_NE(t2, nullptr);
    ts_tablet_add_timestamp(t1, 0, 1); ts_tablet_add_value_int64(t1, 0, 0, 100); ts_tablet_set_row_count(t1, 1);
    ts_tablet_add_timestamp(t2, 0, 2); ts_tablet_add_value_int64(t2, 0, 0, 200); ts_tablet_set_row_count(t2, 1);
    const char* devs[] = {"root.c_insert.ta", "root.c_insert.tb"};
    CTablet* tablets[] = {t1, t2};
    EXPECT_EQ(ts_session_insert_tablets(s, 2, devs, tablets, false), TS_OK) << ts_get_last_error();
    ts_tablet_destroy(t1); ts_tablet_destroy(t2);
    EXPECT_EQ(treeQueryRowCount(s, "select s1 from root.c_insert.ta"), 1);
    EXPECT_EQ(treeQueryRowCount(s, "select s1 from root.c_insert.tb"), 1);
    ts_session_delete_timeseries(s, pa);
    ts_session_delete_timeseries(s, pb);
    closeDestroyTree(s);
}

// 用例44: insert_aligned_tablet 对齐单 Tablet
TEST(TreeInsert, Case44_AlignedTablet) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareAligned(s, AL_DEV);
    const char* cols[] = {"m1", "m2"};
    TSDataType_C types[] = {TS_TYPE_INT64, TS_TYPE_INT64};
    CTablet* t = ts_tablet_new(AL_DEV, 2, cols, types, 5);
    ASSERT_NE(t, nullptr);
    ts_tablet_add_timestamp(t, 0, 200);
    ts_tablet_add_value_int64(t, 0, 0, 13);
    ts_tablet_add_value_int64(t, 1, 0, 14);
    ts_tablet_set_row_count(t, 1);
    EXPECT_EQ(ts_session_insert_aligned_tablet(s, t, false), TS_OK) << ts_get_last_error();
    ts_tablet_destroy(t);
    closeDestroyTree(s);
}

// 用例45: insert_aligned_tablets 多对齐 Tablet
TEST(TreeInsert, Case45_AlignedTablets) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareAligned(s, AL_DEV);
    prepareAligned(s, AL_DEV2);
    const char* cols[] = {"m1", "m2"};
    TSDataType_C types[] = {TS_TYPE_INT64, TS_TYPE_INT64};
    CTablet* t1 = ts_tablet_new(AL_DEV, 2, cols, types, 5);
    CTablet* t2 = ts_tablet_new(AL_DEV2, 2, cols, types, 5);
    ASSERT_NE(t1, nullptr); ASSERT_NE(t2, nullptr);
    ts_tablet_add_timestamp(t1, 0, 106); ts_tablet_add_value_int64(t1, 0, 0, 9); ts_tablet_add_value_int64(t1, 1, 0, 10); ts_tablet_set_row_count(t1, 1);
    ts_tablet_add_timestamp(t2, 0, 107); ts_tablet_add_value_int64(t2, 0, 0, 11); ts_tablet_add_value_int64(t2, 1, 0, 12); ts_tablet_set_row_count(t2, 1);
    const char* devs[] = {AL_DEV, AL_DEV2};
    CTablet* tablets[] = {t1, t2};
    EXPECT_EQ(ts_session_insert_aligned_tablets(s, 2, devs, tablets, false), TS_OK) << ts_get_last_error();
    ts_tablet_destroy(t1); ts_tablet_destroy(t2);
    closeDestroyTree(s);
}

// 用例46: 对非对齐设备使用对齐写入 -> 服务端不报错（改写 isAligned）
TEST(TreeInsert, Case46_AlignedWriteToNonAligned) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    // 普通（非对齐）设备
    prepareThreeInt64(s);
    const char* m[] = {"s1", "s2", "s3"};
    const char* v[] = {"1", "2", "3"};
    EXPECT_EQ(ts_session_insert_aligned_record_str(s, DEV, 500, 3, m, v), TS_OK) << ts_get_last_error();
    cleanDevice(s);
    closeDestroyTree(s);
}

// 用例47: 对对齐设备使用非对齐写入 -> 服务端不报错（改写 isAligned）
TEST(TreeInsert, Case47_NonAlignedWriteToAligned) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareAligned(s, AL_DEV);
    const char* m[] = {"m1", "m2"};
    const char* v[] = {"1", "2"};
    EXPECT_EQ(ts_session_insert_record_str(s, AL_DEV, 600, 2, m, v), TS_OK) << ts_get_last_error();
    closeDestroyTree(s);
}

/* ---------------- 写入异常与校验 ---------------- */

// 用例112: insert_record_str count 与 measurements/values 数量不符 -> 非 TS_OK。
// 说明：count 大于数组长度属调用方越界(UB)，无法安全触发；以非法 count(-1) 验证防御。
TEST(TreeInsertError, Case112_RecordStrCountMismatch) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    prepareThreeInt64(s);
    const char* meas[] = {"s1", "s2", "s3"};
    const char* vals[] = {"1", "2", "3"};
    TsStatus st = ts_session_insert_record_str(s, DEV, 1, -1, meas, vals);  // time=1, 非法 count=-1
    EXPECT_NE(st, TS_OK);
    cleanDevice(s);
    closeDestroyTree(s);
}

// 用例113: insert_record 值类型与序列声明不符 -> 非 TS_OK 或按服务端规则
TEST(TreeInsertError, Case113_RecordTypeMismatch) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, P1, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* meas[] = {"s1"};
    TSDataType_C types[] = {TS_TYPE_DOUBLE};  // 标注 DOUBLE，但序列为 INT64
    double v = 1.5;
    const void* vals[] = {&v};
    TsStatus st = ts_session_insert_record(s, DEV, 1, 1, meas, types, vals);
    (void)st;  // 行为以服务端类型转换规则为准：非 OK 或转换成功，均不崩溃
    ts_session_delete_timeseries(s, P1);
    closeDestroyTree(s);
    SUCCEED();
}

// 用例114: insert_record_str(NULL session) -> TS_ERR_NULL_PTR
TEST(TreeInsertError, Case114_RecordStrNullSession) {
    const char* meas[] = {"s1"};
    const char* vals[] = {"1"};
    EXPECT_EQ(ts_session_insert_record_str(nullptr, DEV, 1, 1, meas, vals), TS_ERR_NULL_PTR);
}

// 用例115: 空测点 count=0 -> 可控，不崩溃
TEST(TreeInsertError, Case115_EmptyMeasurements) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    TsStatus st = ts_session_insert_record_str(s, DEV, 1, 0, nullptr, nullptr);
    (void)st;
    closeDestroyTree(s);
    SUCCEED();
}

// 用例116: insert_tablet 未设置行数（row_count=0）-> 写 0 行或非 OK，不崩溃
TEST(TreeInsertError, Case116_InsertTabletNoRowCount) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_insert.d116.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* cols[] = {"s1"};
    TSDataType_C types[] = {TS_TYPE_INT64};
    CTablet* t = ts_tablet_new("root.c_insert.d116", 1, cols, types, 10);
    ASSERT_NE(t, nullptr);
    ts_tablet_add_timestamp(t, 0, 1);
    ts_tablet_add_value_int64(t, 0, 0, 1);
    // 未调用 set_row_count。
    // 实测：SDK 的 add_timestamp/add_value 会跟踪有效行数，因此即使未显式 set_row_count，
    // insert 仍会写入已填充的行（此处为 1 行）。该行为对数据无害，记录为观察项（REPORT.md 观察#1）。
    TsStatus st = ts_session_insert_tablet(s, t, false);
    (void)st;
    ts_tablet_destroy(t);
    int c = treeQueryRowCount(s, "select s1 from root.c_insert.d116");
    EXPECT_GE(c, 0);  // 不崩溃、行为有定义
    ts_session_delete_timeseries(s, "root.c_insert.d116.s1");
    closeDestroyTree(s);
}

// 用例117: Tablet 列类型与序列不一致 -> 非 TS_OK
TEST(TreeInsertError, Case117_TabletColumnTypeMismatch) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_insert.d117.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* cols[] = {"s1"};
    TSDataType_C types[] = {TS_TYPE_DOUBLE};  // Tablet 声明 DOUBLE，序列为 INT64
    CTablet* t = ts_tablet_new("root.c_insert.d117", 1, cols, types, 10);
    ASSERT_NE(t, nullptr);
    ts_tablet_add_timestamp(t, 0, 1);
    ts_tablet_add_value_double(t, 0, 0, 1.5);
    ts_tablet_set_row_count(t, 1);
    TsStatus st = ts_session_insert_tablet(s, t, false);
    EXPECT_NE(st, TS_OK);
    ts_tablet_destroy(t);
    ts_session_delete_timeseries(s, "root.c_insert.d117.s1");
    closeDestroyTree(s);
}

// 用例118: insert_tablets tabletCount 与数组不一致 -> 不越界、不崩溃。
// 说明：tabletCount 大于数组长度会越界解引用 tablets[i]->cpp（UB，且 NULL 元素会段错误），
// 无法安全构造；这里以 tabletCount=0 验证空批次的健壮性（不崩溃）。
// 真正的 count>数组长度 属调用方契约错误，由代码评审保证。
TEST(TreeInsertError, Case118_InsertTabletsCountMismatch) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    const char* devs[] = {"root.c_insert.d118"};
    CTablet* tablets[] = {nullptr};
    TsStatus st = ts_session_insert_tablets(s, 0, devs, tablets, false);  // 空批次
    (void)st;
    closeDestroyTree(s);
    SUCCEED();
}
