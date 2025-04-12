%{
#include <stdio.h>
#include <string.h>

#include "common/log/log.h"
#include "sql/parser/sql_node.h"
#include "sql/parser/sql_parser.h"
#include "sql/parser/token.lex.h"

// 声明词法分析器函数
int yylex(YYSTYPE* yylval, YYLTYPE* yylloc, void* scanner);
void yyerror(YYLTYPE* loc, sql::SqlParser& parser, sql::ParsedSqlResult& result, const char* msg);

%}

// 生成头文件
%defines "sql/parser/syntax_tree.hpp"
// 生成实现文件
%output "sql/parser/syntax_tree.cpp"

// 使用C++
%language "C++"
// 使用C++命名空间
%define api.namespace {sql}
// 使用C++类型
%define api.value.type variant
// 启用位置跟踪
%locations
// 启用详细错误信息
%define parse.error verbose

// 声明类型
%code requires {
    #include <string>
    #include <vector>
    #include <memory>
    #include "sql/parser/sql_node.h"
    
    namespace sql {
        class SqlParser;
    }
}

// 定义解析器参数
%parse-param { sql::SqlParser& parser }
%parse-param { sql::ParsedSqlResult& result }

// 定义词法分析器参数
%lex-param { void* scanner }

// 定义类型
%token <std::string> ID STRING NUMBER
%token <int> STAR
%token NULL_TOKEN

%type <std::string> table_name column_name
%type <std::vector<std::string>*> id_list
%type <std::vector<std::vector<Value>>*> value_lists
%type <std::vector<Value>*> value_list
%type <Value*> value
%type <std::pair<std::vector<std::string>, std::vector<Value>>*> update_assignments update_assignment
%type <std::vector<RelAttrSqlNode>*> group_by_clause
%type <std::vector<OrderBySqlNode>*> order_by_clause
%type <AggregationType> aggregate_function

%token  SEMICOLON
        DOT

/* 系统命令 */
%token  EXIT
        HELP
        QUIT
        SOURCE
        SHOW

/* 括号 */
%token  LPAREN
        RPAREN
        COMMA

/* 比较运算符 */
%token  EQ
        LE
        GE
        LT
        GT
        NE

/* DDL */
%token  CREATE
        DROP
        ALTER
        TRUNCATE
        TABLE
        DATABASE
        SCHEMA
        INDEX
        VIEW
        PRIMARY
        KEY
        FOREIGN
        UNIQUE
        CONSTRAINT
        REFERENCES
        CHECK
        DEFAULT

/* DML */
%token  INSERT
        INTO
        VALUES
        UPDATE
        SET
        DELETE
        FROM
        MERGE
        LOAD
        DATA

/* DQL */
%token  SELECT
        DISTINCT
        WHERE
        GROUP
        BY
        HAVING
        ORDER
        ASC
        DESC
        LIMIT
        OFFSET
        JOIN
        INNER
        LEFT
        RIGHT
        FULL
        OUTER
        ON
        AS
        COUNT
        SUM
        AVG
        MAX
        MIN
        IN
        NOT
        EXISTS
        BETWEEN
        LIKE
        IS
        NULL
        AND
        OR

/* TCL */
%token  BEGIN
        START
        TRANSACTION
        COMMIT
        ROLLBACK
        SAVEPOINT
        RELEASE

/* 数据类型 */
%token  INT_T
        INTEGER_T
        CHAR_T
        VARCHAR_T
        TEXT_T
        DATE_T
        TIME_T
        DATETIME_T
        FLOAT_T
        DOUBLE_T
        DECIMAL_T
        BOOL_T

%union {
  ParsedSqlNode*                         sql_node;
  ConditionSqlNode*                      condition;
  Value*                                 value;
  RelAttrSqlNode*                        rel_attr;
  AttrSqlNode*                           attribute;
  std::vector<Value>*                    value_list;      // 单条记录的值列表
  std::vector<std::vector<Value>>*       value_list_list; // 多条记录的值列表
  std::vector<RelAttrSqlNode*>*          attr_list;       // 修改为指针类型
  enum CompOp                            comp_op;
  char*                                  string;
  std::vector<std::string>*              relation_list;
  std::vector<JoinSqlNode*>              join_list;
  SelectItemSqlNode*                     select_item;
  std::vector<SelectItemSqlNode>*        select_item_list;
  RelationSqlNode*                       table_ref;
  std::vector<RelationSqlNode>*          table_ref_list;
  JoinType                               join_type;
  std::vector<OrderBySqlNode>*           order_list;
  OrderBySqlNode*                        order_item;
  AggregationType                        agg_type;
}

/* 系统命令 */
%type <sql_node> exit_stmt
%type <sql_node> help_stmt
%type <sql_node> quit_stmt
%type <sql_node> source_stmt
%type <sql_node> desc_stmt
%type <sql_node> show_stmt

/* DDL - 数据定义语言 */
%type <sql_node> create_stmt
%type <sql_node> drop_table_stmt
%type <sql_node> alter_table_stmt
%type <sql_node> create_index_stmt
%type <sql_node> drop_index_stmt
%type <sql_node> create_view_stmt
%type <sql_node> drop_view_stmt
%type <sql_node> create_database_stmt
%type <sql_node> drop_database_stmt
%type <sql_node> create_schema_stmt
%type <sql_node> drop_schema_stmt
%type <sql_node> truncate_stmt

/* DML - 数据操作语言 */
%type <sql_node> insert_stmt
%type <sql_node> update_stmt
%type <sql_node> delete_stmt
%type <sql_node> merge_stmt
%type <sql_node> load_stmt

/* DQL - 数据查询语言 */
%type <sql_node> select_stmt
%type <sql_node> explain_stmt

/* TCL - 事务控制语言 */
%type <sql_node> begin_stmt
%type <sql_node> start_stmt
%type <sql_node> transaction_stmt
%type <sql_node> commit_stmt
%type <sql_node> rollback_stmt
%type <sql_node> savepoint_stmt
%type <sql_node> release_stmt

/* 值表达式相关 */
%type <value_list> value_list_def
%type <value_list_list> value_list_list_def
%type <value> value_expr
%type <value> literal
%type <value> column_value
%type <value> function_call

/* 条件表达式相关 */
%type <condition> where_clause
%type <condition> having_clause
%type <condition> condition_expr
%type <condition> comp_condition
%type <condition> in_condition
%type <condition> between_condition
%type <condition> like_condition
%type <condition> is_condition
%type <condition> exists_condition

/* 属性定义相关 */
%type <rel_attr> rel_attr_def
%type <attribute> attribute_def
%type <attr_list> attribute_def_list

/* 操作符相关 */
%type <comp_op> comp_op_def

/* 类型声明 */
%type <select_item> select_item
%type <select_item_list> select_items
%type <table_ref> table_reference
%type <table_ref_list> table_references
%type <join_type> join_type
%type <order_list> order_by_list
%type <order_item> order_by_item
%type <attr_list> rel_attr_list

%%

/* 开始规则 */
stmt_list:
    stmt SEMICOLON
    | stmt_list stmt SEMICOLON
    ;

stmt:
    exit_stmt
    | help_stmt
    | quit_stmt
    | source_stmt
    | desc_stmt
    | show_stmt
    | create_stmt
    | drop_table_stmt
    | alter_table_stmt
    | create_index_stmt
    | drop_index_stmt
    | create_view_stmt
    | drop_view_stmt
    | create_database_stmt
    | drop_database_stmt
    | create_schema_stmt
    | drop_schema_stmt
    | truncate_stmt
    | insert_stmt
    | update_stmt
    | delete_stmt
    | merge_stmt
    | load_stmt
    | select_stmt
    | explain_stmt
    | begin_stmt
    | start_stmt
    | transaction_stmt
    | commit_stmt
    | rollback_stmt
    | savepoint_stmt
    | release_stmt
    ;

/* 系统命令 */
exit_stmt:
  EXIT
  {
    $$ = new ParsedSqlNode(SCF_EXIT);
  }
  ;

help_stmt:
  HELP
  {
    $$ = new ParsedSqlNode(SCF_HELP);
  }
  ;

quit_stmt:
  QUIT
  {
    $$ = new ParsedSqlNode(SCF_QUIT);
  }
  ;

source_stmt:
  SOURCE STRING
  {
    $$ = new ParsedSqlNode(SCF_SOURCE);
    $$->source.filename = strdup($2);
  }
  ;

select_stmt:
    SELECT select_items FROM table_references where_clause group_by_clause having_clause order_by_clause
    {
        $$ = new ParsedSqlNode(SCF_SELECT);
        $$->selection.select_items = *$2;
        $$->selection.relations = *$4;
        if ($5) {
            $$->selection.where_condition.reset($5);
        }
        if ($6) {
            $$->selection.group_by = *$6;
        }
        if ($7) {
            $$->selection.having_condition.reset($7);
        }
        if ($8) {
            $$->selection.order_by = *$8;
        }
    }
    | SELECT DISTINCT select_items FROM table_references where_clause group_by_clause having_clause order_by_clause
    {
        $$ = new ParsedSqlNode(SCF_SELECT);
        $$->selection.distinct = true;
        $$->selection.select_items = *$3;
        $$->selection.relations = *$5;
        if ($6) {
            $$->selection.where_condition.reset($6);
        }
        if ($7) {
            $$->selection.group_by = *$7;
        }
        if ($8) {
            $$->selection.having_condition.reset($8);
        }
        if ($9) {
            $$->selection.order_by = *$9;
        }
    }
    ;

select_items:
    select_item
    {
        $$ = new std::vector<SelectItemSqlNode>();
        $$->push_back(*$1);
    }
    | select_items COMMA select_item
    {
        $1->push_back(*$3);
        $$ = $1;
    }
    ;

select_item:
    STAR
    {
        $$ = new SelectItemSqlNode();
        $$->attr.attribute_name = "*";
    }
    | rel_attr_def
    {
        $$ = new SelectItemSqlNode();
        $$->attr = *$1;
    }
    | aggregate_function rel_attr_def
    {
        $$ = new SelectItemSqlNode();
        $$->attr = *$2;
        $$->agg_type = $1;
    }
    | rel_attr_def AS ID
    {
        $$ = new SelectItemSqlNode();
        $$->attr = *$1;
        $$->alias = $3;
    }
    ;

table_references:
    table_reference
    {
        $$ = new std::vector<RelationSqlNode>();
        $$->push_back(*$1);
    }
    | table_references COMMA table_reference
    {
        $1->push_back(*$3);
        $$ = $1;
    }
    | table_references join_type JOIN table_reference ON condition_expr
    {
        JoinSqlNode join;
        join.join_type = $2;
        join.left_table = $1->back();
        join.right_table = *$4;
        join.conditions.push_back(*$6);
        $1->pop_back();
        $$->joins.push_back(join);
        $$ = $1;
    }
    ;

join_type:
    INNER   { $$ = JoinType::INNER; }
    | LEFT  { $$ = JoinType::LEFT; }
    | RIGHT { $$ = JoinType::RIGHT; }
    | FULL  { $$ = JoinType::FULL; }
    ;

where_clause:
    /* empty */
    {
        $$ = nullptr;
    }
    | WHERE condition_expr
    {
        $$ = $2;
    }
    ;

group_by_clause:
    /* empty */
    {
        $$ = nullptr;
    }
    | GROUP BY rel_attr_list
    {
        $$ = $3;
    }
    ;

having_clause:
    /* empty */
    {
        $$ = nullptr;
    }
    | HAVING condition_expr
    {
        $$ = $2;
    }
    ;

order_by_clause:
    /* empty */
    {
        $$ = nullptr;
    }
    | ORDER BY order_by_list
    {
        $$ = $3;
    }
    ;

order_by_list:
    order_by_item
    {
        $$ = new std::vector<OrderBySqlNode>();
        $$->push_back(*$1);
    }
    | order_by_list COMMA order_by_item
    {
        $1->push_back(*$3);
        $$ = $1;
    }
    ;

order_by_item:
    rel_attr_def
    {
        $$ = new OrderBySqlNode();
        $$->attr = *$1;
        $$->direction = OrderDirection::ASC;
    }
    | rel_attr_def ASC
    {
        $$ = new OrderBySqlNode();
        $$->attr = *$1;
        $$->direction = OrderDirection::ASC;
    }
    | rel_attr_def DESC
    {
        $$ = new OrderBySqlNode();
        $$->attr = *$1;
        $$->direction = OrderDirection::DESC;
    }
    ;

table_reference:
    ID
    {
        $$ = new RelationSqlNode();
        $$->relation_name = $1;
    }
    | ID ID
    {
        $$ = new RelationSqlNode();
        $$->relation_name = $1;
        $$->alias = $2;
    }
    | ID AS ID
    {
        $$ = new RelationSqlNode();
        $$->relation_name = $1;
        $$->alias = $3;
    }
    ;

rel_attr_list:
    rel_attr_def
    {
        $$ = new std::vector<RelAttrSqlNode>();
        $$->push_back(*$1);
    }
    | rel_attr_list COMMA rel_attr_def
    {
        $1->push_back(*$3);
        $$ = $1;
    }
    ;

aggregate_function:
    COUNT   { $$ = AggregationType::COUNT; }
    | SUM   { $$ = AggregationType::SUM; }
    | AVG   { $$ = AggregationType::AVG; }
    | MAX   { $$ = AggregationType::MAX; }
    | MIN   { $$ = AggregationType::MIN; }
    ;

rel_attr_def:
    ID
    {
        $$ = new RelAttrSqlNode();
        $$->attribute_name = $1;
    }
    | ID DOT ID
    {
        $$ = new RelAttrSqlNode();
        $$->relation_name = $1;
        $$->attribute_name = $3;
    }
    ;

insert_stmt:
    INSERT INTO ID LPAREN id_list RPAREN VALUES value_lists
    {
        $$ = new ParsedSqlNode(SCF_INSERT);
        $$->insertion.relation_name = $3;
        $$->insertion.field_names = *$5;
        $$->insertion.values = *$8;
    }
    | INSERT INTO ID VALUES value_lists
    {
        $$ = new ParsedSqlNode(SCF_INSERT);
        $$->insertion.relation_name = $3;
        $$->insertion.values = *$5;
    }
    ;

id_list:
    ID
    {
        $$ = new std::vector<std::string>();
        $$->push_back($1);
    }
    | id_list COMMA ID
    {
        $1->push_back($3);
        $$ = $1;
    }
    ;

value_lists:
    LPAREN value_list RPAREN
    {
        $$ = new std::vector<std::vector<Value>>();
        $$->push_back(*$2);
    }
    | value_lists COMMA LPAREN value_list RPAREN
    {
        $1->push_back(*$4);
        $$ = $1;
    }
    ;

value_list:
    value
    {
        $$ = new std::vector<Value>();
        $$->push_back(*$1);
    }
    | value_list COMMA value
    {
        $1->push_back(*$3);
        $$ = $1;
    }
    ;

value:
    NUMBER
    {
        $$ = new Value();
        $$->type = DataType::INTS;
        $$->int_value = atoi($1);
    }
    | STRING
    {
        $$ = new Value();
        $$->type = DataType::CHARS;
        $$->str_value = $1;
    }
    | NULL_TOKEN
    {
        $$ = new Value();
        $$->type = DataType::NULLS;
    }
    ;

update_stmt:
    UPDATE ID SET update_assignments where_clause
    {
        $$ = new ParsedSqlNode(SCF_UPDATE);
        $$->update.relation_name = $2;
        $$->update.field_names = $4->first;
        $$->update.values = $4->second;
        if ($5) {
            $$->update.condition.reset($5);
        }
    }
    ;

update_assignments:
    update_assignment
    {
        $$ = $1;
    }
    | update_assignments COMMA update_assignment
    {
        $1->first.insert($1->first.end(), 
                        $3->first.begin(), 
                        $3->first.end());
        $1->second.insert($1->second.end(), 
                         $3->second.begin(), 
                         $3->second.end());
        $$ = $1;
    }
    ;

update_assignment:
    ID EQ value
    {
        $$ = new std::pair<std::vector<std::string>, std::vector<Value>>();
        $$->first.push_back($1);
        $$->second.push_back(*$3);
    }
    ;

delete_stmt:
    DELETE FROM ID where_clause
    {
        $$ = new ParsedSqlNode(SCF_DELETE);
        $$->deletion.relation_name = $3;
        if ($4) {
            $$->deletion.condition.reset($4);
        }
    }
    ;

%%

void yyerror(const char *s)
{
  fprintf(stderr, "Error: %s\n", s);
}

