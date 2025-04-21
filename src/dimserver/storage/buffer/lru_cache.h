#pragma once

#include <list>
#include <unordered_map>
#include <functional>

template <typename Key, typename Value, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>>
class LruCache {
private:
  struct CacheItem {
    Key key;
    Value value;

    CacheItem(const Key& k, const Value& v) : key(k), value(v) { };
  };

  std::list<CacheItem> items_; // 存储lru项目
  std::unordered_map<Key, typename std::list<CacheItem>::iterator, Hash, Pred> lookup_; // 存储key和对应的迭代器
public:
  LruCache(size_t reserve) {
    if (reserve > 0) {
      lookup_.reserve(reserve);
    }
  }

  ~LruCache() { destroy(); }
  void destroy() {
    items_.clear();
    lookup_.clear();
  }

  size_t count() const { return lookup_.size(); }

  bool get(const Key& key, Value& value) {
    auto iter = lookup_.find(key);
    if (iter == lookup_.end()) {
      return false;
    }
    items_.splice(items_.begin(), items_, iter->second);
    value = iter->second->value;
    return true;
  }
  
  void put(const Key& key, const Value& value) {
    auto iter = lookup_.find(key);
    if (iter != lookup_.end()) {
      iter->second->value = value;
      items_.splice(items_.begin(), items_, iter->second);
      return;
    }

    items_.emplace_front(key, value);
    lookup_[key] = items_.begin();
  }

  void remove(const Key& key) {
    auto iter = lookup_.find(key);
    if (iter != lookup_.end()) {
      items_.erase(iter->second);
      lookup_.erase(iter);
    }
  }

  void pop(Value*& value) {
    if (items_.empty()) {
      value = nullptr;
      return;
    }
    value = new Value(items_.back().value);
    lookup_.erase(items_.back().key);
    items_.pop_back();
  }

  void pop() {
    if (items_.empty()) {
      return;
    }

    auto& oldest_item = items_.back();
    lookup_.erase(oldest_item.key);
    items_.pop_back();
  }

  void foreach_reverse(std::function<bool(const Key&, const Value&)> func) {
    for (auto it = items_.rbegin(); it != items_.rend(); ++it) {
      bool ret = func(it->key, it->value);
      if (!ret) {
        break;
      }
    }
  }

  void foreach(std::function<bool(const Key&, const Value&)> func) {
    for (auto it = items_.begin(); it != items_.end(); ++it) {
      bool ret = func(it->key, it->value);
      if (!ret) {
        break;
      }
    }
  }
};