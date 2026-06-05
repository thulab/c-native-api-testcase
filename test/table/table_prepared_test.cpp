/*
 * 表模型 - 预编译语句
 * 覆盖用例: 78-81
 * 说明：服务端可能未暴露 prepareStatement RPC，此时相关用例 SKIP 并记录。
 */
#include "c_test_common.h"

using namespace ctest;

namespace {
bool preparedUnsupported(const char* err) {
    if (!err) return false;
    return std::strstr(err, "prepareStatement") != nullptr ||
           std::strstr(err, "Invalid method name") != nullptr;
}

// 准备一张含全类型字段的表并写入两行，返回是否成功。
void prepareTypedTable(CTableSession* s) {
    tablePrepareDatabase(s, "c_ps_db");
    ts_table_session_execute_non_query(s,
        "CREATE TABLE tps (tag1 string tag, b boolean field, i32 int32 field, "
        "i64 int64 field, f float field, d double field, s string field)");
    const char* cols[] = {"tag1","b","i32","i64","f","d","s"};
    TSDataType_C types[] = {TS_TYPE_STRING, TS_TYPE_BOOLEAN, TS_TYPE_INT32, TS_TYPE_INT64,
                            TS_TYPE_FLOAT, TS_TYPE_DOUBLE, TS_TYPE_STRING};
    TSColumnCategory_C cats[] = {TS_COL_TAG, TS_COL_FIELD, TS_COL_FIELD, TS_COL_FIELD,
                                 TS_COL_FIELD, TS_COL_FIELD, TS_COL_FIELD};
    CTablet* t = ts_tablet_new_with_category("tps", 7, cols, types, cats, 10);
    ts_tablet_add_timestamp(t, 0, 1);
    ts_tablet_add_value_string(t, 0, 0, "dev1");
    ts_tablet_add_value_bool(t, 1, 0, true);
    ts_tablet_add_value_int32(t, 2, 0, 123);
    ts_tablet_add_value_int64(t, 3, 0, 123456789LL);
    ts_tablet_add_value_float(t, 4, 0, 1.5f);
    ts_tablet_add_value_double(t, 5, 0, 2.5);
    ts_tablet_add_value_string(t, 6, 0, "alpha");
    ts_tablet_add_timestamp(t, 1, 2);
    ts_tablet_add_value_string(t, 0, 1, "dev2");
    ts_tablet_add_value_bool(t, 1, 1, false);
    ts_tablet_add_value_int32(t, 2, 1, 456);
    ts_tablet_add_value_int64(t, 3, 1, 987654321LL);
    ts_tablet_add_value_float(t, 4, 1, 3.5f);
    ts_tablet_add_value_double(t, 5, 1, 4.5);
    ts_tablet_add_value_string(t, 6, 1, "beta");
    ts_tablet_set_row_count(t, 2);
    ts_table_session_insert(s, t);
    ts_tablet_destroy(t);
}
}  // namespace

// 用例78: 创建预编译语句句柄，paramCount 正确
TEST(TablePrepared, Case78_New) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    prepareTypedTable(s);
    int pc = 0;
    CTablePreparedStmt* ps = ts_table_prepared_statement_new(
        s, "SELECT i64 FROM tps WHERE tag1 = ?", "c_ps_new", &pc);
    if (!ps && preparedUnsupported(ts_get_last_error())) {
        ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_ps_db");
        closeDestroyTable(s);
        GTEST_SKIP() << "服务端未暴露 prepareStatement RPC";
    }
    ASSERT_NE(ps, nullptr) << ts_get_last_error();
    EXPECT_EQ(pc, 1);
    ts_table_prepared_statement_free(ps);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_ps_db");
    closeDestroyTable(s);
}

// 用例79 + 用例80: 全类型参数绑定 + 执行查询
TEST(TablePrepared, Case79_80_BindAllTypesAndExecute) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    prepareTypedTable(s);
    int pc = 0;
    CTablePreparedStmt* ps = ts_table_prepared_statement_new(
        s, "SELECT i64 FROM tps WHERE tag1 = ? AND b = ? AND i32 = ? AND i64 = ? AND f = ? AND d = ? AND s = ?",
        "c_ps_all", &pc);
    if (!ps && preparedUnsupported(ts_get_last_error())) {
        ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_ps_db");
        closeDestroyTable(s);
        GTEST_SKIP() << "服务端未暴露 prepareStatement RPC";
    }
    ASSERT_NE(ps, nullptr) << ts_get_last_error();
    EXPECT_EQ(pc, 7);
    EXPECT_EQ(ts_table_prepared_statement_set_string(ps, 0, "dev1"), TS_OK);
    EXPECT_EQ(ts_table_prepared_statement_set_bool(ps, 1, true), TS_OK);
    EXPECT_EQ(ts_table_prepared_statement_set_int32(ps, 2, 123), TS_OK);
    EXPECT_EQ(ts_table_prepared_statement_set_int64(ps, 3, 123456789LL), TS_OK);
    EXPECT_EQ(ts_table_prepared_statement_set_float(ps, 4, 1.5f), TS_OK);
    EXPECT_EQ(ts_table_prepared_statement_set_double(ps, 5, 2.5), TS_OK);
    EXPECT_EQ(ts_table_prepared_statement_set_string(ps, 6, "alpha"), TS_OK);

    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_table_prepared_statement_execute_query(ps, -1, &ds), TS_OK) << ts_get_last_error();
    ASSERT_NE(ds, nullptr);
    int count = 0;
    while (ts_dataset_has_next(ds)) {
        CRowRecord* r = ts_dataset_next(ds);
        if (!r) break;
        count++;
        ts_row_record_destroy(r);
    }
    EXPECT_EQ(count, 1);
    ts_dataset_destroy(ds);
    ts_table_prepared_statement_free(ps);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_ps_db");
    closeDestroyTable(s);
}

// 用例81: set_null + clear_parameters
TEST(TablePrepared, Case81_SetNullAndClear) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    prepareTypedTable(s);
    int pc = 0;
    CTablePreparedStmt* ps = ts_table_prepared_statement_new(
        s, "SELECT s FROM tps WHERE s = ?", "c_ps_null", &pc);
    if (!ps && preparedUnsupported(ts_get_last_error())) {
        ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_ps_db");
        closeDestroyTable(s);
        GTEST_SKIP() << "服务端未暴露 prepareStatement RPC";
    }
    ASSERT_NE(ps, nullptr) << ts_get_last_error();
    EXPECT_EQ(pc, 1);
    EXPECT_EQ(ts_table_prepared_statement_set_null(ps, 0), TS_OK);
    EXPECT_EQ(ts_table_prepared_statement_clear_parameters(ps), TS_OK);
    ts_table_prepared_statement_free(ps);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_ps_db");
    closeDestroyTable(s);
}
