#pragma once

#include <sys/types.h>

/**
 * @brief 可靠地写入指定大小的数据
 * 
 * @param fd 文件描述符
 * @param buf 要写入的数据缓冲区
 * @param size 要写入的字节数
 * @return int 成功返回0，失败返回errno
 * 
 * @note 该函数会一直尝试写入，直到所有数据都被写入或发生错误
 *       处理EINTR（被信号中断）和EAGAIN（非阻塞IO暂时无法写入）的情况
 *       适用于网络编程和文件操作中的可靠写入
 */
int writen(int fd, const void* buf, size_t size);

/**
 * @brief 可靠地读取指定大小的数据
 * 
 * @param fd 文件描述符
 * @param buf 读取数据的缓冲区
 * @param size 要读取的字节数
 * @return int 成功返回0，失败返回errno
 * 
 * @note 该函数会一直尝试读取，直到所有数据都被读取、遇到EOF或发生错误
 *       处理EINTR（被信号中断）和EAGAIN（非阻塞IO暂时无法读取）的情况
 *       适用于网络编程和文件操作中的可靠读取
 */
int readn(int fd, void* buf, size_t size);