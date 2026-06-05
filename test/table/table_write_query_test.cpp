/*
 * 表模型 - 写入与查询及其异常
 * 覆盖用例: 72-77, 144-146
 */
#include "c_test_common.h"

using namespace ctest;

// 用例72: 带列类别 Tablet 写入（TAG+ATTRIBUTE+FIELD）
TEST(TableWrite, Case72_InsertCategoryTablet) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    tablePrepareDatabase(s, "c_w_db1");
    ASSERT_EQ(ts_table_session_execute_non_query(s,
        "CREATE TABLE t1 (tag1 string tag, attr1 string attribute, m1 double field)"), TS_OK) << ts_get_last_error();

    const char* cols[] = {"tag1", "attr1", "m1"};
    TSDataType_C types[] = {TS_TYPE_STRING, TS_TYPE_STRING, TS_TYPE_DOUBLE};
    TSColumnCategory_C cats[] = {TS_COL_TAG, TS_COL_ATTRIBUTE, TS_COL_FIELD};
    CTablet* t = ts_tablet_new_with_category("t1", 3, cols, types, cats, 100);
    ASSERT_NE(t, nullptr);
    for (int i = 0; i < 50; i++) {
        ts_tablet_add_timestamp(t, i, (int64_t)i);
        ts_tablet_add_value_string(t, 0, i, "device_A");
        ts_tablet_add_value_string(t, 1, i, "attr_val");
        ts_tablet_add_value_double(t, 2, i, i * 1.5);
    }
    ts_tablet_set_row_count(t, 50);
    EXPECT_EQ(ts_table_session_insert(s, t), TS_OK) << ts_get_last_error();
    ts_tablet_destroy(t);

    EXPECT_EQ(tableQueryRowCount(s, "SELECT * FROM t1"), 50);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_w_db1");
    closeDestroyTable(s);
}

// 用例73: 普通查询 SELECT *
TEST(TableWrite, Case73_SelectAll) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    tablePrepareDatabase(s, "c_w_db2");
    ts_table_session_execute_non_query(s, "CREATE TABLE t2 (tag1 string tag, m1 int32 field)");
    const char* cols[] = {"tag1", "m1"};
    TSDataType_C types[] = {TS_TYPE_STRING, TS_TYPE_INT32};
    TSColumnCategory_C cats[] = {TS_COL_TAG, TS_COL_FIELD};
    CTablet* t = ts_tablet_new_with_category("t2", 2, cols, types, cats, 10);
    for (int i = 0; i < 10; i++) {
        ts_tablet_add_timestamp(t, i, (int64_t)i);
        ts_tablet_add_value_string(t, 0, i, "dev1");
        ts_tablet_add_value_int32(t, 1, i, i * 10);
    }
    ts_tablet_set_row_count(t, 10);
    EXPECT_EQ(ts_table_session_insert(s, t), TS_OK);
    ts_tablet_destroy(t);
    EXPECT_EQ(tableQueryRowCount(s, "SELECT * FROM t2"), 10);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_w_db2");
    closeDestroyTable(s);
}

// 用例74: 超时查询
TEST(TableWrite, Case74_QueryWithTimeout) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    tablePrepareDatabase(s, "c_w_db3");
    ts_table_session_execute_non_query(s, "CREATE TABLE t3 (tag1 string tag, m1 int32 field)");
    const char* cols[] = {"tag1", "m1"};
    TSDataType_C types[] = {TS_TYPE_STRING, TS_TYPE_INT32};
    TSColumnCategory_C cats[] = {TS_COL_TAG, TS_COL_FIELD};
    CTablet* t = ts_tablet_new_with_category("t3", 2, cols, types, cats, 10);
    for (int i = 0; i < 10; i++) {
        ts_tablet_add_timestamp(t, i, (int64_t)i);
        ts_tablet_add_value_string(t, 0, i, "dev1");
        ts_tablet_add_value_int32(t, 1, i, i);
    }
    ts_tablet_set_row_count(t, 10);
    ts_table_session_insert(s, t);
    ts_tablet_destroy(t);
    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_table_session_execute_query_with_timeout(s, "SELECT * FROM t3", 60000, &ds), TS_OK);
    int n = 0;
    while (ts_dataset_has_next(ds)) { CRowRecord* r = ts_dataset_next(ds); if (!r) break; n++; ts_row_record_destroy(r); }
    EXPECT_EQ(n, 10);
    ts_dataset_destroy(ds);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_w_db3");
    closeDestroyTable(s);
}

// 用例75: 表模型结果集列信息 + 行记录元信息
TEST(TableWrite, Case75_DatasetColumnInfo) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    tablePrepareDatabase(s, "c_w_db4");
    ts_table_session_execute_non_query(s, "CREATE TABLE t4 (tag1 string tag, m1 int64 field)");
    const char* cols[] = {"tag1", "m1"};
    TSDataType_C types[] = {TS_TYPE_STRING, TS_TYPE_INT64};
    TSColumnCategory_C cats[] = {TS_COL_TAG, TS_COL_FIELD};
    CTablet* t = ts_tablet_new_with_category("t4", 2, cols, types, cats, 5);
    for (int i = 0; i < 5; i++) {
        ts_tablet_add_timestamp(t, i, (int64_t)i);
        ts_tablet_add_value_string(t, 0, i, "dev1");
        ts_tablet_add_value_int64(t, 1, i, (int64_t)(i * 100));
    }
    ts_tablet_set_row_count(t, 5);
    ts_table_session_insert(s, t);
    ts_tablet_destroy(t);

    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_table_session_execute_query(s, "SELECT * FROM t4", &ds), TS_OK);
    ASSERT_NE(ds, nullptr);
    int colCount = ts_dataset_get_column_count(ds);
    EXPECT_GE(colCount, 2);
    for (int i = 0; i < colCount; i++) {
        const char* ct = ts_dataset_get_column_type(ds, i);
        EXPECT_NE(ct, nullptr);
        EXPECT_GT(strlen(ct), 0u);
    }
    if (ts_dataset_has_next(ds)) {
        CRowRecord* r = ts_dataset_next(ds);
        EXPECT_GE(ts_row_record_get_field_count(r), 1);
        (void)ts_row_record_get_timestamp(r);
        ts_row_record_destroy(r);
    }
    ts_dataset_destroy(ds);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_w_db4");
    closeDestroyTable(s);
}

// 用例76: 多类型 Tablet 写入与回读
TEST(TableWrite, Case76_MultiTypeTablet) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    tablePrepareDatabase(s, "c_w_db5");
    ts_table_session_execute_non_query(s,
        "CREATE TABLE t5 (tag1 string tag, m_bool boolean field, m_int32 int32 field, "
        "m_int64 int64 field, m_float float field, m_double double field, m_text text field)");
    const char* cols[] = {"tag1","m_bool","m_int32","m_int64","m_float","m_double","m_text"};
    TSDataType_C types[] = {TS_TYPE_STRING, TS_TYPE_BOOLEAN, TS_TYPE_INT32, TS_TYPE_INT64,
                            TS_TYPE_FLOAT, TS_TYPE_DOUBLE, TS_TYPE_TEXT};
    TSColumnCategory_C cats[] = {TS_COL_TAG, TS_COL_FIELD, TS_COL_FIELD, TS_COL_FIELD,
                                 TS_COL_FIELD, TS_COL_FIELD, TS_COL_FIELD};
    CTablet* t = ts_tablet_new_with_category("t5", 7, cols, types, cats, 20);
    for (int i = 0; i < 20; i++) {
        ts_tablet_add_timestamp(t, i, (int64_t)(i + 1000));
        ts_tablet_add_value_string(t, 0, i, "dev1");
        ts_tablet_add_value_bool(t, 1, i, (i % 2 == 0));
        ts_tablet_add_value_int32(t, 2, i, i * 10);
        ts_tablet_add_value_int64(t, 3, i, (int64_t)i * 100);
        ts_tablet_add_value_float(t, 4, i, i * 1.1f);
        ts_tablet_add_value_double(t, 5, i, i * 2.2);
        ts_tablet_add_value_string(t, 6, i, "hello");
    }
    ts_tablet_set_row_count(t, 20);
    EXPECT_EQ(ts_table_session_insert(s, t), TS_OK) << ts_get_last_error();
    ts_tablet_destroy(t);
    EXPECT_EQ(tableQueryRowCount(s, "SELECT * FROM t5"), 20);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_w_db5");
    closeDestroyTable(s);
}

// 用例77: OBJECT 字段写入与读回字节长度
TEST(TableWrite, Case77_ObjectField) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    tablePrepareDatabase(s, "c_w_obj");
    TsStatus ddl = ts_table_session_execute_non_query(s,
        "CREATE TABLE tobj (tag1 string tag, payload object field)");
    if (ddl != TS_OK) {
        // 服务端可能不支持 OBJECT 类型，跳过（记录）
        GTEST_SKIP() << "服务端不支持 OBJECT 类型: " << ts_get_last_error();
    }
    const char* cols[] = {"tag1", "payload"};
    TSDataType_C types[] = {TS_TYPE_STRING, TS_TYPE_OBJECT};
    TSColumnCategory_C cats[] = {TS_COL_TAG, TS_COL_FIELD};
    CTablet* t = ts_tablet_new_with_category("tobj", 2, cols, types, cats, 10);
    ASSERT_NE(t, nullptr);
    const uint8_t blob[] = {'h','e','l','l','o','-','o','b','j'};
    ts_tablet_add_timestamp(t, 0, 1000);
    ts_tablet_add_value_string(t, 0, 0, "dev1");
    ASSERT_EQ(ts_tablet_add_value_object(t, 1, 0, true, 0, blob, sizeof(blob)), TS_OK);
    ts_tablet_set_row_count(t, 1);
    EXPECT_EQ(ts_table_session_insert(s, t), TS_OK) << ts_get_last_error();
    ts_tablet_destroy(t);

    CSessionDataSet* ds = nullptr;
    ASSERT_EQ(ts_table_session_execute_query(s, "SELECT * FROM tobj", &ds), TS_OK);
    ASSERT_TRUE(ts_dataset_has_next(ds));
    CRowRecord* r = ts_dataset_next(ds);
    bool found = false;
    int n = ts_row_record_get_field_count(r);
    for (int i = 0; i < n; i++) {
        if (ts_row_record_get_data_type(r, i) == TS_TYPE_OBJECT) {
            found = true;
            EXPECT_FALSE(ts_row_record_is_null(r, i));
            EXPECT_GT(ts_row_record_get_string_byte_length(r, i), 0u);
            break;
        }
    }
    EXPECT_TRUE(found);
    ts_row_record_destroy(r);
    ts_dataset_destroy(ds);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_w_obj");
    closeDestroyTable(s);
}

/* ---------------- 异常 ---------------- */

// 用例144: 语法错误 SQL -> 非 TS_OK
TEST(TableWriteError, Case144_SyntaxError) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    tablePrepareDatabase(s, "c_w_err");
    CSessionDataSet* ds = nullptr;
    EXPECT_NE(ts_table_session_execute_query(s, "selec * fro nope", &ds), TS_OK);
    EXPECT_GT(strlen(ts_get_last_error()), 0u);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_w_err");
    closeDestroyTable(s);
}

// 用例145: 未 USE 数据库 / 表不存在写入 -> 非 TS_OK。
// 说明：表模型 insert 在已选库时会对不存在的表自动建表（服务端 auto-create，REPORT.md 观察#2），
// 因此用「未选择数据库」这一确定性失败路径来验证写入被拒绝。
TEST(TableWriteError, Case145_InsertNonexistTable) {
    CTableSession* s = newOpenTableSession();  // database 为空，且未执行任何 USE
    ASSERT_NE(s, nullptr);
    const char* cols[] = {"tag1", "m1"};
    TSDataType_C types[] = {TS_TYPE_STRING, TS_TYPE_INT32};
    TSColumnCategory_C cats[] = {TS_COL_TAG, TS_COL_FIELD};
    CTablet* t = ts_tablet_new_with_category("no_such_table", 2, cols, types, cats, 5);
    ASSERT_NE(t, nullptr);
    ts_tablet_add_timestamp(t, 0, 1);
    ts_tablet_add_value_string(t, 0, 0, "dev1");
    ts_tablet_add_value_int32(t, 1, 0, 1);
    ts_tablet_set_row_count(t, 1);
    TsStatus st = ts_table_session_insert(s, t);
    EXPECT_NE(st, TS_OK);
    EXPECT_GT(strlen(ts_get_last_error()), 0u);
    ts_tablet_destroy(t);
    closeDestroyTable(s);
}

// 用例146: Tablet 列类别与表 schema 不符 -> 非 TS_OK
TEST(TableWriteError, Case146_ColumnCategoryMismatch) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);
    tablePrepareDatabase(s, "c_w_cat");
    ts_table_session_execute_non_query(s, "CREATE TABLE tcat (tag1 string tag, m1 int32 field)");
    // 将 tag1 错误声明为 FIELD
    const char* cols[] = {"tag1", "m1"};
    TSDataType_C types[] = {TS_TYPE_STRING, TS_TYPE_INT32};
    TSColumnCategory_C cats[] = {TS_COL_FIELD, TS_COL_FIELD};  // tag1 应为 TAG
    CTablet* t = ts_tablet_new_with_category("tcat", 2, cols, types, cats, 5);
    ASSERT_NE(t, nullptr);
    ts_tablet_add_timestamp(t, 0, 1);
    ts_tablet_add_value_string(t, 0, 0, "dev1");
    ts_tablet_add_value_int32(t, 1, 0, 1);
    ts_tablet_set_row_count(t, 1);
    TsStatus st = ts_table_session_insert(s, t);
    EXPECT_NE(st, TS_OK);
    ts_tablet_destroy(t);
    ts_table_session_execute_non_query(s, "DROP DATABASE IF EXISTS c_w_cat");
    closeDestroyTable(s);
}
