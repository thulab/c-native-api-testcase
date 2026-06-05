/*
 * 错误处理与资源释放
 * 覆盖用例: 82-87, 149-151
 */
#include "c_test_common.h"

#include <thread>

using namespace ctest;

// 用例82: 失败调用后 ts_get_last_error 返回最近一次错误信息
TEST(ErrorResource, Case82_LastErrorAfterFailure) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    CSessionDataSet* ds = nullptr;
    EXPECT_NE(ts_session_execute_query(s, "selec bad sql", &ds), TS_OK);
    const char* err = ts_get_last_error();
    ASSERT_NE(err, nullptr);
    EXPECT_GT(strlen(err), 0u);
    closeDestroyTree(s);
}

// 用例83: last_error 指针只保证到下一次 C API 调用前有效 -> 需复制内容
TEST(ErrorResource, Case83_LastErrorPointerLifecycle) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    CSessionDataSet* ds = nullptr;
    ts_session_execute_query(s, "selec bad sql", &ds);
    std::string saved = ts_get_last_error();  // 立即复制内容
    EXPECT_GT(saved.size(), 0u);
    // 再执行一次其它 C API 调用
    bool exists = false;
    ts_session_check_timeseries_exists(s, "root.x.y.z", &exists);
    // 复制的内容仍然有效可用（不依赖旧指针）
    EXPECT_GT(saved.size(), 0u);
    closeDestroyTree(s);
}

// 用例84: dataSet 释放稳定性（遍历完/未遍历完均可 destroy）
TEST(ErrorResource, Case84_DatasetDestroy) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_err.d1.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* m[] = {"s1"};
    for (int64_t t = 0; t < 10; t++) { const char* v[] = {"1"}; ts_session_insert_record_str(s, "root.c_err.d1", t, 1, m, v); }
    // 未遍历完即 destroy
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_err.d1", &ds), TS_OK);
    if (ts_dataset_has_next(ds)) { CRowRecord* r = ts_dataset_next(ds); if (r) ts_row_record_destroy(r); }
    ts_dataset_destroy(ds);  // 提前释放
    // 之后仍可发起新查询
    EXPECT_EQ(treeQueryRowCount(s, "select s1 from root.c_err.d1"), 10);
    ts_session_delete_timeseries(s, "root.c_err.d1.s1");
    closeDestroyTree(s);
}

// 用例85: 逐行 ts_row_record_destroy 无泄漏/崩溃
TEST(ErrorResource, Case85_RowRecordDestroy) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_err.d2.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* m[] = {"s1"};
    for (int64_t t = 0; t < 20; t++) { const char* v[] = {"1"}; ts_session_insert_record_str(s, "root.c_err.d2", t, 1, m, v); }
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_err.d2", &ds), TS_OK);
    int n = 0;
    while (ts_dataset_has_next(ds)) { CRowRecord* r = ts_dataset_next(ds); if (!r) break; n++; ts_row_record_destroy(r); }
    EXPECT_EQ(n, 20);
    ts_dataset_destroy(ds);
    ts_session_delete_timeseries(s, "root.c_err.d2.s1");
    closeDestroyTree(s);
}

// 用例86: insert 后不转移 Tablet 所有权，写入后仍由调用方 destroy
TEST(ErrorResource, Case86_TabletOwnership) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_err.d3.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
    const char* cols[] = {"s1"};
    TSDataType_C types[] = {TS_TYPE_INT64};
    CTablet* t = ts_tablet_new("root.c_err.d3", 1, cols, types, 5);
    ASSERT_NE(t, nullptr);
    ts_tablet_add_timestamp(t, 0, 1);
    ts_tablet_add_value_int64(t, 0, 0, 1);
    ts_tablet_set_row_count(t, 1);
    EXPECT_EQ(ts_session_insert_tablet(s, t, false), TS_OK);
    ts_tablet_destroy(t);  // 写入后调用方销毁，不应 double free
    ts_session_delete_timeseries(s, "root.c_err.d3.s1");
    closeDestroyTree(s);
    SUCCEED();
}

// 用例87: close 仅关闭连接，destroy 仍需显式调用
TEST(ErrorResource, Case87_CloseThenDestroy) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(ts_session_close(s), TS_OK);
    ts_session_destroy(s);
    SUCCEED();
}

// 用例149: 无失败调用时 ts_get_last_error 返回值可控（在新线程内验证）
TEST(ErrorResource, Case149_LastErrorInitialState) {
    std::string val;
    std::thread th([&val]() {
        const char* e = ts_get_last_error();  // 该线程内尚未发生失败调用
        val = (e ? e : "");
    });
    th.join();
    EXPECT_TRUE(val.empty());  // 期望空字符串
}

// 用例150: 成功调用后语义（成功不保证清空错误信息）
TEST(ErrorResource, Case150_LastErrorAfterSuccess) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    CSessionDataSet* ds = nullptr;
    ts_session_execute_query(s, "selec bad", &ds);  // 失败
    std::string afterFail = ts_get_last_error();
    EXPECT_GT(afterFail.size(), 0u);
    // 成功调用
    bool exists = false;
    ASSERT_EQ(ts_session_check_timeseries_exists(s, "root.c_err.x.y", &exists), TS_OK);
    // 成功调用后读取行为以实现为准（可能清空，可能保留），仅验证不崩溃
    const char* e = ts_get_last_error();
    (void)e;
    closeDestroyTree(s);
    SUCCEED();
}

// 用例151: ts_get_last_error 线程局部，A 线程失败不影响 B 线程
TEST(ErrorResource, Case151_LastErrorThreadLocal) {
    std::string errA, errB;
    std::thread ta([&errA]() {
        CSession* s = newOpenTreeSession();
        if (s) {
            CSessionDataSet* ds = nullptr;
            ts_session_execute_query(s, "selec aaa bad", &ds);  // A 线程失败
            errA = ts_get_last_error();
            closeDestroyTree(s);
        }
    });
    ta.join();

    std::thread tb([&errB]() {
        // B 线程尚未发生失败调用，应读到空（线程局部，不受 A 影响）
        errB = ts_get_last_error();
    });
    tb.join();

    EXPECT_GT(errA.size(), 0u);
    EXPECT_TRUE(errB.empty()) << "B 线程不应读到 A 线程的错误: " << errB;
}
