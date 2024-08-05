#pragma once

#include <optional>
#include <string>
#include <vector>

std::vector<std::string> split_by_lines(const std::string& str);

bool is_digit(const char* str);

std::optional<std::string> readFile(const char* fileName);