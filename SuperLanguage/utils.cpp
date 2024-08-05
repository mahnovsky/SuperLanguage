#include "utils.hpp"

#include <iostream>


std::vector<std::string> split_by_lines(const std::string& str)
{
	std::vector<std::string> lines;
	size_t pos = 0;
	while (pos != std::string::npos)
	{
		const auto prev = pos;
		pos = str.find('\n', pos);
		if (pos != std::string::npos)
		{
			pos += 1;
			const size_t len = pos - prev;
			lines.push_back(str.substr(prev, len));

			if (str[pos] == '\r')
			{
				pos += 1;
			}
		}
		else
		{
			lines.push_back(str.substr(prev) + '\n');
		}
	}

	return lines;
}

bool is_digit(const char* str)
{
	if (str[0] == 0)
	{
		return false;
	}

	int pos = 0;
	do
	{
		if (isdigit(str[pos++]) == 0)
		{
			return false;
		}
	} while (str[pos] != 0);

	return true;
}

std::optional<std::string> readFile(const char* fileName)
{
	FILE* f = nullptr;
	const auto status = fopen_s(&f, fileName, "r");
	if (status != 0)
	{
		char buff[1024];
		if (strerror_s(buff, 1024, status) == 0)
		{
			std::cerr << "Failed to open source file: " << fileName
				<< "error: " << buff << '\n';
		}

		return {};
	}

	if(fseek(f, 0L, SEEK_END) != 0)
	{
		std::cerr << "Failed seek to end of file\n";
		return {};
	}
	size_t sz = ftell(f);
	if(fseek(f, 0L, SEEK_SET) != 0)
	{
		std::cerr << "Failed seek to begin of file\n";
		return {};
	}

	const std::unique_ptr<char[]> source { static_cast<char*>(malloc(sz + 1)) };
	if (!source)
	{
		std::cerr << "Failed to create buffer by malloc\n";
		return {};
	}

	memset(source.get(), 0, sz + 1);
	if (fread(source.get(), 1, sz, f) > 0)
	{
		return { source.get() };
	}

	return {};
}