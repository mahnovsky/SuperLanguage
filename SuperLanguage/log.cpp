#include "log.hpp"

namespace details
{
	void write_buff(char const* buff, size_t size, FILE* stream)
	{
		constexpr std::string_view info = "info: ";
		[[maybe_unused]] auto unused0 = fwrite(info.data(), 1, info.size(), stream);
		[[maybe_unused]] auto unused1 = fwrite(buff, 1, size, stream);
		[[maybe_unused]] auto unused2 = fputc('\n', stream);
	}
}