#pragma once

#include <string_view>

bool starts_with(std::string_view str, std::string_view prefix);

bool ends_with(std::string_view str, std::string_view suffix);