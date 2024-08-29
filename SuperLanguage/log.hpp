#pragma once

#include <cstdio>
#include <format>

#define SHOW_INFO_LOG 1

namespace details
{
	void write_buff(char const* buff, size_t size, FILE* stream);
}

#if (SHOW_INFO_LOG)
#define LOG_INFO(fmt_str, ...) \
	{ \
		const auto log_str = std::format(fmt_str, __VA_ARGS__); \
		details::write_buff(log_str.c_str(), log_str.size(), stdout); \
	}
#else
#define LOG_INFO(...)
#endif