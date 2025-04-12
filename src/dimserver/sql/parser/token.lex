%{
#include <string>

#include "sql/parser/sql_node.h"
#include "common/log/log.h"
#include "sql/parser/syntax_tree.hpp"


#define TOKEN(token) LOG_INFO("token: %s", #token); \
  return token
%}

%option noyywrap
%option nodefault
%option reentrant
%option bison-bridge
%option yylineno
%option bison-locations
%option case-insensitive


SPACE                     [ \t\b\f]
DIGIT                     [0-9]

ID                        [a-zA-Z_]+[a-zA-Z0-9_]*
DOT                       \.
QUOTE                     [\'\"]
%x  STR
%x  COMMENT

/* 日期格式定义 */
DATE_SEP                     [-/]
TIME_SEP                     [:]
YEAR                        {DIGIT}{4}
MONTH                       {DIGIT}{1,2}
DAY                         {DIGIT}{1,2}
HOUR                        {DIGIT}{1,2}
MINUTE                      {DIGIT}{1,2}
SECOND                      {DIGIT}{1,2}

/* 基础日期模式 */
DATE_BASE                   {YEAR}{DATE_SEP}{MONTH}{DATE_SEP}{DAY}
TIME_BASE                   {HOUR}{TIME_SEP}{MINUTE}
TIME_SEC_BASE               {HOUR}{TIME_SEP}{MINUTE}{TIME_SEP}{SECOND}

/* 完整日期模式 */
DATE_ISO                    {QUOTE}{DATE_BASE}{QUOTE}
DATE_TIME                   {QUOTE}{DATE_BASE}{SPACE}{TIME_BASE}{QUOTE}
DATE_TIME_SEC               {QUOTE}{DATE_BASE}{SPACE}{TIME_SEC_BASE}{QUOTE}

%%

{SPACE}                       ;
\n                            ;
[\-]?{DIGIT}+                 TOKEN(NUMBER);
[\-]?{DIGIT}+{DOT}{DIGIT}+    TOKEN(FLOAT);

";"                           TOKEN(SEMICOLON);
{DOT}                         TOKEN(DOT);

EXIT                          TOKEN(EXIT);
HELP                          TOKEN(HELP);
QUIT                          TOKEN(QUIT);
SOURCE                        TOKEN(SOURCE);
DESC                          TOKEN(DESC);
SHOW                          TOKEN(SHOW);

"("                           TOKEN(LPAREN);
")"                           TOKEN(RPAREN);
","                           TOKEN(COMMA);

"="                           TOKEN(EQ);
"<="                          TOKEN(LE);
">="                          TOKEN(GE);
"<"                           TOKEN(LT);
">"                           TOKEN(GT);
"<>"                          TOKEN(NE);
"!="                          TOKEN(NE);

"+" |
"-" |
"*" |
"/" |
"%"                           { return yytext[0]; }


"--".*                        ;
"/*"                          BEGIN(COMMENT);
<COMMENT>"*/"                 BEGIN(INITIAL);
<COMMENT>.|\n                 ;

CREATE                        TOKEN(CREATE);
DROP                          TOKEN(DROP);
ALTER                         TOKEN(ALTER);
TRUNCATE                      TOKEN(TRUNCATE);
TABLE                         TOKEN(TABLE);
DATABASE                      TOKEN(DATABASE);
SCHEMA                        TOKEN(SCHEMA);
INDEX                         TOKEN(INDEX);
VIEW                          TOKEN(VIEW);
PRIMARY                       TOKEN(PRIMARY);
KEY                           TOKEN(KEY);
FOREIGN                       TOKEN(FOREIGN);
UNIQUE                        TOKEN(UNIQUE);
CONSTRAINT                    TOKEN(CONSTRAINT);
REFERENCES                    TOKEN(REFERENCES);
CHECK                         TOKEN(CHECK);
DEFAULT                       TOKEN(DEFAULT);

INSERT                        TOKEN(INSERT);
INTO                          TOKEN(INTO);
VALUES                        TOKEN(VALUES);
UPDATE                        TOKEN(UPDATE);
SET                           TOKEN(SET);
DELETE                        TOKEN(DELETE);
FROM                          TOKEN(FROM);
MERGE                         TOKEN(MERGE);
LOAD                          TOKEN(LOAD);
DATA                          TOKEN(DATA);

SELECT                        TOKEN(SELECT);
DISTINCT                      TOKEN(DISTINCT);
WHERE                         TOKEN(WHERE);
GROUP                         TOKEN(GROUP);
BY                            TOKEN(BY);
HAVING                        TOKEN(HAVING);
ORDER                         TOKEN(ORDER);
ASC                           TOKEN(ASC);
DESC                          TOKEN(DESC);
LIMIT                         TOKEN(LIMIT);
OFFSET                        TOKEN(OFFSET);
JOIN                          TOKEN(JOIN);
INNER                         TOKEN(INNER);
LEFT                          TOKEN(LEFT);
RIGHT                         TOKEN(RIGHT);
FULL                          TOKEN(FULL);
OUTER                         TOKEN(OUTER);
ON                            TOKEN(ON);
AS                            TOKEN(AS);
COUNT                         TOKEN(COUNT);
SUM                           TOKEN(SUM);
AVG                           TOKEN(AVG);
MAX                           TOKEN(MAX);
MIN                           TOKEN(MIN);
IN                            TOKEN(IN);
NOT                           TOKEN(NOT);
EXISTS                        TOKEN(EXISTS);
BETWEEN                       TOKEN(BETWEEN);
LIKE                          TOKEN(LIKE);
IS                            TOKEN(IS);
NULL                          TOKEN(NULL_TOKEN);
AND                           TOKEN(AND);
OR                            TOKEN(OR);

BEGIN                         TOKEN(BEGIN);
START                         TOKEN(START);
TRANSACTION                   TOKEN(TRANSACTION);
COMMIT                        TOKEN(COMMIT);
ROLLBACK                      TOKEN(ROLLBACK);
SAVEPOINT                     TOKEN(SAVEPOINT);
RELEASE                       TOKEN(RELEASE);

INT                           TOKEN(INT_T);
INTEGER                       TOKEN(INT_T);
CHAR                          TOKEN(CHAR_T);
VARCHAR                       TOKEN(VARCHAR_T);
TEXT                          TOKEN(TEXT_T);
DATE                          TOKEN(DATE_T);
TIME                          TOKEN(TIME_T);
DATETIME                      TOKEN(DATETIME_T);
FLOAT                         TOKEN(FLOAT_T);
DOUBLE                        TOKEN(DOUBLE_T);
DECIMAL                       TOKEN(DECIMAL_T);
BOOLEAN                       TOKEN(BOOL_T);

{DATE_ISO}                    TOKEN(DATE);
{DATE_TIME}                   TOKEN(DATETIME);
{DATE_TIME_SEC}               TOKEN(DATETIME);

{ID}                          TOKEN(ID);

\"[^"]*\"                     TOKEN(SSS);
'[^']*\'                      TOKEN(SSS);

\"                            BEGIN(STR);
<STR>\"                       yylval.string = strdup(yytext);

.                             LOG_DEBUG("unknown character [%c]", yytext[0]); return yytext[0];
%%