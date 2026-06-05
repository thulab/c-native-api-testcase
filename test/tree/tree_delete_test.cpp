/*
 * 树模型 - 数据删除及其异常
 * 覆盖用例: 65-67, 140-141
 */
#include "c_test_common.h"

using namespace ctest;

namespace {
const char* DEV = "root.c_del.d1";
void writeRows(CSession* s, const char* path, const char* meas, int n) {
    ensureTimeseries(s, path, TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* ms[] = {meas};
    for (int64_t t = 0; t < n; t++) { const char* v[] = {"1"}; ts_session_insert_record_str(s, DEV, t, 1, ms, v); }
}
}  // namespace

// 用例65: delete_data 单路径按 endTime 删除
TEST(TreeDelete, Case65_DeleteData) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    writeRows(s, "root.c_del.d1.s1", "s1", 100);
    ASSERT_EQ(treeQueryRowCount(s, "select s1 from root.c_del.d1"), 100);
    EXPECT_EQ(ts_session_delete_data(s, "root.c_del.d1.s1", 49), TS_OK) << ts_get_last_error();
    EXPECT_EQ(treeQueryRowCount(s, "select s1 from root.c_del.d1"), 50);  // 保留 [50,99]
    ts_session_delete_timeseries(s, "root.c_del.d1.s1");
    closeDestroyTree(s);
}

// 用例66: delete_data_batch 多路径按 endTime 删除
TEST(TreeDelete, Case66_DeleteDataBatch) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    writeRows(s, "root.c_del.d1.s1", "s1", 100);
    writeRows(s, "root.c_del.d1.s2", "s2", 100);
    const char* paths[] = {"root.c_del.d1.s1", "root.c_del.d1.s2"};
    EXPECT_EQ(ts_session_delete_data_batch(s, 2, paths, 49), TS_OK) << ts_get_last_error();
    EXPECT_EQ(treeQueryRowCount(s, "select s1,s2 from root.c_del.d1"), 50);
    ts_session_delete_timeseries_batch(s, paths, 2);
    closeDestroyTree(s);
}

// 用例67: delete_data_range 多路径时间区间删除
TEST(TreeDelete, Case67_DeleteDataRange) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    writeRows(s, "root.c_del.d1.s1", "s1", 100);
    const char* paths[] = {"root.c_del.d1.s1"};
    EXPECT_EQ(ts_session_delete_data_range(s, 1, paths, 10, 19), TS_OK) << ts_get_last_error();
    EXPECT_EQ(treeQueryRowCount(s, "select s1 from root.c_del.d1"), 90);  // 删除 [10,19] 共 10 行
    ts_session_delete_timeseries(s, "root.c_del.d1.s1");
    closeDestroyTree(s);
}

// 用例140: delete_data_range startTime>endTime -> 非 TS_OK 或不删除
TEST(TreeDeleteError, Case140_RangeReversed) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    writeRows(s, "root.c_del.d1.s1", "s1", 50);
    const char* paths[] = {"root.c_del.d1.s1"};
    TsStatus st = ts_session_delete_data_range(s, 1, paths, 100, 0);
    (void)st;  // 行为有定义：非 OK 或不删除任何数据
    EXPECT_EQ(treeQueryRowCount(s, "select s1 from root.c_del.d1"), 50);  // 原数据不变
    ts_session_delete_timeseries(s, "root.c_del.d1.s1");
    closeDestroyTree(s);
}

// 用例141: delete_data_batch 含不存在路径 -> 可控，不崩溃
TEST(TreeDeleteError, Case141_BatchWithNonexist) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    writeRows(s, "root.c_del.d1.s1", "s1", 50);
    const char* paths[] = {"root.c_del.d1.s1", "root.c_del.d1.nope"};
    TsStatus st = ts_session_delete_data_batch(s, 2, paths, 49);
    (void)st;  // 存在路径数据按 endTime 删除；不崩溃
    ts_session_delete_timeseries(s, "root.c_del.d1.s1");
    closeDestroyTree(s);
    SUCCEED();
}
