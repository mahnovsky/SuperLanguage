#include "log.hpp"

namespace details
{
	void write_buff(char const* buff, size_t size, FILE* stream)
	{
		[[maybe_unused]] auto unused1 = fwrite(buff, 1, size, stream);
	}

	void write_buff_ln(char const* buff, size_t size, FILE* stream)
	{
		[[maybe_unused]] auto unused1 = fwrite(buff, 1, size, stream);
		[[maybe_unused]] auto unused2 = fputc('\n', stream);
	}
}