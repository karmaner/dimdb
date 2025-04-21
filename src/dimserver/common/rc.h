#pragma once

// 定义错误码宏
#define RC_SET                              \
    RC_DEF(SUCCESS, 0)                      \
    /* General errors (-1 ~ -99) */         \
    RC_DEF(INTERNAL, -1)                    \
    RC_DEF(INVALID_ARGUMENT, -2)            \
    RC_DEF(OUT_OF_MEMORY, -3)               \
    RC_DEF(UNIMPLEMENTED, -4)               \
    RC_DEF(TIMEOUT, -5)                     \
    /* File system errors (-100 ~ -199) */  \
    RC_DEF(FILE_NOT_FOUND, -100)            \
    RC_DEF(FILE_EXISTS, -101)               \
    RC_DEF(FILE_PERMISSION, -102)           \
    RC_DEF(FILE_CORRUPTED, -103)            \
    RC_DEF(FILE_OPEND, -104)                \
    RC_DEF(FILE_NOT_OPEN, -105)             \
    RC_DEF(FILE_FULL, -110)                 \
    RC_DEF(FILE_NAME_INVALID, -120)         \
    RC_DEF(FILE_CREATE_ERR, -130)           \
    /* Buffer pool errors (-200 ~ -299) */  \
    RC_DEF(BUFFER_POOL_FULL, -200)          \
    RC_DEF(PAGE_NOT_FOUND, -201)            \
    RC_DEF(PAGE_UNPIN_ERROR, -202)          \
    /* SQL errors (-300 ~ -399) */          \
    RC_DEF(SQL_SYNTAX, -300)                \
    RC_DEF(SQL_SEMANTIC, -301)              \
    RC_DEF(SQL_EXECUTION, -302)             \
    RC_DEF(TABLE_NOT_EXIST, -303)           \
    RC_DEF(TABLE_EXIST, -304)               \
    RC_DEF(FIELD_NOT_EXIST, -305)           \
    RC_DEF(FIELD_TYPE_MISMATCH, -306)       \
    RC_DEF(INDEX_EXIST, -307)               \
    RC_DEF(INDEX_NOT_FOUND, -308)           \
    /* Transaction errors (-400 ~ -499) */  \
    RC_DEF(TXN_ABORTED, -400)               \
    RC_DEF(TXN_CONFLICT, -401)              \
    RC_DEF(LOCK_TIMEOUT, -402)              \
    /* Type errors (-500 ~ -599) */         \
    RC_DEF(TYPE_MISMATCH, -500)             \
    RC_DEF(TYPE_NOT_SUPPORTED, -501)        \
    RC_DEF(VALUE_OUT_OF_RANGE, -502)        \
    /* Expression errors (-600 ~ -699) */   \
    RC_DEF(EXPR_INVALID, -600)              \
    RC_DEF(EXPR_TYPE_MISMATCH, -601)        \
    RC_DEF(EXPR_EVALUATION, -602)           \
    /* Other errors (-700 ~ -799) */        \
    RC_DEF(NOT_AUTHORIZED, -700)            \
    RC_DEF(CONFIG_ERROR, -701)              \
    RC_DEF(IOERR_READ, -710)                \
    RC_DEF(IOERR_WRITE, -711)               \
    RC_DEF(IOERR_SEEK, -712)                \
    RC_DEF(MESSAGE_INVAID, -750)            \
    RC_DEF(NO_MEM_POOL, -760)               \
    RC_DEF(BUFFERPOOL_INVALID_PAGE_NUM, -800)\
    RC_DEF(BUFFERPOOL_OPENED, -810)         \
    RC_DEF(SCHEMA_DB_EXIST, -820)

// 定义枚举类
enum class RC {
#define RC_DEF(name, value) name = value,
  RC_SET
#undef RC_DEF
};

extern const char* strrc(RC rc);

extern bool IS_SUCC(RC rc);
extern bool IS_FAIL(RC rc);