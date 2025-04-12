#include <unistd.h>
#include <errno.h>

/**
 * @file io.cpp
 * @brief 实现可靠的IO操作函数
 */

/**
 * @brief writen函数实现
 * @details 通过循环写入确保数据完整性
 * 
 * 实现要点：
 * 1. 使用指针移动跟踪写入位置
 * 2. 处理部分写入的情况
 * 3. 特殊错误处理：
 *    - EINTR: 被信号中断，继续写入
 *    - EAGAIN: 非阻塞IO暂时无法写入，继续尝试
 *    - 其他错误: 返回错误码
 */
int writen(int fd, const void* buf, size_t size) {
  const char* ptr = static_cast<const char*>(buf);
  while (size > 0) {
    const ssize_t n = ::write(fd, ptr, size);
    if (n >= 0) {
      ptr += n;
      size -= n;
      continue;
    }
    if (errno != EAGAIN && errno != EINTR) {
      return errno;
    }
  }
  return 0;
}

/**
 * @brief readn函数实现
 * @details 通过循环读取确保数据完整性
 * 
 * 实现要点：
 * 1. 使用指针移动跟踪读取位置
 * 2. 处理部分读取的情况
 * 3. 特殊错误处理：
 *    - EINTR: 被信号中断，继续读取
 *    - EAGAIN: 非阻塞IO暂时无法读取，继续尝试
 *    - EOF: 文件结束，正常返回
 *    - 其他错误: 返回错误码
 */
int readn(int fd, void* buf, size_t size) {
  char* ptr = static_cast<char*>(buf);
  while (size > 0) {
    const ssize_t n = ::read(fd, ptr, size);
    if (n > 0) {
      ptr += n;
      size -= n;
      continue;
    } else if (n == 0) { // 文件结束
      return -1;
    } else if (errno != EAGAIN && errno != EINTR) {
      return errno;
    }
  }
  return 0;
}