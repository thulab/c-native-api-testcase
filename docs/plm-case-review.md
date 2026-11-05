# C 原生接口用例导入审查（2026-11-05）

基线：`thulab/c-native-api-testcase` `main@2eb5e09`。共 **146** 条测试函数，与 `--gtest_list_tests` 一致。

## 一、总体分布

| 模块 | 功能点路径 | 用例数 |
| --- | --- | --- |
| 树模型-生命周期 | `接口模块/C 原生接口/树模型-生命周期` | 22 |
| 树模型-Schema | `接口模块/C 原生接口/树模型-Schema` | 20 |
| 公共-Tablet | `接口模块/C 原生接口/公共-Tablet` | 16 |
| 树模型-写入 | `接口模块/C 原生接口/树模型-写入` | 24 |
| 树模型-查询 | `接口模块/C 原生接口/树模型-查询` | 31 |
| 树模型-删除 | `接口模块/C 原生接口/树模型-删除` | 5 |
| 表模型-生命周期 | `接口模块/C 原生接口/表模型-生命周期` | 8 |
| 表模型-写入与查询 | `接口模块/C 原生接口/表模型-写入与查询` | 8 |
| 表模型-查询 | `接口模块/C 原生接口/表模型-查询` | 1 |
| 公共-错误处理与资源 | `接口模块/C 原生接口/公共-错误处理与资源` | 9 |
| 公共-稳定性 | `接口模块/C 原生接口/公共-稳定性` | 2 |

## 二、用例清单

### 树模型-生命周期

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 1 | C 原生｜树模型-生命周期：ts_session_new 单节点创建 | `TreeLifecycle.Case01_NewSingleNode` | P1 | 1 |  |
| 2 | C 原生｜树模型-生命周期：ts_session_new_with_zone 带时区与 fetchSize，并可读回时区 | `TreeLifecycle.Case02_NewWithZone` | P1 | 2 |  |
| 3 | C 原生｜树模型-生命周期：ts_session_new_multi_node 多节点 URL 创建并打开 | `TreeLifecycle.Case03_NewMultiNode` | P1 | 3 |  |
| 4 | C 原生｜树模型-生命周期：ts_session_open 普通打开 | `TreeLifecycle.Case04_Open` | P1 | 4 |  |
| 5 | C 原生｜树模型-生命周期：ts_session_open_with_compression 启用 RPC 压缩 | `TreeLifecycle.Case05_OpenWithCompression` | P1 | 5 |  |
| 6 | C 原生｜树模型-生命周期：close 后 destroy 的生命周期释放 | `TreeLifecycle.Case06_87_CloseThenDestroy` | P1 | 6 + 用例87 |  |
| 7 | C 原生｜树模型-生命周期：ts_session_get_timezone 获取默认时区 | `TreeLifecycle.Case08_GetTimezone` | P1 | 8 |  |
| 8 | C 原生｜树模型-生命周期：ts_session_set_timezone 设置 Asia/Shanghai 后读回 | `TreeLifecycle.Case09_SetTimezone` | P1 | 9 |  |
| 9 | C 原生｜树模型-生命周期：ts_session_get_timezone 缓冲区不足 -> 非 TS_OK，不越界 | `TreeLifecycle.Case10_GetTimezoneBufferTooSmall` | P1 | 10 |  |
| 10 | C 原生｜树模型-生命周期：重复生命周期 open/close 循环多轮稳定 | `TreeLifecycle.Case89_RepeatedLifecycle` | P1 | 89 |  |
| 11 | C 原生｜树模型-生命周期：用户名正确密码错误 -> open 失败，错误信息含 801 | `TreeLifecycleError.Case07_WrongPassword` | P2 | 7 |  |
| 12 | C 原生｜树模型-生命周期：主机不可达/端口错误 -> 连接错误 | `TreeLifecycleError.Case90_ConnectionRefused` | P2 | 90 |  |
| 13 | C 原生｜树模型-生命周期：用户名错误 -> open 失败 | `TreeLifecycleError.Case91_WrongUser` | P2 | 91 |  |
| 14 | C 原生｜树模型-生命周期：已 open 再次 open，幂等或可读错误，不崩溃 | `TreeLifecycleError.Case92_DoubleOpen` | P2 | 92 |  |
| 15 | C 原生｜树模型-生命周期：ts_session_open(NULL) -> TS_ERR_NULL_PTR | `TreeLifecycleError.Case93_OpenNull` | P2 | 93 |  |
| 16 | C 原生｜树模型-生命周期：非法 zoneId 创建会话，行为可控（不崩溃） | `TreeLifecycleError.Case94_IllegalZone` | P2 | 94 |  |
| 17 | C 原生｜树模型-生命周期：设置非法时区 -> 非 TS_OK，且不影响后续 | `TreeLifecycleError.Case95_SetIllegalZone` | P2 | 95 |  |
| 18 | C 原生｜树模型-生命周期：未 open 直接 close，再 destroy，不崩溃 | `TreeLifecycleError.Case96_CloseWithoutOpen` | P2 | 96 |  |
| 19 | C 原生｜树模型-生命周期：连续 close 两次幂等无崩溃 | `TreeLifecycleError.Case97_DoubleClose` | P2 | 97 |  |
| 20 | C 原生｜树模型-生命周期：open 状态下直接 destroy（内部应关闭连接） | `TreeLifecycleError.Case98_DestroyWhileOpen` | P2 | 98 |  |
| 21 | C 原生｜树模型-生命周期：ts_session_destroy(NULL) 为安全空操作 | `TreeLifecycleError.Case99_DestroyNull` | P2 | 99 |  |
| 22 | C 原生｜树模型-生命周期：destroy 后对会话调用接口（use-after-free 防御）—— 仅验证不段错误。 | `TreeLifecycleError.Case100_UseAfterFreeDefense` | P2 | 100 |  |

### 树模型-Schema

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 23 | C 原生｜树模型-Schema：创建数据库 | `TreeSchema.Case11_CreateDatabase` | P1 | 11 |  |
| 24 | C 原生｜树模型-Schema：删除（空）数据库 | `TreeSchema.Case12_DeleteDatabase` | P1 | 12 |  |
| 25 | C 原生｜树模型-Schema：批量删除数据库 | `TreeSchema.Case13_DeleteDatabases` | P1 | 13 |  |
| 26 | C 原生｜树模型-Schema：创建单条时间序列 + 存在性检查 | `TreeSchema.Case14_CreateTimeseries` | P1 | 14 |  |
| 27 | C 原生｜树模型-Schema：扩展建序列（属性/标签/别名） | `TreeSchema.Case15_CreateTimeseriesEx` | P1 | 15 |  |
| 28 | C 原生｜树模型-Schema：批量建序列 | `TreeSchema.Case16_CreateMultiTimeseries` | P1 | 16 |  |
| 29 | C 原生｜树模型-Schema：创建对齐时间序列 | `TreeSchema.Case17_CreateAlignedTimeseries` | P1 | 17 |  |
| 30 | C 原生｜树模型-Schema：已存在路径 exists=true | `TreeSchema.Case18_CheckExistsTrue` | P1 | 18 |  |
| 31 | C 原生｜树模型-Schema：不存在路径 exists=false | `TreeSchema.Case19_CheckExistsFalse` | P1 | 19 |  |
| 32 | C 原生｜树模型-Schema：单条删除后 exists=false | `TreeSchema.Case20_DeleteTimeseries` | P1 | 20 |  |
| 33 | C 原生｜树模型-Schema：批量删除时间序列 | `TreeSchema.Case21_DeleteTimeseriesBatch` | P1 | 21 |  |
| 34 | C 原生｜树模型-Schema：重复创建同名数据库 -> 非 TS_OK | `TreeSchemaError.Case103_CreateDuplicateDatabase` | P2 | 103 |  |
| 35 | C 原生｜树模型-Schema：删除不存在数据库 -> 行为可控（非 TS_OK 或幂等） | `TreeSchemaError.Case104_DeleteNonexistDatabase` | P2 | 104 |  |
| 36 | C 原生｜树模型-Schema：delete_databases count 与数组不一致 -> 非 TS_OK，不越界。 | `TreeSchemaError.Case105_DeleteDatabasesCountMismatch` | P2 | 105 |  |
| 37 | C 原生｜树模型-Schema：重复创建同一 timeseries -> 非 TS_OK | `TreeSchemaError.Case106_CreateDuplicateTimeseries` | P2 | 106 |  |
| 38 | C 原生｜树模型-Schema：非法路径（不以 root. 开头）-> 非 TS_OK | `TreeSchemaError.Case107_IllegalPath` | P2 | 107 |  |
| 39 | C 原生｜树模型-Schema：类型与编码不兼容（BOOLEAN + GORILLA）-> 非 TS_OK | `TreeSchemaError.Case108_IncompatibleTypeEncoding` | P2 | 108 |  |
| 40 | C 原生｜树模型-Schema：对齐序列数组与 count 不一致 -> 非 TS_OK，不崩溃。 | `TreeSchemaError.Case109_AlignedCountMismatch` | P2 | 109 |  |
| 41 | C 原生｜树模型-Schema：check_timeseries_exists exists 出参为 NULL -> 非 TS_OK | `TreeSchemaError.Case110_CheckExistsNullOut` | P2 | 110 |  |
| 42 | C 原生｜树模型-Schema：删除不存在序列 -> 行为可控，不崩溃 | `TreeSchemaError.Case111_DeleteNonexistTimeseries` | P2 | 111 |  |

### 公共-Tablet

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 43 | C 原生｜公共-Tablet：ts_tablet_new 基础创建 | `Tablet.Case22_New` | P1 | 22 |  |
| 44 | C 原生｜公共-Tablet：ts_tablet_new_with_category 带列类别创建 | `Tablet.Case23_NewWithCategory` | P1 | 23 |  |
| 45 | C 原生｜公共-Tablet：新建 Tablet 行数初始为 0 | `Tablet.Case24_InitialRowCount` | P1 | 24 |  |
| 46 | C 原生｜公共-Tablet：add_timestamp + add_value_int64 写入并回读校验 | `Tablet.Case25_AddTimestampAndInt64` | P1 | 25 |  |
| 47 | C 原生｜公共-Tablet：多类型写值（bool/int32/float/double/string）并回读 | `Tablet.Case26_MultiTypeValues` | P1 | 26 |  |
| 48 | C 原生｜公共-Tablet：set_row_count 控制有效行数，insert 仅写前 N 行 | `Tablet.Case28_SetRowCount` | P1 | 28 |  |
| 49 | C 原生｜公共-Tablet：reset 清状态后复用同一 Tablet | `Tablet.Case29_Reset` | P1 | 29 |  |
| 50 | C 原生｜公共-Tablet：非法 row/col 索引写入 -> 期望非 TS_OK 且不崩溃。 | `TabletError.Case30_IndexOutOfRange` | P2 | 30 | GTEST_SKIP（源码缺陷跟踪） |
| 51 | C 原生｜公共-Tablet：Tablet 重复销毁无 double free | `TabletError.Case101_DoubleDestroy` | P2 | 101 |  |
| 52 | C 原生｜公共-Tablet：insert 不转移所有权 -> reset 后复用再次 insert，最后单次 destroy | `Tablet.Case102_ReuseAfterInsert` | P1 | 102 |  |
| 53 | C 原生｜公共-Tablet：columnCount=0 创建 -> 句柄可控（非空亦可，后续接口安全） | `TabletError.Case119_ZeroColumns` | P2 | 119 |  |
| 54 | C 原生｜公共-Tablet：columnNames/dataTypes 与 columnCount 不一致。 | `TabletError.Case120_CountArrayConsistency` | P2 | 120 |  |
| 55 | C 原生｜公共-Tablet：set_row_count(rowCount > maxRowNumber) | `TabletError.Case121_SetRowCountOverMax` | P2 | 121 |  |
| 56 | C 原生｜公共-Tablet：add_timestamp rowIndex >= maxRowNumber -> 期望非 TS_OK。 | `TabletError.Case122_AddTimestampRowOutOfRange` | P2 | 122 | GTEST_SKIP（源码缺陷跟踪） |
| 57 | C 原生｜公共-Tablet：类型不匹配 setter（对 INT64 列调用 string setter）-> 可控 | `TabletError.Case123_TypeMismatchSetter` | P2 | 123 |  |
| 58 | C 原生｜公共-Tablet：add_value_int64(NULL,...) -> TS_ERR_NULL_PTR | `TabletError.Case124_AddValueNullTablet` | P2 | 124 |  |

### 树模型-写入

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 59 | C 原生｜树模型-写入：insert_record_str 字符串值单条写入 | `TreeInsert.Case31_InsertRecordStr` | P1 | 31 |  |
| 60 | C 原生｜树模型-写入：insert_record 强类型 INT32/DOUBLE/INT64 | `TreeInsert.Case32_InsertRecordTyped` | P1 | 32 |  |
| 61 | C 原生｜树模型-写入：insert_record 新类型 TIMESTAMP/DATE/BLOB/STRING | `TreeInsert.Case33_InsertRecordNewTypes` | P1 | 33 |  |
| 62 | C 原生｜树模型-写入：insert_records_str 多设备批量字符串 | `TreeInsert.Case34_InsertRecordsStr` | P1 | 34 |  |
| 63 | C 原生｜树模型-写入：insert_records 强类型批量 | `TreeInsert.Case35_InsertRecordsTyped` | P1 | 35 |  |
| 64 | C 原生｜树模型-写入：insert_records_of_one_device sorted=true | `TreeInsert.Case36_InsertRecordsOfOneDevice` | P1 | 36 |  |
| 65 | C 原生｜树模型-写入：insert_aligned_record_str | `TreeInsert.Case37_AlignedRecordStr` | P1 | 37 |  |
| 66 | C 原生｜树模型-写入：insert_aligned_record 强类型 | `TreeInsert.Case38_AlignedRecordTyped` | P1 | 38 |  |
| 67 | C 原生｜树模型-写入：insert_aligned_records_str | `TreeInsert.Case39_AlignedRecordsStr` | P1 | 39 |  |
| 68 | C 原生｜树模型-写入：insert_aligned_records 强类型 | `TreeInsert.Case40_AlignedRecordsTyped` | P1 | 40 |  |
| 69 | C 原生｜树模型-写入：insert_aligned_records_of_one_device | `TreeInsert.Case41_AlignedRecordsOfOneDevice` | P1 | 41 |  |
| 70 | C 原生｜树模型-写入：insert_tablet 单 Tablet | `TreeInsert.Case42_InsertTablet` | P1 | 42 |  |
| 71 | C 原生｜树模型-写入：insert_tablets 多设备 | `TreeInsert.Case43_InsertTablets` | P1 | 43 |  |
| 72 | C 原生｜树模型-写入：insert_aligned_tablet 对齐单 Tablet | `TreeInsert.Case44_AlignedTablet` | P1 | 44 |  |
| 73 | C 原生｜树模型-写入：insert_aligned_tablets 多对齐 Tablet | `TreeInsert.Case45_AlignedTablets` | P1 | 45 |  |
| 74 | C 原生｜树模型-写入：对非对齐设备使用对齐写入 -> 服务端不报错（改写 isAligned） | `TreeInsert.Case46_AlignedWriteToNonAligned` | P1 | 46 |  |
| 75 | C 原生｜树模型-写入：对对齐设备使用非对齐写入 -> 服务端不报错（改写 isAligned） | `TreeInsert.Case47_NonAlignedWriteToAligned` | P1 | 47 |  |
| 76 | C 原生｜树模型-写入：insert_record_str count 与 measurements/values 数量不符 -> 非 TS_OK。 | `TreeInsertError.Case112_RecordStrCountMismatch` | P2 | 112 |  |
| 77 | C 原生｜树模型-写入：insert_record 值类型与序列声明不符 -> 非 TS_OK 或按服务端规则 | `TreeInsertError.Case113_RecordTypeMismatch` | P2 | 113 |  |
| 78 | C 原生｜树模型-写入：insert_record_str(NULL session) -> TS_ERR_NULL_PTR | `TreeInsertError.Case114_RecordStrNullSession` | P2 | 114 |  |
| 79 | C 原生｜树模型-写入：空测点 count=0 -> 可控，不崩溃 | `TreeInsertError.Case115_EmptyMeasurements` | P2 | 115 |  |
| 80 | C 原生｜树模型-写入：insert_tablet 未设置行数（row_count=0）-> 写 0 行或非 OK，不崩溃 | `TreeInsertError.Case116_InsertTabletNoRowCount` | P2 | 116 |  |
| 81 | C 原生｜树模型-写入：Tablet 列类型与序列不一致 -> 非 TS_OK | `TreeInsertError.Case117_TabletColumnTypeMismatch` | P2 | 117 |  |
| 82 | C 原生｜树模型-写入：insert_tablets tabletCount 与数组不一致 -> 不越界、不崩溃。 | `TreeInsertError.Case118_InsertTabletsCountMismatch` | P2 | 118 |  |

### 树模型-查询

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 83 | C 原生｜树模型-查询：execute_query 普通查询 | `TreeQuery.Case48_ExecuteQuery` | P1 | 48 |  |
| 84 | C 原生｜树模型-查询：execute_query_with_timeout | `TreeQuery.Case49_QueryWithTimeout` | P1 | 49 |  |
| 85 | C 原生｜树模型-查询：execute_non_query 执行 insert/DDL | `TreeQuery.Case50_ExecuteNonQuery` | P1 | 50 |  |
| 86 | C 原生｜树模型-查询：raw_data_query 正常时间范围 | `TreeQuery.Case51_RawDataQuery` | P1 | 51 |  |
| 87 | C 原生｜树模型-查询：raw_data_query 负时间戳范围 | `TreeQuery.Case52_RawDataQueryNegativeTime` | P1 | 52 |  |
| 88 | C 原生｜树模型-查询：raw_data_query 包含空值列（部分列在某时间点为空） | `TreeQuery.Case53_RawDataQueryWithNull` | P1 | 53 |  |
| 89 | C 原生｜树模型-查询：last_data_query 不带 lastTime | `TreeQuery.Case54_LastDataQuery` | P1 | 54 |  |
| 90 | C 原生｜树模型-查询：last_data_query_with_time 带 lastTime | `TreeQuery.Case55_LastDataQueryWithTime` | P1 | 55 |  |
| 91 | C 原生｜树模型-查询：last_data_query_with_time future 时间无结果 | `TreeQuery.Case56_LastDataQueryFutureEmpty` | P1 | 56 |  |
| 92 | C 原生｜树模型-查询：dataset_get_column_count | `TreeQuery.Case57_ColumnCount` | P1 | 57 |  |
| 93 | C 原生｜树模型-查询：dataset_get_column_name / column_type | `TreeQuery.Case58_ColumnNameType` | P1 | 58 |  |
| 94 | C 原生｜树模型-查询：set_fetch_size 大于 1 的批大小 | `TreeQuery.Case59_SetFetchSize` | P1 | 59 |  |
| 95 | C 原生｜树模型-查询：row timestamp + field_count | `TreeQuery.Case60_RowTimestampFieldCount` | P1 | 60 |  |
| 96 | C 原生｜树模型-查询：row get bool/int32/int64/float/double/string | `TreeQuery.Case61_RowTypedGetters` | P1 | 61 |  |
| 97 | C 原生｜树模型-查询：row get_data_type | `TreeQuery.Case62_RowDataType` | P1 | 62 |  |
| 98 | C 原生｜树模型-查询：row is_null 空字段识别 | `TreeQuery.Case63_RowIsNull` | P1 | 63 |  |
| 99 | C 原生｜树模型-查询：语法错误 SQL -> 非 TS_OK | `TreeQueryError.Case125_SyntaxError` | P2 | 125 |  |
| 100 | C 原生｜树模型-查询：空结果集 | `TreeQueryError.Case126_EmptyResult` | P2 | 126 |  |
| 101 | C 原生｜树模型-查询：execute_query dataSet 出参为 NULL -> 非 TS_OK | `TreeQueryError.Case127_QueryNullOut` | P2 | 127 |  |
| 102 | C 原生｜树模型-查询：非法非查询 SQL -> 非 TS_OK | `TreeQueryError.Case128_NonQueryIllegal` | P2 | 128 |  |
| 103 | C 原生｜树模型-查询：raw_data_query pathCount=0 | `TreeQueryError.Case129_RawDataQueryZeroPaths` | P2 | 129 |  |
| 104 | C 原生｜树模型-查询：raw_data_query startTime>endTime | `TreeQueryError.Case130_RawDataQueryReversedTime` | P2 | 130 |  |
| 105 | C 原生｜树模型-查询：last_data_query 不存在 path -> 空集 | `TreeQueryError.Case131_LastDataQueryNonexist` | P2 | 131 |  |
| 106 | C 原生｜树模型-查询：column_name 越界 -> 返回空（实现返回 ""），不崩溃 | `TreeQueryError.Case132_ColumnNameOutOfRange` | P2 | 132 |  |
| 107 | C 原生｜树模型-查询：column_type 越界（index=-1）-> 返回空，不崩溃 | `TreeQueryError.Case133_ColumnTypeOutOfRange` | P2 | 133 |  |
| 108 | C 原生｜树模型-查询：结果集耗尽后再 next -> NULL | `TreeQueryError.Case134_NextAfterExhausted` | P2 | 134 |  |
| 109 | C 原生｜树模型-查询：dataSet use-after-free 防御（以 NULL 句柄验证不崩溃） | `TreeQueryError.Case135_DatasetUseAfterFreeDefense` | P2 | 135 |  |
| 110 | C 原生｜树模型-查询：row get_data_type 越界 -> TS_TYPE_INVALID | `TreeQueryError.Case136_DataTypeOutOfRange` | P2 | 136 |  |
| 111 | C 原生｜树模型-查询：getter 类型不匹配（对文本列调用 int getter）-> 可控，不崩溃 | `TreeQueryError.Case137_GetterTypeMismatch` | P2 | 137 |  |
| 112 | C 原生｜树模型-查询：row is_null 越界 -> 返回 true，不崩溃 | `TreeQueryError.Case138_IsNullOutOfRange` | P2 | 138 |  |
| 113 | C 原生｜树模型-查询：set_fetch_size <= 0 边界 | `TreeQueryError.Case139_FetchSizeNonPositive` | P2 | 139 |  |

### 树模型-删除

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 114 | C 原生｜树模型-删除：delete_data 单路径按 endTime 删除 | `TreeDelete.Case65_DeleteData` | P1 | 65 |  |
| 115 | C 原生｜树模型-删除：delete_data_batch 多路径按 endTime 删除 | `TreeDelete.Case66_DeleteDataBatch` | P1 | 66 |  |
| 116 | C 原生｜树模型-删除：delete_data_range 多路径时间区间删除 | `TreeDelete.Case67_DeleteDataRange` | P1 | 67 |  |
| 117 | C 原生｜树模型-删除：delete_data_range startTime>endTime -> 非 TS_OK 或不删除 | `TreeDeleteError.Case140_RangeReversed` | P2 | 140 |  |
| 118 | C 原生｜树模型-删除：delete_data_batch 含不存在路径 -> 可控，不崩溃 | `TreeDeleteError.Case141_BatchWithNonexist` | P2 | 141 |  |

### 表模型-生命周期

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 119 | C 原生｜表模型-生命周期：ts_table_session_new database 为空字符串可创建/打开 | `TableLifecycle.Case68_NewEmptyDatabase` | P1 | 68 |  |
| 120 | C 原生｜表模型-生命周期：ts_table_session_new_multi_node | `TableLifecycle.Case69_NewMultiNode` | P1 | 69 |  |
| 121 | C 原生｜表模型-生命周期：open/close/destroy 正常释放 | `TableLifecycle.Case70_OpenCloseDestroy` | P1 | 70 |  |
| 122 | C 原生｜表模型-生命周期：DDL 建库/切库/建表，SHOW TABLES 可见 | `TableLifecycle.Case71_DDL` | P1 | 71 |  |
| 123 | C 原生｜表模型-生命周期：ts_table_session_new database=NULL（可空性以实现为准） | `TableLifecycleError.Case142_NullDatabase` | P2 | 142 |  |
| 124 | C 原生｜表模型-生命周期：主机不可达 -> 连接错误。 | `TableLifecycleError.Case143_ConnectionRefused` | P2 | 143 |  |
| 125 | C 原生｜表模型-生命周期：close 后再 execute -> 期望非 TS_OK。 | `TableLifecycleError.Case147_ExecuteAfterClose` | P2 | 147 | GTEST_SKIP（源码缺陷跟踪） |
| 126 | C 原生｜表模型-生命周期：destroy 后调用（use-after-free 防御）—— 以 NULL 句柄验证不崩溃 | `TableLifecycleError.Case148_UseAfterFreeDefense` | P2 | 148 |  |

### 表模型-写入与查询

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 127 | C 原生｜表模型-写入与查询：带列类别 Tablet 写入（TAG+ATTRIBUTE+FIELD） | `TableWrite.Case72_InsertCategoryTablet` | P1 | 72 |  |
| 128 | C 原生｜表模型-写入与查询：普通查询 SELECT * | `TableWrite.Case73_SelectAll` | P1 | 73 |  |
| 129 | C 原生｜表模型-写入与查询：超时查询 | `TableWrite.Case74_QueryWithTimeout` | P1 | 74 |  |
| 130 | C 原生｜表模型-写入与查询：表模型结果集列信息 + 行记录元信息 | `TableWrite.Case75_DatasetColumnInfo` | P1 | 75 |  |
| 131 | C 原生｜表模型-写入与查询：多类型 Tablet 写入与回读 | `TableWrite.Case76_MultiTypeTablet` | P1 | 76 |  |
| 132 | C 原生｜表模型-写入与查询：语法错误 SQL -> 非 TS_OK | `TableWriteError.Case144_SyntaxError` | P2 | 144 |  |
| 133 | C 原生｜表模型-写入与查询：未 USE 数据库 / 表不存在写入 -> 非 TS_OK。 | `TableWriteError.Case145_InsertNonexistTable` | P2 | 145 |  |
| 134 | C 原生｜表模型-写入与查询：Tablet 列类别与表 schema 不符 -> 非 TS_OK | `TableWriteError.Case146_ColumnCategoryMismatch` | P2 | 146 |  |

### 表模型-查询

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 135 | C 原生｜表模型-查询：Case_QueryNonexistentTable_ErrorMessageIsSpecific | `TableQueryNonexistent.Case_QueryNonexistentTable_ErrorMessageIsSpecific` | P1 |  |  |

### 公共-错误处理与资源

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 136 | C 原生｜公共-错误处理与资源：失败调用后 ts_get_last_error 返回最近一次错误信息 | `ErrorResource.Case82_LastErrorAfterFailure` | P2 | 82 |  |
| 137 | C 原生｜公共-错误处理与资源：last_error 指针只保证到下一次 C API 调用前有效 -> 需复制内容 | `ErrorResource.Case83_LastErrorPointerLifecycle` | P2 | 83 |  |
| 138 | C 原生｜公共-错误处理与资源：dataSet 释放稳定性（遍历完/未遍历完均可 destroy） | `ErrorResource.Case84_DatasetDestroy` | P2 | 84 |  |
| 139 | C 原生｜公共-错误处理与资源：逐行 ts_row_record_destroy 无泄漏/崩溃 | `ErrorResource.Case85_RowRecordDestroy` | P2 | 85 |  |
| 140 | C 原生｜公共-错误处理与资源：insert 后不转移 Tablet 所有权，写入后仍由调用方 destroy | `ErrorResource.Case86_TabletOwnership` | P2 | 86 |  |
| 141 | C 原生｜公共-错误处理与资源：close 仅关闭连接，destroy 仍需显式调用 | `ErrorResource.Case87_CloseThenDestroy` | P2 | 87 |  |
| 142 | C 原生｜公共-错误处理与资源：无失败调用时 ts_get_last_error 返回值可控（在新线程内验证） | `ErrorResource.Case149_LastErrorInitialState` | P2 | 149 |  |
| 143 | C 原生｜公共-错误处理与资源：成功调用后语义（成功不保证清空错误信息） | `ErrorResource.Case150_LastErrorAfterSuccess` | P2 | 150 |  |
| 144 | C 原生｜公共-错误处理与资源：ts_get_last_error 线程局部，A 线程失败不影响 B 线程 | `ErrorResource.Case151_LastErrorThreadLocal` | P2 | 151 |  |

### 公共-稳定性

| # | PLM 用例名 | gtest target | 优先级 | 需求编号 | 备注 |
| --- | --- | --- | --- | --- | --- |
| 145 | C 原生｜公共-稳定性：大结果集稳定遍历，逐行释放，行数一致 | `Stability.Case88_153_LargeResultSet` | P1 | 88 + 用例153 |  |
| 146 | C 原生｜公共-稳定性：多线程各自独立会话并发读写，数据互不串扰，无崩溃 | `Stability.Case152_ConcurrentSessions` | P1 | 152 |  |

## 三、待你确认的分歧项

1. **用例总数口径**：`--gtest_list_tests` 为 146；`REPORT.md` 记载需求台账 156、测试函数 154（6&87、79&80、88&153 各合并为一个函数，另 3 条跳过）。本清单按可执行的 146 个 gtest 函数生成，一个函数对应一个 PLM 用例。
2. **跳过用例**：30、122（Tablet 越界，源码缺陷 #1）与 147（close 后 execute 仍返回 TS_OK，源码缺陷 #2）在代码中 `GTEST_SKIP`。导入后 PLM 会显示为跳过；是否保留由你决定。
3. **C 的 K8s 重定向风险**：`SessionC.h` 没有关闭重定向的接口，而 C++ 已用 `enableRedirections=false` 规避。计划用一条最简用例连 K8s 实例做 spike；若确实重定向，再决定补 C 客户端能力或调整接入方式。
4. **默认口令变更**：`c_test_common.h` 的连接参数改为环境变量优先，默认口令从 `root/root` 改为 PLM 环境的 `TimechoDB@2021`。111 上独立运行需设置 `IOTDB_USER=root IOTDB_PASSWORD=root`。
5. **Tablet 用例归类**：Tablet 相关用例（22-30、101-102、119-124）归入「公共-Tablet」，模型记为「通用」，与 C++ 的功能点口径保持一致。
6. **合并用例命名**：`Case06_87`、`Case88_153` 这类一个测试函数覆盖两个需求编号的，PLM 里只建一条用例，名称沿用「用例6+87」「用例88/153」的原始注释。

## 四、验证记录

- 76 执行机：`sh compile.sh` 成功，`./build/test/main --gtest_list_tests` 输出 146 条。
- 单条执行与 K8s 重定向 spike 结果待补。
