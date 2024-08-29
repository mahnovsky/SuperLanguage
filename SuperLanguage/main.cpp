#include <iostream>
#include <map>
#include <vector>
#include <string>

#include "lexer.hpp"
#include "nodes.hpp"
#include "parser.hpp"
#include "interpreter.hpp"


void init_internal_functions(Interpreter* interp)
{
	interp->add_internal_function(new InternalFunction("prints", [](Interpreter* interp, Scope* s)
		{
			std::string res = "\tOutput> ";
			auto vars = s->get_variables();
			for(auto v : vars)
			{
				auto obj = interp->get_stack_variable(v);

				std::string str;
				if(obj->get(&str))
				{
					res += str;
				}

				int ival;
				if(obj->get(&ival))
				{
					res += std::to_string(ival);
				}

				float fval;
				if (obj->get(&fval))
				{
					res += std::to_string(fval);
				}
			}

			puts(res.c_str());
		}));
}

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
	
	Interpreter interpreter(p.parse());

	init_internal_functions(&interpreter);

	interpreter.run();

	return EXIT_SUCCESS;
}
