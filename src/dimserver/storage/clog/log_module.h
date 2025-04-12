#pragma once

#include <stdint.h>

/**
 * @brief 日志模块类
 * 用于标识和管理不同类型的日志模块
 */
class LogModule {
public:
  /**
   * @brief 日志模块ID枚举
   * 定义了系统中所有可能的日志模块类型
   */
  enum class Id {
    BUFFER_POOL,   // 缓存池模块
    BPLUS_TREE,    // B+树模块
    RECORD_MANAGER,// 记录管理模块
    TRANSACTION    // 事务模块
  };

public:
  /**
   * @brief 构造函数
   * @param id 日志模块ID
   */
  explicit LogModule(Id id) : m_id(id) {}

  /**
   * @brief 构造函数
   * @param id 日志模块ID的整数值
   */
  explicit LogModule(int32_t id) : m_id(static_cast<Id>(id)) {}

  /**
   * @brief 获取模块名称
   * @return 返回模块名称的字符串表示
   */
  [[nodiscard]] const char* name() const {
    switch(m_id) {
#define xx(name) case Id::name: return #name
      xx(BUFFER_POOL);
      xx(BPLUS_TREE);
      xx(RECORD_MANAGER);
      xx(TRANSACTION);
#undef xx
      default:
        return "UNKNOWN";
    }
  }

  /**
   * @brief 获取模块ID
   * @return 返回模块ID
   */
  Id id() const { return m_id;}

  /**
   * @brief 获取模块索引
   * @return 返回模块ID的整数值
   */
  int32_t index() const { return static_cast<int32_t>(m_id);}

private:
  Id m_id;  // 模块ID
};