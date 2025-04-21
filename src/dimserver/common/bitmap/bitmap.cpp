#include <algorithm>
#include <sstream>
#include <cstring>

#include "common/bitmap/bitmap.h"

int Bitmap::find_first_zero(char byte, int start) {
  for (int i = start; i < 8; ++i) {
    if ((byte & (1 << i)) == 0) {
      return i;
    }
  }
  return -1;
}

int Bitmap::find_first_one(char byte, int start) {
  for (int i = start; i < 8; i++) {
    if ((byte & (1 << i)) != 0) {
      return i;
    }
  }
  return -1;
}

Bitmap::Bitmap() : size_(0), bitmap_(nullptr) {}
Bitmap::Bitmap(char* bitmap, int size) : size_(size), bitmap_(bitmap) {}

void Bitmap::init(char* bitmap, int size) {
  size_ = size;
  bitmap_ = bitmap;
}

void Bitmap::set(size_t index) {
  char &bits = bitmap_[index / 8];
  bits |= (1 << (index % 8));
}

void Bitmap::clear(size_t index) {
  char &bits = bitmap_[index / 8];
  bits &= ~(1 << (index % 8));
}

bool Bitmap::get(size_t index) const {
  char bits = bitmap_[index / 8];
  return (bits & (1 << (index % 8))) != 0;
}

int Bitmap::next_zero_bit(int start) const {
  int ret = -1;
  int start_in_byte = start % 8;
  for (int iter = start / 8, end = bytes(); iter < end; iter++) {
    char byte = bitmap_[iter];
    if (byte != -1) {
      int index_in_byte = find_first_zero(byte, start_in_byte);
      if (index_in_byte >= 0) {
        ret = iter * 8 + index_in_byte;
        break;
      }
    }
    start_in_byte = 0;
  }

  if (ret >= size_) {
    ret = -1;
  }
  return ret;
}

int Bitmap::next_one_bit(int start) const {
  int ret = -1;
  int start_in_byte = start % 8;
  for (int iter = start / 8, end = bytes(); iter < end; iter++) {
    char byte = bitmap_[iter];
    if (byte != 0x00) {
      int index_in_byte = find_first_one(byte, start_in_byte);
      if (index_in_byte >= 0) {
        ret = iter * 8 + index_in_byte;
        break;
      }
    }
    start_in_byte = 0;
  }

  if (ret >= size_) {
    ret = -1;
  }
  return ret;
}

std::string Bitmap::to_string() const {
  std::stringstream ss;
  for (int i = 0; i < size_; i++) {
    ss << (get(i) ? "1" : "0");
  }
  return ss.str();
}