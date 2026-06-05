/*
 * 非功能 - 性能与稳定性 / 并发
 * 覆盖用例: 88, 152, 153
 * 说明：为保证自动化时长可控，数据量默认取万级（可按需调大到十万级做专项压测）。
 */
#include "c_test_common.h"

#include <thread>
#include <vector>
#include <atomic>

using namespace ctest;

namespace {
const int kLargeRows = 10000;  // 专项压测可改为 100000
}

// 用例88 + 用例153: 大结果集稳定遍历，逐行释放，行数一致
TEST(Stability, Case88_153_LargeResultSet) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ensureTimeseries(s, "root.c_perf.d1.s1", TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);

    // 用 Tablet 批量写入提速
    const char* cols[] = {"s1"};
    TSDataType_C types[] = {TS_TYPE_INT64};
    const int CHUNK = 1000;
    for (int base = 0; base < kLargeRows; base += CHUNK) {
        CTablet* t = ts_tablet_new("root.c_perf.d1", 1, cols, types, CHUNK);
        ASSERT_NE(t, nullptr);
        for (int i = 0; i < CHUNK; i++) {
            ts_tablet_add_timestamp(t, i, (int64_t)(base + i));
            ts_tablet_add_value_int64(t, 0, i, (int64_t)(base + i));
        }
        ts_tablet_set_row_count(t, CHUNK);
        ASSERT_EQ(ts_session_insert_tablet(s, t, false), TS_OK) << ts_get_last_error();
        ts_tablet_destroy(t);
    }

    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_session_execute_query(s, "select s1 from root.c_perf.d1", &ds), TS_OK);
    ts_dataset_set_fetch_size(ds, 4096);
    long n = 0;
    while (ts_dataset_has_next(ds)) {
        CRowRecord* r = ts_dataset_next(ds);
        if (!r) break;
        n++;
        ts_row_record_destroy(r);
    }
    EXPECT_EQ(n, kLargeRows);
    ts_dataset_destroy(ds);
    ts_session_delete_timeseries(s, "root.c_perf.d1.s1");
    closeDestroyTree(s);
}

// 用例152: 多线程各自独立会话并发读写，数据互不串扰，无崩溃
TEST(Stability, Case152_ConcurrentSessions) {
    const int THREADS = 4;
    const int ROWS = 200;
    std::atomic<int> okThreads{0};

    auto worker = [&](int tid) {
        CSession* s = ts_session_new(kHost, kPort, kUser, kPass);
        if (!s) return;
        if (ts_session_open(s) != TS_OK) { ts_session_destroy(s); return; }
        std::string dev = "root.c_conc.d" + std::to_string(tid);
        std::string path = dev + ".s1";
        bool exists = false;
        if (ts_session_check_timeseries_exists(s, path.c_str(), &exists) == TS_OK && exists)
            ts_session_delete_timeseries(s, path.c_str());
        ts_session_create_timeseries(s, path.c_str(), TS_TYPE_INT64, TS_ENCODING_RLE, TS_COMPRESSION_SNAPPY);
        const char* m[] = {"s1"};
        bool allOk = true;
        for (int64_t t = 0; t < ROWS; t++) {
            std::string vs = std::to_string(tid * 1000 + t);
            const char* v[] = {vs.c_str()};
            if (ts_session_insert_record_str(s, dev.c_str(), t, 1, m, v) != TS_OK) { allOk = false; break; }
        }
        // 读回校验行数
        std::string sql = "select s1 from " + dev;
        int cnt = treeQueryRowCount(s, sql.c_str());
        if (allOk && cnt == ROWS) okThreads++;
        ts_session_delete_timeseries(s, path.c_str());
        closeDestroyTree(s);
    };

    std::vector<std::thread> ths;
    for (int i = 0; i < THREADS; i++) ths.emplace_back(worker, i);
    for (auto& t : ths) t.join();

    EXPECT_EQ(okThreads.load(), THREADS) << "部分线程数据不一致或失败";
}
