#include "common/rc.h"

// 返回错误码对应的字符串描述
const char* strrc(RC rc) {
  switch (rc) {
#define RC_DEF(name, value) case RC::name: return #name;
  RC_SET
#undef RC_DEF
    default:
      return "UNKNOWN";
  }
}

// 判断是否成功
bool IS_SUCC(RC rc) {
  return rc == RC::SUCCESS;
}

// 判断是否失败
bool IS_FAIL(RC rc) {
  return rc != RC::SUCCESS;
}
