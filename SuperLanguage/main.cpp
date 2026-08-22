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
#include "hive.h"
#include "new_ast.h"

namespace data
{
	Storage storage;
}

void init_internal_functions(Interpreter* interp)
{
	using namespace data;

	interp->add_internal_function("__print", [](Interpreter* interp, data::Scope* s)
	{
		std::string res = "--> ";
		const auto vars = s->get_variables();
			
		for(const auto v : vars)
		{
			const auto obj = interp->get_stack_variable(v);

			res.append(obj->to_string());
			//res.append(" ");
		}

		puts(res.c_str());
	});

	interp->add_internal_function("__dump_callstack", [](Interpreter* interp, data::Scope* s)
	{
		puts("Callstack dump:");
		const auto& cs = interp->get_call_stack();
		for(const auto& key : cs | std::views::keys)
		{
			putc('\t', stdout);
			puts(key.c_str());
		}
	});

	interp->add_internal_function("__to_string", [](Interpreter* interp, data::Scope* s)
	{
		const auto vars = s->get_variables();
		if (vars.size() == 1)
		{
			const auto obj = interp->get_stack_variable(vars.back());

			interp->set_return_value(std::make_shared<String>(obj->to_string()));
		}
	});

	interp->add_internal_function("__exit", [](Interpreter* interp, data::Scope* s)
	{
		const auto vars = s->get_variables();
		if (vars.size() == 1)
		{
			auto obj = interp->get_stack_variable(vars.back());
			int res = 0;
			if (obj->get(&res))
			{
				exit(res);
			}
		}
	});

	interp->add_internal_function("__get_array_element", [](Interpreter* interp, data::Scope* s)
	{
		const auto vars = s->get_variables();
		if (vars.size() == 2)
		{
			const auto array_obj = interp->get_stack_variable(vars.front());
			const auto index_obj = interp->get_stack_variable(vars.back());

			int index = 0;
			std::vector<ObjectPtr>* arr;
			if (array_obj->get(&arr) && index_obj->get(&index))
			{
				interp->set_return_value(arr->at(index));
			}
		}
	});

	interp->add_internal_function("__set_array_element", [](Interpreter* interp, data::Scope* s)
	{
		const auto vars = s->get_variables();
		if (vars.size() == 3)
		{
			const auto array_obj = interp->get_stack_variable(vars.front());
			const auto index_obj = interp->get_stack_variable(vars[1]);
			const auto obj = interp->get_stack_variable(vars.back());

			int index = 0;
			std::vector<ObjectPtr>* arr;
			if (array_obj->get(&arr) && index_obj->get(&index) && obj)
			{
				if (arr->size() > index) 
				{
					(*arr)[index] = obj;
				}
			}
		}
	});

	interp->add_internal_function("__get_array_size", [](Interpreter* interp, data::Scope* s)
	{
		const auto vars = s->get_variables();
		if (vars.size() == 1)
		{
			const auto array_obj = interp->get_stack_variable(vars.front());
				
			std::vector<ObjectPtr>* arr;
			if (array_obj->get(&arr))
			{
				interp->set_return_value(std::make_shared<Integer>(arr->size()));
			}
		}
	});

	interp->add_internal_function("__array_append", [](Interpreter* interp, data::Scope* s)
	{
		const auto vars = s->get_variables();
		if (vars.size() == 2)
		{
			const auto array_obj = interp->get_stack_variable(vars.front());
			const auto obj = interp->get_stack_variable(vars.back());
			std::vector<ObjectPtr>* arr;
			if (array_obj->get(&arr) && obj)
			{
				arr->emplace_back(obj);
			}
		}
	});
}

struct Foo
{
	static inline constexpr auto Index = new_ast::Node::Index::Variable;
	std::string name;

	Foo(const std::string& n)
		: name(n)
	{
	}
};

void test_hive()
{
	
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
