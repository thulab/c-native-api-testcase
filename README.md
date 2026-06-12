# SessionC 原生接口自动化测试程序

-----

## 一、介绍

本程序基于 IoTDB **C 接口（SDK）** 头文件 `SessionC.h` 进行自动化测试，覆盖树模型 `CSession`
与表模型 `CTableSession` 的会话生命周期、Schema 管理、写入、查询、结果集、数据删除、
预编译语句、错误处理与资源释放、稳定性/并发等能力。

> 说明：C 与 C++ 客户端共用同一套 `include/` 与 `lib/`，差别仅在于 C 侧**只引用唯一头文件
> `SessionC.h`**。测试代码以 C++ 编译以便使用 gtest 框架，但访问 IoTDB 的能力只经过 `SessionC.h`，
> 不引用 `Session.h` / `TableSession.h` 等任何其它客户端头文件。

### 1、目录

```txt
├── client                      // 头文件和库文件目录（由 setup_client.sh 填充）
│   ├── include                 // SessionC.h + gtest/
│   └── lib                     // libiotdb_session.so（含 ts_* 与内嵌 thrift）/ libgtest.a
├── data                        // 预留测试数据目录
├── example                     // C 示例程序（仅 include SessionC.h，默认不参与测试构建）
│   ├── tree_example.c          // 树模型示例：建序列→写入→查询→清理
│   ├── table_example.c         // 表模型示例：建库/表→Tablet 写入→预编译/查询→清理
│   └── CMakeLists.txt          // 示例编译配置
├── test                        // 测试代码目录
│   ├── common                  // 公共头文件与公共用例
│   │   ├── c_test_common.h     // 连接参数与会话/查询辅助函数
│   │   ├── error_resource_test.cpp  // 错误处理与资源释放（用例 82-87,149-151）
│   │   └── stability_test.cpp       // 性能/稳定性/并发（用例 88,152,153）
│   ├── tree                    // 树模型用例
│   │   ├── tree_lifecycle_test.cpp  // 生命周期/鉴权/时区（1-10,89,90-100）
│   │   ├── tree_schema_test.cpp     // 库/序列管理（11-21,103-111）
│   │   ├── tree_tablet_test.cpp     // 公共 Tablet（22-30,101-102,119-124）
│   │   ├── tree_insert_test.cpp     // 写入（31-47,112-118）
│   │   ├── tree_query_test.cpp      // 查询/结果集/行记录（48-64,125-139）
│   │   └── tree_delete_test.cpp     // 数据删除（65-67,140-141）
│   ├── table                   // 表模型用例
│   │   ├── table_lifecycle_test.cpp     // 生命周期/DDL（68-71,142-143,147-148）
│   │   ├── table_write_query_test.cpp   // 写入与查询（72-77,144-146）
│   │   └── table_prepared_test.cpp      // 预编译语句（78-81）
│   ├── CMakeLists.txt          // 测试目标配置
│   └── main.cpp                // gtest 入口
├── CMakeLists.txt              // 主配置
├── setup_client.sh            // 拷贝头文件/库文件到 client/
├── compile.sh                  // 编译脚本
├── run.sh                      // 执行脚本（-m test / -m example）
├── README.md
└── REPORT.md                   // 每轮测试结果汇总与缺陷记录
```

### 2、项目内容

本仓库沉淀 SessionC 接口测试的可重复执行资产，当前入库内容包括：

- `test/`：154 个 gtest 测试函数，覆盖 CSV 台账中的 156 条需求用例（含新增 154-156 预编译误用用例）；
  测试函数统一使用 `CaseNN_` 前缀关联用例编号。
- `test/common/c_test_common.h`：连接参数、树模型/表模型会话 RAII 包装、SQL 执行、查询结果读取、
  Tablet 与清理辅助函数。
- `compile.sh` / `run.sh`：本地编译和执行入口，运行后生成
  `build/test/c_session_test_report.json`。
- `setup_client.sh`：从 IoTDB C/C++ 客户端构建产物中同步 `SessionC.h`、gtest 头文件和链接库到
  `client/`。
- `REPORT.md`：最近一次完整执行报告、缺陷记录、观察项和分类结果。
- `client/include/README.md`、`client/lib/README.md`：外部头文件与库文件占位说明。

以下内容不入库：`build/` 编译产物、`client/include/SessionC.h`、`client/include/gtest/`、
`client/lib/*.a`、`client/lib/*.so`。这些文件由 `setup_client.sh` 或本地构建生成。

### 3、准备头文件与库文件

C 与 C++ 共用同一套产物：

```bash
# 拉取并编译（含 client-cpp 模块，生成并解压 C/C++ 客户端 zip）
git clone https://github.com/apache/iotdb.git
cd iotdb
mvn package -DskipTests -P with-cpp -pl example/client-c-example,iotdb-client/client-cpp -am
# 头文件位于：iotdb-client/client-cpp/target/client-cpp-*-SNAPSHOT-cpp-linux-x86_64/include
# 库文件位于：iotdb-client/client-cpp/target/client-cpp-*-SNAPSHOT-cpp-linux-x86_64/lib
```

将 `SessionC.h`、`gtest/` 放入 `client/include`；将 `libiotdb_session.so`、`libgtest.a` 放入
`client/lib`。可直接执行脚本自动完成（路径可用环境变量覆盖）：

> 注：#17801 重构后 thrift 已内嵌进 `libiotdb_session.so`，新构建产物不再单独生成 `libthrift.a`，
> 链接与运行均不再依赖它（`setup_client.sh` 对其改为存在才拷贝；对齐 cpp-native-api-testcase 0020e3b）。

```bash
./setup_client.sh
# 或： CPP_CLIENT_DIR=/path/to/client-cpp-x.y.z-cpp-linux-x86_64 GTEST_DIR=/path/to/gtest ./setup_client.sh
```

> 注意：`libiotdb_session.so` 必须来自**包含 SessionC 的构建**（`nm -D` 可见 `ts_session_new` 等
> `ts_*` 符号），否则链接失败。

---

## 二、使用

连接参数在 `test/common/c_test_common.h` 顶部（默认 `127.0.0.1:6667`，`root` / `TimechoDB@2021`），
按实际环境修改。需先启动 IoTDB / TimechoDB 服务。

```bash
# 编译
./compile.sh
# 执行（生成 build/test/c_session_test_report.json）
./run.sh
# 只跑部分用例（gtest 过滤器）
./run.sh -m test --gtest_filter='TreeInsert.*'
# 跑 C 示例程序（需先在根 CMakeLists.txt 取消 add_subdirectory("example") 注释并重新编译）
./run.sh -m example
```

## 三、用例与需求对应

- 需求文档：`V2.0.10.1-接口模块-SessionC接口-需求分析`
- 测试用例台账：`原生接口测试用例 - 新增 C Session.csv`（共 156 条，含新增 154-156）
- 每个 `TEST` 以 `CaseNN_` 前缀标注对应用例序号，便于回填测试结论。

### 关于异常/边界用例的实现约定

- 正常路径（功能/写入/查询）：严格断言返回 `TS_OK` 且数据一致。
- 异常路径：依据需求“行为以实现为准”，断言**非 `TS_OK` 且进程不崩溃**；对头文件已明确的
  返回码（如空指针 `TS_ERR_NULL_PTR`、越界类型 `TS_TYPE_INVALID`）做精确断言。
- 「count 与数组长度不一致」类用例：C 契约下 `count` 大于数组长度属调用方越界（未定义行为），
  无法在不触发 UB 的前提下安全构造，统一改以**非法 `count(-1)`** 验证实现的防御性拦截，
  并在用例注释中标注（见 `Case105/109/112/118`）。
- 个别需求期望但当前实现未做客户端校验的点（如 `set_row_count` 上界）在用例注释中以 `NOTE`
  标注为「待确认项」。

## 三点五、代码覆盖率测试（SDK 源码覆盖率）

衡量本测试套件对 **SessionC SDK 源码**（`SessionC.cpp` 等）的覆盖程度，而非测试代码自身。
由于 `libiotdb_session.so` 默认是无插桩的预编译产物，需用 `--coverage` 重新编译 SDK 后再跑测试。

> 工具：`gcov` + `lcov`（≥1.15）+ `genhtml`；要求 `gcc` 与 `gcov` 主版本一致。

### 步骤

```bash
# 1) 给 SDK 注入覆盖率选项并重编（在 client-cpp 的 cmake 构建目录，-O0 避免行号失真）
SDK_BUILD=<client-cpp>/target/build/main
cd "$SDK_BUILD"
cp -f libiotdb_session.so libiotdb_session.so.orig.bak   # 备份原始非插桩 so
cmake -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage -O0 -g" \
      -DCMAKE_SHARED_LINKER_FLAGS="-fprofile-arcs -ftest-coverage" .
make -j$(nproc)                                          # 生成插桩 so + .gcno

# 2) 用插桩 so 替换测试程序的 client/lib，跑全量测试生成 .gcda
cp -f client/lib/libiotdb_session.so client/lib/libiotdb_session.so.orig.bak
cp -f "$SDK_BUILD/libiotdb_session.so" client/lib/
./compile.sh && ./run.sh

# 3) 采集并生成报告（必须剔除 thrift 自动生成代码，否则总覆盖率被严重稀释）
lcov --capture --directory "$SDK_BUILD" --output-file coverage/raw.info \
     --ignore-errors mismatch,gcov,source,negative,unused,empty
lcov --extract coverage/raw.info "*/generated-sources-cpp/*" --output-file coverage/sdk.info --ignore-errors unused,empty
lcov --remove coverage/sdk.info "*IClientRPCService*" "*_types.*" "*_constants.*" "*/thrift/*" \
     --output-file coverage/final.info --ignore-errors unused,empty
genhtml coverage/final.info --output-directory coverage/html --ignore-errors source,empty,category
```

### 覆盖率结果（最近一次）

- SDK 整体：行 58.6%，函数 56.0%（24 个手写源文件，已剔除 thrift 生成代码）
- **C 接口核心 `SessionC.cpp`：行 82.5%，函数 100%**（96 个 `ts_*` 接口全覆盖）

### 有意义覆盖原则

- **不为覆盖而覆盖**：`SessionC.cpp` 中约 52 行重复的 `session/tablet is null` 防御分支，代表性接口已验证，
  逐个接口补 NULL 测试无新缺陷发现价值，**不补**。
- **补真实误用路径**：预编译语句的真实误用（用例 154-156：漏绑参数 execute、set_string 传 NULL、
  new 传 NULL out_param_count）已补充，命中对应防御分支。
- 覆盖率产物（`coverage/`、`*.gcda/gcno/gcov/info`）不入库，详见 `.gitignore`。
- 覆盖率测完后应将 `client/lib` 与 SDK 的 `.so` 还原为原始非插桩版（`*.orig.bak`）。

---

## 四、运行测试规则

执行/维护本测试程序时遵守以下规则：

1. **运行测试由测试人员自行完成**（编译 → 启动服务 → 运行 → 分析结果）。
2. **缺陷归类处理**：
   - 若失败原因是**被测源码（SessionC SDK / 服务端）问题**：不修改测试代码，将问题**汇总记录到
     本项目根目录的 `REPORT.md`**（含用例号、现象、期望、实际、定位线索）。
   - 若失败原因是**测试代码自身问题**：直接**修改测试代码**修复，使其正确反映需求。
3. **测试报告**：每轮测试结束后的结果汇总（通过/失败/跳过统计、失败明细、结论）统一写入
   `REPORT.md`。原始 gtest JSON 报告位于 `build/test/c_session_test_report.json`。

## 五、集群环境与激活信息

测试运行于 TimechoDB 集群（colony 配置）。机器码用于厂商签发激活码，可通过 leader 上
`show system info` 或各节点 `activation/system_info` 文件获取：

| 节点 | 角色 | 机器码（SystemInfo） |
|---|---|---|
| 172.20.31.63 | leader | `02-URA5CM25-MJJRUN77-AM2V36PJ` |
| 172.20.31.65 | 节点   | `02-VU2NQAJV-TKM27GBC-ZVE6Y2ZF` |
| 172.20.31.70 | 节点   | `02-YB2NG77V-4EFDQT6U-5PTDH7AA` |

聚合机器码（leader `show system info` 返回，可整串提交厂商）：

```
02-URA5CM25-MJJRUN77-AM2V36PJ,02-VU2NQAJV-TKM27GBC-ZVE6Y2ZF,02-YB2NG77V-4EFDQT6U-5PTDH7AA
```

- 安装目录：`/data/iotdb-test/timechodb-2.0.10.1-bin-rc1-colony`；启动需先
  `export JAVA_HOME=/data/iotdb-test/tool/jdk/jdk-17.0.12（.70 节点为 tools/jdk，复数）`。
- 激活/启动顺序：**先单独启动 63 leader 的 CN+DN → 在 63 激活（此时仅 1 个 ConfigNode，1 个激活码即可）
  → 再启动 65 / 70 的 CN+DN（加入已激活集群自动跟随，显示 ACTIVATED(W)）**；集群读写恢复后方可复跑全量测试。
- 注意：全新重建（删 data）后机器码会变，旧激活码失效，须用重建后 `show system info` 的新机器码重新签发。
- 历史：license 于 **2026-06-06 过期**导致集群只读，需重新激活。
