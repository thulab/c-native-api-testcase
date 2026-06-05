/*
 * 树模型 - 生命周期与连接管理 / 连接与鉴权异常 / 资源生命周期
 * 覆盖用例: 1-10, 87, 89, 90-100
 */
#include "c_test_common.h"

using namespace ctest;

/* ====================== 正常场景 ====================== */

// 用例1: ts_session_new 单节点创建
TEST(TreeLifecycle, Case01_NewSingleNode) {
    CSession* s = ts_session_new(kHost, kPort, kUser, kPass);
    ASSERT_NE(s, nullptr) << ts_get_last_error();
    EXPECT_EQ(ts_session_open(s), TS_OK) << ts_get_last_error();
    closeDestroyTree(s);
}

// 用例2: ts_session_new_with_zone 带时区与 fetchSize，并可读回时区
TEST(TreeLifecycle, Case02_NewWithZone) {
    CSession* s = ts_session_new_with_zone(kHost, kPort, kUser, kPass, "Asia/Shanghai", 1024);
    ASSERT_NE(s, nullptr) << ts_get_last_error();
    ASSERT_EQ(ts_session_open(s), TS_OK) << ts_get_last_error();
    char buf[64] = {0};
    EXPECT_EQ(ts_session_get_timezone(s, buf, sizeof(buf)), TS_OK) << ts_get_last_error();
    EXPECT_STREQ(buf, "Asia/Shanghai");
    closeDestroyTree(s);
}

// 用例3: ts_session_new_multi_node 多节点 URL 创建并打开
TEST(TreeLifecycle, Case03_NewMultiNode) {
    const char* urls[] = {"127.0.0.1:6667"};
    CSession* s = ts_session_new_multi_node(urls, 1, kUser, kPass);
    ASSERT_NE(s, nullptr) << ts_get_last_error();
    ASSERT_EQ(ts_session_open(s), TS_OK) << ts_get_last_error();
    bool exists = false;
    EXPECT_EQ(ts_session_check_timeseries_exists(s, "root.__system.no.such", &exists), TS_OK);
    closeDestroyTree(s);
}

// 用例4: ts_session_open 普通打开
TEST(TreeLifecycle, Case04_Open) {
    CSession* s = ts_session_new(kHost, kPort, kUser, kPass);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(ts_session_open(s), TS_OK) << ts_get_last_error();
    closeDestroyTree(s);
}

// 用例5: ts_session_open_with_compression 启用 RPC 压缩
TEST(TreeLifecycle, Case05_OpenWithCompression) {
    CSession* s = ts_session_new(kHost, kPort, kUser, kPass);
    ASSERT_NE(s, nullptr);
    ASSERT_EQ(ts_session_open_with_compression(s, true), TS_OK) << ts_get_last_error();
    CSessionDataSet* ds = nullptr;
    EXPECT_EQ(ts_session_execute_query(s, "show databases", &ds), TS_OK) << ts_get_last_error();
    if (ds) ts_dataset_destroy(ds);
    closeDestroyTree(s);
}

// 用例6 + 用例87: close 后 destroy 的生命周期释放
TEST(TreeLifecycle, Case06_87_CloseThenDestroy) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(ts_session_close(s), TS_OK) << ts_get_last_error();
    ts_session_destroy(s);  // 不崩溃即通过
}

// 用例8: ts_session_get_timezone 获取默认时区
TEST(TreeLifecycle, Case08_GetTimezone) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    char buf[64] = {0};
    EXPECT_EQ(ts_session_get_timezone(s, buf, sizeof(buf)), TS_OK) << ts_get_last_error();
    EXPECT_GT(strlen(buf), 0u);
    closeDestroyTree(s);
}

// 用例9: ts_session_set_timezone 设置 Asia/Shanghai 后读回
TEST(TreeLifecycle, Case09_SetTimezone) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(ts_session_set_timezone(s, "Asia/Shanghai"), TS_OK) << ts_get_last_error();
    char buf[64] = {0};
    EXPECT_EQ(ts_session_get_timezone(s, buf, sizeof(buf)), TS_OK);
    EXPECT_STREQ(buf, "Asia/Shanghai");
    closeDestroyTree(s);
}

// 用例10: ts_session_get_timezone 缓冲区不足 -> 非 TS_OK，不越界
TEST(TreeLifecycle, Case10_GetTimezoneBufferTooSmall) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    char tiny[2] = {0};
    TsStatus st = ts_session_get_timezone(s, tiny, (int)sizeof(tiny));
    EXPECT_NE(st, TS_OK);
    EXPECT_GT(strlen(ts_get_last_error()), 0u);
    closeDestroyTree(s);
}

// 用例89: 重复生命周期 open/close 循环多轮稳定
TEST(TreeLifecycle, Case89_RepeatedLifecycle) {
    for (int i = 0; i < 5; i++) {
        CSession* s = ts_session_new(kHost, kPort, kUser, kPass);
        ASSERT_NE(s, nullptr) << ts_get_last_error();
        ASSERT_EQ(ts_session_open(s), TS_OK) << ts_get_last_error();
        CSessionDataSet* ds = nullptr;
        EXPECT_EQ(ts_session_execute_query(s, "show databases", &ds), TS_OK);
        if (ds) ts_dataset_destroy(ds);
        EXPECT_EQ(ts_session_close(s), TS_OK);
        ts_session_destroy(s);
    }
}

/* ====================== 异常场景 ====================== */

// 用例7: 用户名正确密码错误 -> open 失败，错误信息含 801
TEST(TreeLifecycleError, Case07_WrongPassword) {
    CSession* s = ts_session_new(kHost, kPort, kUser, kWrongPass);
    ASSERT_NE(s, nullptr);
    EXPECT_NE(ts_session_open(s), TS_OK);
    EXPECT_NE(std::string(ts_get_last_error()).find("801"), std::string::npos)
        << "实际错误: " << ts_get_last_error();
    ts_session_destroy(s);
}

// 用例90: 主机不可达/端口错误 -> 连接错误
TEST(TreeLifecycleError, Case90_ConnectionRefused) {
    CSession* s = ts_session_new("127.0.0.1", 6699, kUser, kPass);  // 6699 无监听
    ASSERT_NE(s, nullptr);
    EXPECT_NE(ts_session_open(s), TS_OK);
    EXPECT_GT(strlen(ts_get_last_error()), 0u);
    ts_session_destroy(s);
}

// 用例91: 用户名错误 -> open 失败
TEST(TreeLifecycleError, Case91_WrongUser) {
    CSession* s = ts_session_new(kHost, kPort, kWrongUser, kPass);
    ASSERT_NE(s, nullptr);
    EXPECT_NE(ts_session_open(s), TS_OK);
    EXPECT_GT(strlen(ts_get_last_error()), 0u);
    ts_session_destroy(s);
}

// 用例92: 已 open 再次 open，幂等或可读错误，不崩溃
TEST(TreeLifecycleError, Case92_DoubleOpen) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    TsStatus st = ts_session_open(s);
    (void)st;  // 不要求特定返回，只要求不崩溃且连接仍可用
    CSessionDataSet* ds = nullptr;
    EXPECT_EQ(ts_session_execute_query(s, "show databases", &ds), TS_OK);
    if (ds) ts_dataset_destroy(ds);
    closeDestroyTree(s);
}

// 用例93: ts_session_open(NULL) -> TS_ERR_NULL_PTR
TEST(TreeLifecycleError, Case93_OpenNull) {
    EXPECT_EQ(ts_session_open(nullptr), TS_ERR_NULL_PTR);
}

// 用例94: 非法 zoneId 创建会话，行为可控（不崩溃）
TEST(TreeLifecycleError, Case94_IllegalZone) {
    CSession* s = ts_session_new_with_zone(kHost, kPort, kUser, kPass, "Not/AZone", 1024);
    if (s) {
        TsStatus st = ts_session_open(s);
        if (st == TS_OK) {
            char buf[64] = {0};
            ts_session_get_timezone(s, buf, sizeof(buf));  // 读回值不强约束
        }
        closeDestroyTree(s);
    }
    SUCCEED();  // 关键是不崩溃
}

// 用例95: 设置非法时区 -> 非 TS_OK，且不影响后续
TEST(TreeLifecycleError, Case95_SetIllegalZone) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    char before[64] = {0};
    ts_session_get_timezone(s, before, sizeof(before));
    TsStatus st = ts_session_set_timezone(s, "Invalid/Zone");
    EXPECT_NE(st, TS_OK);
    char after[64] = {0};
    EXPECT_EQ(ts_session_get_timezone(s, after, sizeof(after)), TS_OK);
    EXPECT_STREQ(before, after) << "非法设置不应改变原时区";
    closeDestroyTree(s);
}

// 用例96: 未 open 直接 close，再 destroy，不崩溃
TEST(TreeLifecycleError, Case96_CloseWithoutOpen) {
    CSession* s = ts_session_new(kHost, kPort, kUser, kPass);
    ASSERT_NE(s, nullptr);
    ts_session_close(s);  // 可控状态码
    ts_session_destroy(s);
    SUCCEED();
}

// 用例97: 连续 close 两次幂等无崩溃
TEST(TreeLifecycleError, Case97_DoubleClose) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(ts_session_close(s), TS_OK);
    ts_session_close(s);  // 二次不崩溃
    ts_session_destroy(s);
    SUCCEED();
}

// 用例98: open 状态下直接 destroy（内部应关闭连接）
TEST(TreeLifecycleError, Case98_DestroyWhileOpen) {
    CSession* s = newOpenTreeSession();
    ASSERT_NE(s, nullptr);
    ts_session_destroy(s);  // 不显式 close
    SUCCEED();
}

// 用例99: ts_session_destroy(NULL) 为安全空操作
TEST(TreeLifecycleError, Case99_DestroyNull) {
    ts_session_destroy(nullptr);
    SUCCEED();
}

// 用例100: destroy 后对会话调用接口（use-after-free 防御）—— 仅验证不段错误。
// 注意：标准 use-after-free 行为未定义，此处不实际调用已释放句柄以避免未定义行为，
// 改为验证对 NULL 句柄调用业务接口被防御性拦截。
TEST(TreeLifecycleError, Case100_UseAfterFreeDefense) {
    EXPECT_NE(ts_session_execute_non_query(nullptr, "show databases"), TS_OK);
    SUCCEED();
}
