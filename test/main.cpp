#include "gtest/gtest.h"

// SessionC 自动化测试入口
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
