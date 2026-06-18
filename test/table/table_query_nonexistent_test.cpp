/*
 * 查询不存在的表 - 错误信息明确性（覆盖 V2-835）
 *
 * 背景（缺陷 V2-835）：查询一个不存在的表时，通过接口 ts_get_last_error 仅能拿到 "std::exception"，
 *   过于笼统，需优化成提示“表不存在”或内容更详细的报错。
 *   ts_get_last_error 透传底层异常的 e.what()；修复后服务端/客户端使 e.what() 携带明确的
 *   "Table 'xxx' does not exist." 信息，而非笼统的 std::exception。
 *
 * 本用例只验证“修复版本”的预期行为：查询不存在的表失败后，ts_get_last_error 返回的信息
 *   能明确指出“表不存在”，且不再是笼统的 "std::exception"。
 */
#include "c_test_common.h"

#include <string>
#include <cctype>

using namespace ctest;

namespace {
// 全小写化，便于大小写无关地匹配关键词
std::string toLower(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}
const char* kDb835 = "test_835_nonexistent";
const char* kNoSuchTable = "no_such_table_v2_835";
}  // namespace

// 用例：表模型查询不存在的表，ts_get_last_error 应给出“表不存在”的明确信息（非笼统 std::exception）
TEST(TableQueryNonexistent, Case_QueryNonexistentTable_ErrorMessageIsSpecific) {
    CTableSession* s = newOpenTableSession();
    ASSERT_NE(s, nullptr);

    // 准备一个干净的数据库并 USE（不创建目标表）
    tablePrepareDatabase(s, kDb835);

    // 查询不存在的表：必须失败
    CSessionDataSet* ds = nullptr;
    std::string sql = std::string("SELECT * FROM ") + kNoSuchTable;
    TsStatus st = ts_table_session_execute_query(s, sql.c_str(), &ds);
    EXPECT_NE(st, TS_OK) << "[V2-835 FAIL] 查询不存在的表应失败，却返回 TS_OK";

    // 取错误信息（立即复制，避免指针失效）
    std::string err = ts_get_last_error() ? ts_get_last_error() : "";
    std::cout << "[V2-835] ts_get_last_error => " << err << std::endl;

    // 断言 1：错误信息非空
    ASSERT_GT(err.size(), 0u) << "[V2-835 FAIL] ts_get_last_error 为空";

    // 断言 2：不再是笼统的 std::exception
    EXPECT_NE(toLower(err), "std::exception")
        << "[V2-835 FAIL] 错误信息仍为笼统的 std::exception: " << err;

    // 断言 3：信息能明确指向“表不存在”（兼容中英文/不同措辞：not exist / does not exist / 不存在）
    std::string low = toLower(err);
    bool specific = (low.find("not exist") != std::string::npos) ||
                    (low.find("does not exist") != std::string::npos) ||
                    (err.find("不存在") != std::string::npos) ||
                    (low.find("table") != std::string::npos && low.find("exist") != std::string::npos);
    EXPECT_TRUE(specific)
        << "[V2-835 FAIL] 错误信息未明确提示表不存在，实际为: " << err;

    if (ds) ts_dataset_destroy(ds);

    // 清理
    std::string drop = std::string("DROP DATABASE IF EXISTS ") + kDb835;
    ts_table_session_execute_non_query(s, drop.c_str());
    closeDestroyTable(s);
}
