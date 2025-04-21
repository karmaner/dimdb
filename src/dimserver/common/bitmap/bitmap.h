#pragma once

#include <cstdint>
#include <string>

class Bitmap {
public:
  explicit Bitmap();
  explicit Bitmap(char* bitmap, int size);

  void init(char* bitmap, int size);

  void set(size_t index);
  void clear(size_t index);
  bool get(size_t index) const;

  /**
   * @param start 从哪个位开始查找，start是包含在内的
   */
  int next_zero_bit(int start) const;
  int next_one_bit(int start) const;

  int size() const { return size_; }
  int bytes() const { return (size_ + 7) / 8; }
  std::string to_string() const;
  
  // 获取位图数据的指针
  const char* data() const { return bitmap_; }
  char* data() { return bitmap_; }

private:
  // 在字节中查找第一个0位
  static int find_first_zero(char byte, int start);
  // 在字节中查找第一个1位
  static int find_first_one(char byte, int start);

  int size_ = 0;
  char* bitmap_{nullptr};
};