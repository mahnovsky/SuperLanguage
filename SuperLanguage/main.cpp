#include <iostream>
#include <map>
#include <ranges>
#include <vector>
#include <string>

#include "lexer.hpp"
#include "nodes.hpp"
#include "parser.hpp"
#include "interpreter.hpp"
#include "log.hpp"
#include "number.hpp"

void init_internal_functions(Interpreter* interp)
{
	interp->add_internal_function(new InternalFunction("__print", [](Interpreter* interp, Scope* s)
		{
			std::string res = "--> ";
			auto vars = s->get_variables();
			for(auto v : vars)
			{
				auto obj = interp->get_stack_variable(v);

				std::string str;
				if(obj->get(&str))
				{
					res += str;
					continue;
				}

				int ival;
				if(obj->get(&ival))
				{
					res += std::to_string(ival);
					continue;
				}

				float fval;
				if (obj->get(&fval))
				{
					res += std::to_string(fval);
					continue;
				}

				bool bval;
				if (obj->get(&bval))
				{
					res += (bval ? "true" : "false");
					continue;
				}
			}

			puts(res.c_str());
		}));

	interp->add_internal_function(new InternalFunction("__dump_callstack", [](Interpreter* interp, Scope* s)
		{
			puts("Callstack dump:");
			const auto& cs = interp->get_call_stack();
			for(const auto& key : cs | std::views::keys)
			{
				putc('\t', stdout);
				puts(key.c_str());
			}
		}));

	interp->add_internal_function(new InternalFunction("__exit", [](Interpreter* interp, Scope* s)
		{
			auto vars = s->get_variables();
			if (vars.size() == 1)
			{
				auto obj = interp->get_stack_variable(vars.back());
				int res = 0;
				if (obj->get(&res))
				{
					exit(res);
				}
			}
		}));
}

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		std::cerr << "Invalid args, script file name missing\n";
		return EXIT_FAILURE;
	}

	const auto file_source = readFile(argv[1]);
	if(!file_source.has_value())
	{
		return EXIT_FAILURE;
	}
	Lexer lexer;

	Parser p{ lexer.tokenize(file_source.value()) };
	
	Interpreter interpreter(p.parse());

	init_internal_functions(&interpreter);

	interpreter.run();

	char line[256];

	while (fgets(line, sizeof line, stdin) != NULL)
	{
		if(line[0] == '\n')
		{
			break;
		}
		Lexer lex;
		interpreter.run_once(p.add_tokens(lex.tokenize(line)));
	}

	return EXIT_SUCCESS;
}
