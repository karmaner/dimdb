#include "common/utils/utils.h"

bool starts_with(std::string_view str, std::string_view prefix) {
  return str.size() >= prefix.size() &&
    str.substr(0, prefix.size()) == prefix;
}

bool ends_with(std::string_view str, std::string_view suffix) {
  return str.size() >= suffix.size() &&
    str.substr(str.size() - suffix.size()) == suffix;
}