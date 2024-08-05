#include <iostream>
#include <map>
#include <vector>

#include "lexer.hpp"
#include "nodes.hpp"
#include "parser.hpp"
#include "interpreter.hpp"

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		std::cerr << "Invalid args, script file name missing\n";
		return EXIT_FAILURE;
	}

	const auto fileSource = readFile(argv[1]);
	if(!fileSource.has_value())
	{
		return EXIT_FAILURE;
	}
	Lexer lexer;

	Parser p{ lexer.tokenize(fileSource.value()) };
	
	Interpreter iterpreter(p.parse());

	iterpreter.run();

	return EXIT_SUCCESS;
}
