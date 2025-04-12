#pragma once

#include <string>
#include <vector>
#include <memory>

#include "common/value.h"

namespace sql {

// SQL命令类型
enum class SqlCommandFlag {
  SCF_ERROR = 0,         // 错误

  // 数据定义语言(DDL)
  SCF_CREATE_TABLE,     // CREATE TABLE
  SCF_DROP_TABLE,       // DROP TABLE
  SCF_CREATE_INDEX,     // CREATE INDEX
  SCF_DROP_INDEX,       // DROP INDEX
  SCF_ALTER_TABLE,      // ALTER TABLE
  SCF_TRUNCATE_TABLE,   // TRUNCATE TABLE
  SCF_CREATE_VIEW,      // CREATE VIEW
  SCF_DROP_VIEW,        // DROP VIEW
  SCF_CREATE_DATABASE,  // CREATE DATABASE
  SCF_DROP_DATABASE,    // DROP DATABASE
  SCF_CREATE_SCHEMA,    // CREATE SCHEMA
  SCF_DROP_SCHEMA,      // DROP SCHEMA
  
  // 数据操作语言(DML)
  SCF_SELECT,           // SELECT
  SCF_INSERT,           // INSERT
  SCF_UPDATE,           // UPDATE
  SCF_DELETE,           // DELETE
  SCF_LOAD_DATA,        // LOAD DATA
  SCF_MERGE,            // MERGE
  SCF_CALL,             // CALL
  
  // 数据控制语言(DCL)
  SCF_GRANT,            // GRANT
  SCF_REVOKE,           // REVOKE
  SCF_DENY,             // DENY
  SCF_GRANT_ROLE,       // GRANT ROLE
  SCF_REVOKE_ROLE,      // REVOKE ROLE
  
  // 事务控制语言(TCL)
  SCF_BEGIN,            // BEGIN
  SCF_COMMIT,           // COMMIT
  SCF_ROLLBACK,         // ROLLBACK
  SCF_SAVEPOINT,        // SAVEPOINT
  SCF_RELEASE_SAVEPOINT,// RELEASE SAVEPOINT
  SCF_SET_TRANSACTION,  // SET TRANSACTION
  
  // 系统控制
  SCF_SET,              // SET
  SCF_SHOW,             // SHOW
  SCF_DESC,             // DESC/DESCRIBE
  SCF_CALC,             // CALC
  SCF_EXPLAIN,          // EXPLAIN
  SCF_ANALYZE,          // ANALYZE
  SCF_OPTIMIZE,         // OPTIMIZE
  SCF_CHECK,            // CHECK
  SCF_REPAIR,           // REPAIR
  
  // 用户管理
  SCF_CREATE_USER,      // CREATE USER
  SCF_DROP_USER,        // DROP USER
  SCF_ALTER_USER,       // ALTER USER
  SCF_RENAME_USER,      // RENAME USER
  SCF_SET_PASSWORD,     // SET PASSWORD
  
  // 备份恢复
  SCF_BACKUP,           // BACKUP
  SCF_RESTORE,          // RESTORE
  SCF_DUMP,             // DUMP
  SCF_LOAD,             // LOAD
  
  // 其他
  SCF_HELP,             // HELP
  SCF_EXIT,             // EXIT
  SCF_QUIT,             // QUIT
  SCF_SOURCE,           // SOURCE
  SCF_USE,              // USE
  SCF_CHANGE_DB,        // CHANGE DATABASE
  SCF_UNKNOWN           // 未知命令
};

// 关系属性引用节点
struct RelAttrSqlNode {
  std::string relation_name;  // 关系名（表名，可选）
  std::string attribute_name; // 属性名
  std::string alias;          // 别名（可选）
  
  RelAttrSqlNode() = default;
};


// 数据类型
enum class DataType {
  UNKNOWN,    // UNKNOWN
  CHARS,      // CHAR/VARCHAR
  DATES,      // DATE
  FLOATS,     // FLOAT/DOUBLE
  INTS,       // INT
  TEXTS,      // TEXT
  BOOLEANS,   // BOOLEAN
  NULLS,       // NULL
  MAX_TYPE
};

// 属性约束
enum class AttrConstraint {
  NONE,           // 无约束
  NOT_NULL,       // NOT NULL
  PRIMARY_KEY,    // PRIMARY KEY
  UNIQUE,         // UNIQUE
  DEFAULT,        // DEFAULT
  CHECK           // CHECK
};

// 属性节点
struct AttrSqlNode {
  std::string name;           // 属性名
  DataType type;              // 数据类型
  size_t length;              // 长度（用于CHAR/VARCHAR）
  bool nullable;              // 是否可为空
  Value default_value;        // 默认值
  std::vector<AttrConstraint> constraints;  // 约束列表
  
  AttrSqlNode() : type(DataType::CHARS), length(0), nullable(true) {}
};

// 操作数类型
enum class OperandType {
  FIELD,        // 字段
  VALUE,        // 值
  NULL_VALUE    // NULL值
};


// 操作数节点
struct OperandSqlNode {
  OperandType type;           // 操作数类型
  RelAttrSqlNode rel_attr;    // 关系属性引用（如果是字段）
  Value value;                // 值（如果是值）
  
  OperandSqlNode() : type(OperandType::VALUE) {}
};

// 比较运算符类型
enum class CompOp {
  EQUAL_TO,     // =
  NOT_EQUAL,    // != 或 <>
  LESS_THAN,    // <
  LESS_EQUAL,   // <=
  GREAT_THAN,   // >
  GREAT_EQUAL,  // >=
  IS_NULL,      // IS NULL
  IS_NOT_NULL,  // IS NOT NULL
  IN,           // IN
  NOT_IN,       // NOT IN
  LIKE,         // LIKE
  NOT_LIKE,     // NOT LIKE
  EXISTS,       // EXISTS
  NOT_EXISTS,   // NOT EXISTS
  BETWEEN,      // BETWEEN
  NOT_BETWEEN   // NOT BETWEEN
};

// 比较条件节点
struct CompConditionSqlNode {
  OperandSqlNode left;        // 左操作数
  CompOp op;                  // 比较运算符
  OperandSqlNode right;       // 右操作数
  
  CompConditionSqlNode() = default;
};

// 逻辑运算符类型
enum class LogicOp {
  AND,    // 与
  OR      // 或
};

// 条件节点
struct ConditionSqlNode {
  LogicOp logic_op;           // 逻辑运算符
  CompConditionSqlNode comp_cond;  // 比较条件
  ConditionSqlNode() : logic_op(LogicOp::AND) {}
};

// 关系节点
struct RelationSqlNode {
  std::string relation_name;  // 关系名
  std::string alias;          // 别名（可选）
  
  RelationSqlNode() = default;
};

// 连接类型
enum class JoinType {
  INNER,    // INNER JOIN
  LEFT,     // LEFT JOIN
  RIGHT,    // RIGHT JOIN
  FULL      // FULL JOIN
};

// 连接节点
struct JoinSqlNode {
  JoinType join_type;         // 连接类型
  RelationSqlNode left_table;  // 左表
  RelationSqlNode right_table; // 右表
  std::vector<ConditionSqlNode> conditions;  // 连接条件列表
  
  JoinSqlNode() : join_type(JoinType::INNER) {}
};

// 排序方向
enum class OrderDirection {
  ASC,     // 升序
  DESC     // 降序
};

// 排序节点
struct OrderBySqlNode {
  RelAttrSqlNode attr;        // 排序属性
  OrderDirection direction;   // 排序方向
  
  OrderBySqlNode() : direction(OrderDirection::ASC) {}
};

// 聚合函数类型
enum class AggregationType {
  COUNT,   // COUNT
  SUM,     // SUM
  AVG,     // AVG
  MAX,     // MAX
  MIN,     // MIN
  NONE     // 非聚合
};

// 选择项节点
struct SelectItemSqlNode {
  RelAttrSqlNode attr;        // 属性
  AggregationType agg_type;   // 聚合函数类型
  std::string alias;          // 别名（可选）
  
  SelectItemSqlNode() : agg_type(AggregationType::NONE) {}
};

// SELECT语句节点
struct SelectSqlNode {
  std::vector<SelectItemSqlNode> select_items;  // 选择项列表
  std::vector<RelationSqlNode> relations;       // 关系列表
  std::vector<JoinSqlNode> joins;               // 连接列表
  std::unique_ptr<ConditionSqlNode> where_condition;  // WHERE条件
  std::vector<RelAttrSqlNode> group_by;         // GROUP BY属性列表
  std::unique_ptr<ConditionSqlNode> having_condition; // HAVING条件
  std::vector<OrderBySqlNode> order_by;         // ORDER BY列表
  bool distinct;                                // 是否去重
  
  SelectSqlNode() : distinct(false) {}
};

// 算术运算符类型
enum class ArithOp {
  PLUS,     // +
  MINUS,    // -
  MULTIPLY, // *
  DIVIDE,   // /
  MOD       // %
};

// 表达式节点
struct ExprSqlNode {
  ArithOp op;                 // 算术运算符
  OperandSqlNode left;        // 左操作数
  OperandSqlNode right;       // 右操作数
  std::unique_ptr<ExprSqlNode> expr;  // 子表达式
  
  ExprSqlNode() : op(ArithOp::PLUS) {}
};

// CALC语句节点
struct CalcSqlNode {
  std::unique_ptr<int> expr;  // 计算表达式
  
  CalcSqlNode() = default;
};

// INSERT语句节点
struct InsertSqlNode {
  std::string relation_name;  // 目标表名
  std::vector<std::string> field_names;  // 要插入的字段名列表（可选）
  std::vector<std::vector<Value>> values;  // 要插入的值列表
  std::unique_ptr<SelectSqlNode> select;  // SELECT语句（可选）
  
  InsertSqlNode() = default;
};

// DELETE语句节点
struct DeleteSqlNode {
  std::string relation_name;  // 目标表名
  std::unique_ptr<ConditionSqlNode> condition;  // WHERE条件（可选）
  
  DeleteSqlNode() = default;
};

// UPDATE语句节点
struct UpdateSqlNode {
  std::string relation_name;  // 目标表名
  std::vector<std::string> field_names;  // 要更新的字段名列表
  std::vector<Value> values;  // 要更新的值列表
  std::unique_ptr<ConditionSqlNode> condition;  // WHERE条件（可选）
  
  UpdateSqlNode() = default;
};

// CREATE TABLE语句节点
struct CreateTableSqlNode {
  std::string relation_name;  // 表名
  std::vector<AttrSqlNode> field_defs;  // 字段定义列表
  std::vector<std::string> primary_keys;  // 主键字段列表
  std::vector<std::vector<std::string>> unique_keys;  // 唯一键字段列表
  std::vector<std::vector<std::string>> foreign_keys;  // 外键字段列表
  std::vector<std::string> foreign_ref_tables;  // 外键引用表列表
  std::vector<std::vector<std::string>> foreign_ref_fields;  // 外键引用字段列表
  
  CreateTableSqlNode() = default;
};

// DROP TABLE语句节点
struct DropTableSqlNode {
  std::string relation_name;  // 表名
  bool cascade;              // 是否使用CASCADE选项
  
  DropTableSqlNode() : cascade(false) {}
};

// 索引类型
enum class IndexType {
  NORMAL,     // 普通索引
  UNIQUE,     // 唯一索引
  PRIMARY     // 主键索引
};

// CREATE INDEX语句节点
struct CreateIndexSqlNode {
  std::string index_name;    // 索引名
  std::string relation_name; // 表名
  std::vector<std::string> field_names;  // 索引字段列表
  IndexType index_type;      // 索引类型
  bool unique;              // 是否为唯一索引
  
  CreateIndexSqlNode() : index_type(IndexType::NORMAL), unique(false) {}
};

// DROP INDEX语句节点
struct DropIndexSqlNode {
  std::string index_name;    // 索引名
  std::string relation_name; // 表名
  
  DropIndexSqlNode() = default;
};

// DESC TABLE语句节点
struct DescTableSqlNode {
  std::string relation_name;  // 表名
  
  DescTableSqlNode() = default;
};

// 字段分隔符类型
enum class FieldTerminator {
  COMMA,     // 逗号分隔
  TAB,       // 制表符分隔
  SPACE,     // 空格分隔
  CUSTOM     // 自定义分隔符
};

// LOAD DATA语句节点
struct LoadDataSqlNode {
  std::string relation_name;  // 目标表名
  std::string file_path;     // 数据文件路径
  std::vector<std::string> field_names;  // 字段名列表（可选）
  FieldTerminator terminator;  // 字段分隔符类型
  std::string custom_terminator;  // 自定义分隔符
  bool ignore_header;        // 是否忽略文件头
  size_t skip_lines;        // 跳过的行数
  
  LoadDataSqlNode() 
    : terminator(FieldTerminator::COMMA)
    , ignore_header(false)
    , skip_lines(0) {}
};

// 变量作用域
enum class VariableScope {
  SESSION,    // 会话变量
  GLOBAL,     // 全局变量
  LOCAL       // 本地变量
};

// SET语句节点
struct SetVariableSqlNode {
  std::string variable_name;  // 变量名
  Value value;                // 变量值
  VariableScope scope;        // 变量作用域
  
  SetVariableSqlNode() : scope(VariableScope::SESSION) {}
};

// 错误SQL节点
struct ErrorSqlNode {
  std::string error_msg;  // 错误信息
  
  ErrorSqlNode() = default;
};

class ParsedSqlNode;
// EXPLAIN语句节点
struct ExplainSqlNode {
  std::unique_ptr<ParsedSqlNode> sql_node;  // 要解释的SQL语句
  
  ExplainSqlNode() = default;
};

struct SourceSqlNode {
  std::string filename;

  SourceSqlNode() = default;
};

// SQL解析节点
class ParsedSqlNode {
public:
  SqlCommandFlag flag;           // SQL命令类型
  ErrorSqlNode error;           // 错误节点
  CalcSqlNode calc;             // 计算节点
  SelectSqlNode selection;      // 查询节点
  InsertSqlNode insertion;      // 插入节点
  DeleteSqlNode deletion;       // 删除节点
  UpdateSqlNode update;         // 更新节点
  CreateTableSqlNode create_table;  // 创建表节点
  DropTableSqlNode drop_table;      // 删除表节点
  CreateIndexSqlNode create_index;  // 创建索引节点
  DropIndexSqlNode drop_index;      // 删除索引节点
  DescTableSqlNode desc_table;      // 描述表节点
  LoadDataSqlNode load_data;        // 加载数据节点
  ExplainSqlNode explain;           // 解释节点
  SetVariableSqlNode set_variable;  // 设置变量节点
  SourceSqlNode source;             // 源文件节点
public:
  ParsedSqlNode() : flag(SqlCommandFlag::SCF_UNKNOWN) {}
  explicit ParsedSqlNode(SqlCommandFlag flag) : flag(flag) {}
};

// SQL解析结果
class ParsedSqlResult {
public:
  std::vector<std::unique_ptr<ParsedSqlNode>> sql_nodes;  // SQL语句节点列表
  bool success;                                            // 解析是否成功
  std::string error_msg;                                   // 错误信息（如果解析失败）

public:
  ParsedSqlResult() : success(true) {}
  
  // 添加SQL节点
  void add_sql_node(std::unique_ptr<ParsedSqlNode> node) {
    sql_nodes.emplace_back(std::move(node));
  }
  
  // 设置解析失败
  void set_error(const std::string& msg) {
    success = false;
    error_msg = msg;
  }
  
  // 清空结果
  void clear() {
    sql_nodes.clear();
    success = true;
    error_msg.clear();
  }
  
  // 获取SQL节点数量
  size_t size() const {
    return sql_nodes.size();
  }
  
  // 判断是否为空
  bool empty() const {
    return sql_nodes.empty();
  }
};

} // namespace sql